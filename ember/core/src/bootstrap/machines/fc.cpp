/*
 * ------------------------------------------------------------------------
 * 名称：fc.cpp
 * 说明：主机型号 fc（经典 FC，NTSC）的装配、电源与运行。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：CPU 总线：$0000-$1FFF 内部 RAM（2K 镜像）、$2000-$3FFF PPU（每 8 字节镜像）、$4000-$4013 与 $4015 APU、
 *       $4014 精灵 DMA、$4016-$4017 手柄口（写 $4017 同时转给 APU 的帧计数器）、$4020-$FFFF 卡槽；$4018-$401F 无设备（开放总线）；
 *       PPU 总线：$0000-$1FFF 图案表（卡槽）、$2000-$3EFF 名称表（按显存控制线选择主板显存或卡带），调色板在 PPU 内。
 * ------------------------------------------------------------------------
 */

#include "bootstrap/machines/fc.hpp"

#include <algorithm>
#include <cstdio>
#include <string>
#include <utility>
#include "contracts/hardware/hardware-error.hpp"
#include "shared/errors/core-error.hpp"

namespace ember::bootstrap {

namespace hw = contracts::hardware;

namespace {

/** 内部 RAM 的容量（字节）。 */
constexpr std::size_t INTERNAL_RAM_SIZE = 2048;

}  // namespace

FcMachine::FcMachine()
    : cartridgeIrq_(irq_),
      ram_(INTERNAL_RAM_SIZE),
      ramDevice_(ram_),
      nametableRam_(slot_),
      apu_(cpuBus_, scheduler_, irq_, rdy_, hw::ClockDomain{CPU_TICKS_PER_CYCLE}, MASTER_CLOCK_HZ / static_cast<double>(CPU_TICKS_PER_CYCLE)),
      ppu_(scheduler_, ppuBus_, nmi_, hw::ClockDomain{PPU_TICKS_PER_DOT}, PPU_PHASE_TICKS),
      dma_(cpuBus_, scheduler_, rdy_, hw::ClockDomain{CPU_TICKS_PER_CYCLE}),
      padApuPort_(pad_, apu_),
      cpu_(cpuBus_, scheduler_, hw::ClockDomain{CPU_TICKS_PER_CYCLE}, irq_, nmi_, rdy_),
      peek_([this](std::uint16_t address) { return cpuBus_.peek(address); }),
      recorders_(peek_),
      probeHub_(breakpoints_, recorders_) {
  cpuBus_.mount(0x0000, 0x1FFF, ramDevice_);
  cpuBus_.mount(0x2000, 0x3FFF, ppu_);
  cpuBus_.mount(0x4000, 0x4013, apu_);
  cpuBus_.mount(0x4014, 0x4014, dma_);
  cpuBus_.mount(0x4015, 0x4015, apu_);
  cpuBus_.mount(0x4016, 0x4017, padApuPort_);
  cpuBus_.mount(0x4020, 0xFFFF, slot_.cpuSide());

  ppuBus_.mount(0x0000, 0x1FFF, slot_.ppuSide());
  ppuBus_.mount(0x2000, 0x3EFF, nametableRam_);

  for (unsigned index = 0; index < joypads_.size(); ++index) pad_.port(index).plug(&joypads_[index]);
  cpu_.setInterruptObserver(this);
}

void FcMachine::powerOn() {
  // 时钟先归零并丢弃旧事件，各设备复位时再登记自己的事件；CPU 最后复位，复位序列会推进时钟。
  scheduler_.reset();
  ram_.fill(0);
  nametableRam_.fill(0);
  apu_.reset(true);
  dma_.reset(true);
  ppu_.reset(true);
  cartridgeIrq_.set(false);
  slot_.reset(true);
  poweredOn_ = true;
  skipExecuteAt_.reset();
  cpu_.reset(true);
  breakpoints_.discardPending();
}

void FcMachine::powerOff() { poweredOn_ = false; }

void FcMachine::softReset() {
  if (!poweredOn_) return;
  apu_.reset(false);
  dma_.reset(false);
  ppu_.reset(false);
  slot_.reset(false);
  cpu_.reset(false);
  skipExecuteAt_.reset();
  breakpoints_.discardPending();
}

void FcMachine::run(hw::MasterTime ticks) {
  if (!poweredOn_ || ticks <= 0) return;
  const hw::MasterTime cycles = (ticks + CPU_TICKS_PER_CYCLE - 1) / CPU_TICKS_PER_CYCLE;
  cpu_.run(static_cast<std::uint32_t>(cycles));
  ppu_.catchUp(scheduler_.now());
  apu_.catchUp(scheduler_.now());
}

void FcMachine::runFrame() {
  if (!poweredOn_) return;
  const std::uint64_t target = ppu_.frameCount() + 1;
  while (ppu_.frameCount() < target) {
    if (breakpoints_.hasExecute()) {
      const std::uint16_t pc = cpu_.registers().pc;
      const bool skip = skipExecuteAt_ == pc;
      skipExecuteAt_.reset();
      if (!skip && breakpoints_.hitsExecute(pc)) {
        skipExecuteAt_ = pc;
        return;
      }
    }
    beforeInstruction();
    cpu_.run(1);
    ppu_.catchUp(scheduler_.now());
    if (breakpoints_.hasPending()) return;
  }
  apu_.catchUp(scheduler_.now());
}

void FcMachine::takeAudio(std::vector<float>& samples) {
  if (poweredOn_) apu_.catchUp(scheduler_.now());
  apu_.takeAudio(samples);
}

void FcMachine::setButtons(unsigned port, std::uint8_t buttons) {
  if (port >= joypads_.size()) throw shared::CoreError(hw::ERROR_PORT_NOT_FOUND, "手柄口只有 2 个，编号为 0 与 1");
  joypads_[port].setButtons(buttons);
}

// ---------------------------------------------------------------- 调试接口

namespace {

constexpr std::size_t CPU_SPACE_SIZE = 0x10000;
constexpr std::size_t PPU_SPACE_SIZE = 0x4000;
constexpr std::size_t NAMETABLE_SIZE = 2048;
constexpr std::size_t OAM_SIZE = 256;
constexpr std::size_t PALETTE_SIZE = 32;

/** 调色板的存储下标：$3F10、$3F14、$3F18、$3F1C 镜像 $3F00、$3F04、$3F08、$3F0C。 */
std::size_t paletteSlot(std::uint32_t address) {
  std::size_t index = address & 0x1FU;
  if ((index & 0x13U) == 0x10U) index &= ~std::size_t{0x10U};
  return index;
}

/** 检查范围，越界时抛出 hardware.spaceOutOfRange。 */
void checkRange(std::string_view space, std::size_t size, std::uint32_t address, std::size_t length) {
  if (address > size || length > size - address) {
    throw shared::CoreError(hw::ERROR_SPACE_OUT_OF_RANGE, "空间 " + std::string(space) + " 的大小是 " + std::to_string(size) + " 字节，访问 " + std::to_string(address) + " 起的 " + std::to_string(length) + " 字节超出范围");
  }
}

[[noreturn]] void unknownSpace(std::string_view space) { throw shared::CoreError(hw::ERROR_UNKNOWN_SPACE, "没有这个内存空间：" + std::string(space)); }

hw::StateField field(const char* name, std::int64_t value, const char* description) { return hw::StateField{name, value, description}; }

}  // namespace

std::vector<hw::MemorySpace> FcMachine::memorySpaces() const {
  return {
      {"cpu", "CPU 总线 $0000-$FFFF：读不产生副作用，寄存器类地址读出 0；写经真实的总线，与 CPU 写一样有副作用", CPU_SPACE_SIZE, true},
      {"ppu", "PPU 总线 $0000-$3FFF：图案表与名称表经卡槽，$3F00 起是调色板；读不产生副作用", PPU_SPACE_SIZE, true},
      {"board.ram", "主板的内部 RAM（2K）", INTERNAL_RAM_SIZE, true},
      {"board.nametable", "主板的名称表显存（2K）", NAMETABLE_SIZE, true},
      {"chip.oam", "PPU 的精灵属性存储（OAM）", OAM_SIZE, true},
      {"chip.palette", "PPU 的调色板（32 字节，$3F10、$3F14、$3F18、$3F1C 是 $3F00、$3F04、$3F08、$3F0C 的镜像，这里是未镜像的存储）", PALETTE_SIZE, true},
  };
}

void FcMachine::readMemory(std::string_view space, std::uint32_t address, std::span<std::uint8_t> out) {
  if (space == "cpu") {
    checkRange(space, CPU_SPACE_SIZE, address, out.size());
    for (std::size_t index = 0; index < out.size(); ++index) out[index] = cpuBus_.peek(static_cast<std::uint16_t>(address + index));
  } else if (space == "ppu") {
    checkRange(space, PPU_SPACE_SIZE, address, out.size());
    for (std::size_t index = 0; index < out.size(); ++index) {
      const auto target = static_cast<std::uint16_t>(address + index);
      out[index] = target < 0x3F00U ? ppuBus_.peek(target) : ppu_.palette()[paletteSlot(target)];
    }
  } else if (space == "board.ram") {
    checkRange(space, INTERNAL_RAM_SIZE, address, out.size());
    ram_.readBlock(address, out);
  } else if (space == "board.nametable") {
    checkRange(space, NAMETABLE_SIZE, address, out.size());
    std::copy_n(nametableRam_.bytes().begin() + address, out.size(), out.begin());
  } else if (space == "chip.oam") {
    checkRange(space, OAM_SIZE, address, out.size());
    std::copy_n(ppu_.oam().begin() + address, out.size(), out.begin());
  } else if (space == "chip.palette") {
    checkRange(space, PALETTE_SIZE, address, out.size());
    std::copy_n(ppu_.palette().begin() + address, out.size(), out.begin());
  } else {
    unknownSpace(space);
  }
}

void FcMachine::writeMemory(std::string_view space, std::uint32_t address, std::span<const std::uint8_t> bytes) {
  // 调试写入不触发读写断点，也不进记录。
  const ProbeSuspend suspend(*this, true);
  if (space == "cpu") {
    checkRange(space, CPU_SPACE_SIZE, address, bytes.size());
    for (std::size_t index = 0; index < bytes.size(); ++index) cpuBus_.write(static_cast<std::uint16_t>(address + index), bytes[index]);
  } else if (space == "ppu") {
    checkRange(space, PPU_SPACE_SIZE, address, bytes.size());
    for (std::size_t index = 0; index < bytes.size(); ++index) {
      const auto target = static_cast<std::uint16_t>(address + index);
      if (target < 0x3F00U) ppuBus_.write(target, bytes[index]);
      else ppu_.pokePalette(paletteSlot(target), bytes[index]);
    }
  } else if (space == "board.ram") {
    checkRange(space, INTERNAL_RAM_SIZE, address, bytes.size());
    for (std::size_t index = 0; index < bytes.size(); ++index) ram_.write(address + index, bytes[index]);
  } else if (space == "board.nametable") {
    checkRange(space, NAMETABLE_SIZE, address, bytes.size());
    for (std::size_t index = 0; index < bytes.size(); ++index) nametableRam_.poke(address + index, bytes[index]);
  } else if (space == "chip.oam") {
    checkRange(space, OAM_SIZE, address, bytes.size());
    for (std::size_t index = 0; index < bytes.size(); ++index) ppu_.pokeOam(address + index, bytes[index]);
  } else if (space == "chip.palette") {
    checkRange(space, PALETTE_SIZE, address, bytes.size());
    for (std::size_t index = 0; index < bytes.size(); ++index) ppu_.pokePalette(address + index, bytes[index]);
  } else {
    unknownSpace(space);
  }
}

hw::AddressTarget FcMachine::resolve(std::string_view space, std::uint32_t address) const {
  if (space == "cpu") {
    checkRange(space, CPU_SPACE_SIZE, address, 1);
    if (address < 0x2000U) return {"ram", "board.ram", address & (INTERNAL_RAM_SIZE - 1), "内部 RAM，$0000-$1FFF 每 2K 镜像一次"};
    if (address < 0x4000U) return {"register", "chip.ppu", address & 7U, "PPU 寄存器，$2000-$3FFF 每 8 字节镜像一次"};
    if (address == 0x4014U) return {"register", "chip.dma", 0, "精灵 DMA"};
    if (address <= 0x4013U || address == 0x4015U) return {"register", "chip.apu", address - 0x4000U, "APU 寄存器"};
    if (address <= 0x4017U) return {"register", "chip.pad", address - 0x4016U, "手柄口（写 $4017 同时属于 APU 的帧计数器）"};
    if (address < 0x4020U) return {"none", "", 0, "没有设备，读到总线上最近的值"};
    return {"cartridge", "", address, "由卡槽里的卡带响应"};
  }
  if (space == "ppu") {
    checkRange(space, PPU_SPACE_SIZE, address, 1);
    if (address < 0x2000U) return {"cartridge", "", address, "图案表，由卡槽里的卡带响应"};
    if (address < 0x3F00U) {
      const hw::CiramControl control = slot_.ciramControl(static_cast<std::uint16_t>(address));
      if (!control.enable) return {"cartridge", "", address, "名称表，由卡带自带的显存响应"};
      return {"ram", "board.nametable", (control.a10 ? 0x400U : 0U) | (address & 0x3FFU), "名称表，由主板的显存响应（连法由卡带决定）"};
    }
    return {"ram", "chip.palette", paletteSlot(address), "调色板（每 32 字节镜像一次）"};
  }
  unknownSpace(space);
}

std::vector<std::string> FcMachine::stateModules() const { return {"cpu", "ppu"}; }

std::vector<hw::StateField> FcMachine::readState(std::string_view module) const {
  if (module == "cpu") {
    const hw::CpuRegisters registers = cpu_.registers();
    return {field("pc", registers.pc, "程序计数器，指向下一条要执行的指令"),
            field("a", registers.a, "累加器"),
            field("x", registers.x, "X 变址寄存器"),
            field("y", registers.y, "Y 变址寄存器"),
            field("sp", registers.sp, "栈指针，栈在 $0100-$01FF"),
            field("status", registers.status, "状态寄存器（N V - - D I Z C，位 5 恒为 1，位 4 恒为 0）"),
            field("cycles", static_cast<std::int64_t>(registers.cycles), "自上电以来的 CPU 周期数（只读）")};
  }
  if (module == "ppu") {
    return {field("scanline", ppu_.scanline(), "当前扫描线（0 到 261，261 是预渲染行）"),
            field("dot", ppu_.dot(), "当前点（0 到 340）"),
            field("frame", static_cast<std::int64_t>(ppu_.frameCount()), "已完成的帧数"),
            field("vramAddress", ppu_.vramAddress(), "当前的显存地址 v（15 位）"),
            field("tempAddress", ppu_.tempAddress(), "临时显存地址 t（15 位）"),
            field("fineX", ppu_.fineX(), "精细 X 滚动（0 到 7）"),
            field("writeToggle", ppu_.writeToggle() ? 1 : 0, "写锁存 w"),
            field("vblank", ppu_.vblankFlag() ? 1 : 0, "vblank 标志"),
            field("spriteZeroHit", ppu_.spriteZeroHit() ? 1 : 0, "精灵 0 命中标志"),
            field("spriteOverflow", ppu_.spriteOverflow() ? 1 : 0, "精灵溢出标志")};
  }
  throw shared::CoreError(hw::ERROR_INVALID_STATE, "没有这个状态模块：" + std::string(module));
}

void FcMachine::writeState(std::string_view module, std::span<const hw::StateField> fields) {
  if (module == "ppu") throw shared::CoreError(hw::ERROR_INVALID_STATE, "ppu 的状态只读");
  if (module != "cpu") throw shared::CoreError(hw::ERROR_INVALID_STATE, "没有这个状态模块：" + std::string(module));
  hw::CpuRegisters registers = cpu_.registers();
  for (const hw::StateField& item : fields) {
    const auto inRange = [&](std::int64_t maximum) {
      if (item.value < 0 || item.value > maximum) throw shared::CoreError(hw::ERROR_INVALID_STATE, "cpu." + item.name + " 的取值必须在 0 到 " + std::to_string(maximum) + " 之间");
    };
    if (item.name == "pc") {
      inRange(0xFFFF);
      registers.pc = static_cast<std::uint16_t>(item.value);
    } else if (item.name == "a" || item.name == "x" || item.name == "y" || item.name == "sp" || item.name == "status") {
      inRange(0xFF);
      const auto value = static_cast<std::uint8_t>(item.value);
      if (item.name == "a") registers.a = value;
      else if (item.name == "x") registers.x = value;
      else if (item.name == "y") registers.y = value;
      else if (item.name == "sp") registers.sp = value;
      else registers.status = value;
    } else {
      throw shared::CoreError(hw::ERROR_INVALID_STATE, "cpu 没有可修改的状态项：" + item.name);
    }
  }
  cpu_.setRegisters(registers);
  skipExecuteAt_.reset();
}

void FcMachine::refreshProbe() { cpuBus_.setProbe(probeHub_.needed() ? &probeHub_ : nullptr); }

FcMachine::ProbeSuspend::ProbeSuspend(FcMachine& machine, bool alsoRecording) : machine_(machine), before_(machine.probeHub_.suspend(true, alsoRecording)) {}

FcMachine::ProbeSuspend::~ProbeSuspend() { machine_.probeHub_.restore(before_); }

void FcMachine::beforeInstruction() {
  const bool recording = recorders_.needsInstructionHook();
  if (!recording && !cdl_.active()) return;
  const hw::CpuRegisters registers = cpu_.registers();
  if (recording) recorders_.onInstruction(registers);
  if (cdl_.active()) cdl_.onInstruction(registers, peek_);
}

std::vector<hw::DisassembledInstruction> FcMachine::disassemble(std::uint16_t address, std::uint32_t count) {
  std::vector<hw::DisassembledInstruction> list;
  list.reserve(count);
  const hw::CpuRegisters registers = cpu_.registers();
  std::uint16_t pc = address;
  for (std::uint32_t index = 0; index < count; ++index) {
    const logic::hardware::Disassembly code = logic::hardware::disassemble(pc, registers, peek_);
    std::string bytes;
    for (std::uint8_t offset = 0; offset < code.length; ++offset) {
      char text[4];
      std::snprintf(text, sizeof(text), "%02X", peek_(static_cast<std::uint16_t>(pc + offset)));
      if (offset > 0) bytes += ' ';
      bytes += text;
    }
    list.push_back({pc, std::move(bytes), code.text, code.unofficial});
    pc = static_cast<std::uint16_t>(pc + code.length);
  }
  return list;
}

std::uint32_t FcMachine::startRecorder(const hw::RecorderSpec& spec) {
  const std::uint32_t id = recorders_.start(spec);
  refreshProbe();
  return id;
}

hw::RecorderOutput FcMachine::stopRecorder(std::uint32_t id) {
  hw::RecorderOutput output = recorders_.stop(id);
  refreshProbe();
  return output;
}

void FcMachine::onInterrupt(hw::InterruptKind kind, std::uint16_t returnAddress, std::uint16_t handler) {
  if (recorders_.recordsIrq()) recorders_.onInterrupt(kind, returnAddress, handler, cpu_.registers().cycles);
}

hw::Breakpoint FcMachine::addBreakpoint(hw::BreakpointKind kind, std::uint16_t first, std::uint16_t last) {
  const hw::Breakpoint added = breakpoints_.add(kind, first, last);
  refreshProbe();
  return added;
}

bool FcMachine::removeBreakpoint(std::uint32_t id) {
  const bool removed = breakpoints_.remove(id);
  refreshProbe();
  return removed;
}

std::size_t FcMachine::clearBreakpoints() {
  const std::size_t removed = breakpoints_.clear();
  refreshProbe();
  return removed;
}

hw::StepResult FcMachine::step(hw::StepUnit unit, std::uint64_t count) {
  hw::StepResult result;
  if (!poweredOn_ || count == 0) return result;
  // 单步忽略断点，但照常进记录。
  const ProbeSuspend suspend(*this, false);
  skipExecuteAt_.reset();
  breakpoints_.discardPending();
  const std::uint64_t startCycles = cpu_.registers().cycles;
  const std::uint64_t startFrames = ppu_.frameCount();
  // 每次 run(1) 执行恰好一条指令（含随后响应的中断序列）。
  const auto instruction = [&] {
    beforeInstruction();
    cpu_.run(1);
    ppu_.catchUp(scheduler_.now());
    ++result.instructions;
  };
  switch (unit) {
    case hw::StepUnit::Instruction:
      for (std::uint64_t done = 0; done < count; ++done) instruction();
      break;
    case hw::StepUnit::Cycle:
      while (cpu_.registers().cycles - startCycles < count) instruction();
      break;
    case hw::StepUnit::Scanline: {
      int last = ppu_.scanline();
      for (std::uint64_t remaining = count; remaining > 0;) {
        instruction();
        if (ppu_.scanline() != last) {
          last = ppu_.scanline();
          --remaining;
        }
      }
      break;
    }
    case hw::StepUnit::Frame:
      while (ppu_.frameCount() - startFrames < count) instruction();
      break;
  }
  apu_.catchUp(scheduler_.now());
  result.cycles = cpu_.registers().cycles - startCycles;
  result.frames = ppu_.frameCount() - startFrames;
  return result;
}

}  // namespace ember::bootstrap
