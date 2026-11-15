/*
 * ------------------------------------------------------------------------
 * 名称：sb2000.cpp
 * 说明：主机型号 sb2000（小霸王 2000 学习机，PAL）的装配、电源、运行、输入与调试接口。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：CPU 总线：$0000-$1FFF 内部 RAM（2K 镜像）、$2000-$3FFF 图形端口（$2000-$2008、调色板 $2040-$207F）、
 *       $4000-$4013 与 $4015 PSG、$4014 精灵 DMA、$4016-$4017 手柄口、$4020-$4022 键盘、$4023-$4024 鼠标、$4025 并口数据、$4026 扩展 I/O、$4027 DAC、
 *       $4031 初始设置、$4032-$4033 中断、$4034-$4036 定时器、$4040-$4047 程序分页、$4048-$404F 通用 DMA、$4080-$4081 实时时钟、
 *       $4100-$43FF 超级 I/O（显存页、语音存根）、$4400-$5FFF 工作窗口、$6000-$7FFF 外部 RAM（8K）、$8000-$FFFF 程序窗口（分页到 BIOS ROM 或扩展 RAM）；
 *       没有器件的地址（如 $4018-$401F）读到总线上最近的值；
 *       显存总线（16 位）：$0000-$7FFF 字符 RAM，$8000-$FFFF 扩展显存的 32K 窗口。
 * ------------------------------------------------------------------------
 */

#include "bootstrap/machines/sb2000.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>

#include "contracts/hardware/hardware-error.hpp"
#include "shared/errors/core-error.hpp"

namespace ember::bootstrap {

namespace hw = contracts::hardware;

namespace {

using debugging::checkRange;
using debugging::field;
using debugging::unknownSpace;

constexpr std::size_t INTERNAL_RAM_SIZE = 2048;
constexpr std::size_t EXTERNAL_RAM_SIZE = 8192;
constexpr std::size_t CPU_SPACE_SIZE = 0x10000;
constexpr std::size_t VIDEO_SPACE_SIZE = 0x10000;
constexpr std::size_t OAM_SIZE = 256;
constexpr std::size_t PALETTE_SIZE = 64;
/** 每行的主时钟周期数（341 点）。 */
constexpr hw::MasterTime LINE_TICKS = static_cast<hw::MasterTime>(logic::hardware::PpuUm6576::DOTS_PER_SCANLINE) * Sb2000Machine::PPU_TICKS_PER_DOT;

/** 游戏手柄标准映射里的按键下标（浏览器 Gamepad API）。 */
constexpr std::size_t GAMEPAD_SOUTH = 0;
constexpr std::size_t GAMEPAD_EAST = 1;
constexpr std::size_t GAMEPAD_SELECT = 8;
constexpr std::size_t GAMEPAD_START = 9;
constexpr std::size_t GAMEPAD_UP = 12;
constexpr std::size_t GAMEPAD_DOWN = 13;
constexpr std::size_t GAMEPAD_LEFT = 14;
constexpr std::size_t GAMEPAD_RIGHT = 15;
/** 摇杆算作按下的偏移量。 */
constexpr double AXIS_THRESHOLD = 0.5;

void requireBios(const std::vector<std::uint8_t>& bios) {
  if (bios.size() != Sb2000Machine::BIOS_SIZE) {
    throw shared::CoreError(hw::ERROR_INVALID_ARGUMENT, "sb2000 的 BIOS 必须是 " + std::to_string(Sb2000Machine::BIOS_SIZE) + " 字节，收到 " + std::to_string(bios.size()) + " 字节");
  }
}

}  // namespace

Sb2000Machine::Sb2000Machine(std::vector<std::uint8_t> bios, hw::PrinterOutput* printerOutput, hw::FloppyActivity* floppyActivity)
    : cartridgeIrq_(irq_),
      bios_(std::move(bios)),
      ram_(INTERNAL_RAM_SIZE),
      ramDevice_(ram_),
      externalRam_(EXTERNAL_RAM_SIZE),
      externalRamDevice_(externalRam_),
      program_(paging_, bios_),
      workWindow_(sysinit_, ram_),
      printer_(printerOutput != nullptr ? std::make_unique<logic::hardware::PrinterEscp>(scheduler_, *printerOutput, PRINTER_TIMEOUT_TICKS) : nullptr),
      parallel_(printer_ != nullptr ? static_cast<hw::ParallelPort&>(*printer_) : static_cast<hw::ParallelPort&>(nullParallel_)),
      irqController_(irq_),
      fdc_(scheduler_, irqController_, logic::hardware::IrqUm6576::SOURCE_FLOPPY, FDC_STEP_TICKS, FDC_POLL_STEPS),
      superIo_(video_, fdc_),
      timer_(scheduler_, irqController_, CPU_TICKS_PER_CYCLE, LINE_TICKS),
      expio_(parallel_),
      dma_(cpuBus_, scheduler_, rdy_, hw::ClockDomain{CPU_TICKS_PER_CYCLE}, *this),
      keyboardDevice_(scheduler_, MILLISECOND_TICKS),
      mouseDevice_(scheduler_, MILLISECOND_TICKS),
      keyboardPort_(keyboardDevice_, irqController_, scheduler_, MILLISECOND_TICKS),
      mousePort_(mouseDevice_, irqController_),
      printerPort_(parallel_),
      pad_(parallel_),
      psg_(scheduler_, hw::ClockDomain{CPU_TICKS_PER_CYCLE}, MASTER_CLOCK_HZ / static_cast<double>(CPU_TICKS_PER_CYCLE)),
      ppu_(scheduler_, videoBus_, nmi_, hw::ClockDomain{PPU_TICKS_PER_DOT}, 0),
      cpu_(cpuBus_, scheduler_, hw::ClockDomain{CPU_TICKS_PER_CYCLE}, irq_, nmi_, rdy_),
      debug_(cpuBus_, cpu_) {
  requireBios(bios_);

  cpuBus_.mount(0x0000, 0x1FFF, ramDevice_);
  cpuBus_.mount(0x2000, 0x3FFF, ppu_);
  cpuBus_.mount(0x4000, 0x4013, psg_);
  cpuBus_.mount(0x4014, 0x4014, dma_);
  cpuBus_.mount(0x4015, 0x4015, psg_);
  cpuBus_.mount(0x4016, 0x4017, pad_);
  cpuBus_.mount(0x4020, 0x4022, keyboardPort_);
  cpuBus_.mount(0x4023, 0x4024, mousePort_);
  cpuBus_.mount(0x4025, 0x4025, printerPort_);
  cpuBus_.mount(0x4026, 0x4026, expio_);
  cpuBus_.mount(0x4027, 0x4027, dac_);
  cpuBus_.mount(0x4031, 0x4031, sysinit_);
  cpuBus_.mount(0x4032, 0x4033, irqController_);
  cpuBus_.mount(0x4034, 0x4036, timer_);
  cpuBus_.mount(0x4040, 0x4047, paging_);
  cpuBus_.mount(0x4048, 0x404F, dma_);
  cpuBus_.mount(0x4080, 0x4081, rtc_);
  cpuBus_.mount(0x4100, 0x43FF, superIo_);
  cpuBus_.mount(0x4400, 0x5FFF, workWindow_);
  cpuBus_.mount(0x6000, 0x7FFF, externalRamDevice_);
  cpuBus_.mount(0x8000, 0xFFFF, program_);

  videoBus_.mount(0x0000, 0xFFFF, video_);

  for (unsigned index = 0; index < joypads_.size(); ++index) pad_.port(index).plug(&joypads_[index]);
  fdc_.attachDrive(0, &floppy_);
  if (floppyActivity != nullptr) floppy_.onMotorChanged = [floppyActivity](bool on) { floppyActivity->motorChanged(0, on); };
  dac_.onLevel = [this](std::uint8_t level) { psg_.setDacLevel(level); };
  dma_.timingControlOn = [this] { return (keyboardPort_.timingControl() & 0x80) != 0; };
  expio_.onMouseReset = [this] { mouseDevice_.pulseControl(); };
  expio_.onSpeechReset = [this] { superIo_.resetSpeech(); };
}

void Sb2000Machine::resetDevices(bool power) {
  sysinit_.reset(power);
  irqController_.reset(power);
  timer_.reset(power);
  rtc_.reset(power);
  paging_.reset(power);
  expio_.reset(power);
  dac_.reset(power);
  dma_.reset(power);
  fdc_.reset(power);
  keyboardDevice_.reset(power);
  mouseDevice_.reset(power);
  keyboardPort_.reset(power);
  mousePort_.reset(power);
  printerPort_.reset(power);
  pad_.reset(power);
  psg_.reset(power);
  ppu_.reset(power);
  parallel_.reset();
  superIo_.resetSpeech();
}

void Sb2000Machine::powerOn() {
  // 时钟先归零并丢弃旧事件，各设备复位时再登记自己的事件；CPU 最后复位，复位序列会推进时钟。
  scheduler_.reset();
  ram_.fill(0);
  externalRam_.fill(0);
  program_.clear();
  video_.clear();
  cartridgeIrq_.set(false);
  resetDevices(true);
  poweredOn_ = true;
  mouseRemainderX_ = 0;
  mouseRemainderY_ = 0;
  debug_.clearStopState();
  cpu_.reset(true);
  debug_.discardPendingHit();
}

void Sb2000Machine::powerOff() { poweredOn_ = false; }

void Sb2000Machine::softReset() {
  if (!poweredOn_) return;
  resetDevices(false);
  cpu_.reset(false);
  debug_.clearStopState();
}

void Sb2000Machine::run(hw::MasterTime ticks) {
  if (!poweredOn_ || ticks <= 0) return;
  const hw::MasterTime cycles = (ticks + CPU_TICKS_PER_CYCLE - 1) / CPU_TICKS_PER_CYCLE;
  cpu_.run(static_cast<std::uint32_t>(cycles));
  psg_.catchUp(scheduler_.now());
}

void Sb2000Machine::runFrame() {
  if (!poweredOn_) return;
  const std::uint64_t target = ppu_.frameCount() + 1;
  while (ppu_.frameCount() < target) {
    if (debug_.stopAtExecuteBreakpoint()) return;
    debug_.beforeInstruction();
    cpu_.run(1);
    if (debug_.hasPendingHit()) return;
  }
  psg_.catchUp(scheduler_.now());
}

void Sb2000Machine::takeAudio(std::vector<float>& samples) {
  if (poweredOn_) psg_.catchUp(scheduler_.now());
  psg_.takeAudio(samples);
}

void Sb2000Machine::setButtons(unsigned port, std::uint8_t buttons) {
  if (port >= joypads_.size()) throw shared::CoreError(hw::ERROR_PORT_NOT_FOUND, "手柄口只有 2 个，编号为 0 与 1");
  joypads_[port].setButtons(buttons);
}

// ---------------------------------------------------------------- 输入

void Sb2000Machine::input(const hw::InputEvent& event) {
  if (const auto* key = std::get_if<hw::KeyInput>(&event)) {
    // 键盘只接在学习机上：物理键位换成扫描码，不认识的键丢弃。
    if (const auto code = logic::hardware::KeyboardSb2000::scanCodeFor(key->code)) keyboardDevice_.setKey(*code, key->down);
  } else if (const auto* mouse = std::get_if<hw::MouseInput>(&event)) {
    applyMouse(*mouse);
  } else if (const auto* gamepad = std::get_if<hw::GamepadInput>(&event)) {
    applyGamepad(*gamepad);
  }
}

void Sb2000Machine::applyMouse(const hw::MouseInput& mouse) {
  if (mouse.kind == hw::MouseKind::Wheel) return;
  if (mouse.kind == hw::MouseKind::Leave) {
    mouseRemainderX_ = 0;
    mouseRemainderY_ = 0;
    mouseDevice_.move(0, 0, 0);
    return;
  }
  // 位移的单位是标准帧的像素；凑不够 1 个单位的小数留到下一次。
  mouseRemainderX_ += mouse.dx;
  mouseRemainderY_ += mouse.dy;
  const double wholeX = std::trunc(mouseRemainderX_);
  const double wholeY = std::trunc(mouseRemainderY_);
  mouseRemainderX_ -= wholeX;
  mouseRemainderY_ -= wholeY;
  // 左键 1、右键 2；中键与串口鼠标无关。
  const std::uint8_t buttons = mouse.buttons & (logic::hardware::MouseSb2000::BUTTON_LEFT | logic::hardware::MouseSb2000::BUTTON_RIGHT);
  mouseDevice_.move(static_cast<int>(wholeX), static_cast<int>(wholeY), buttons);
}

void Sb2000Machine::applyGamepad(const hw::GamepadInput& gamepad) {
  if (gamepad.index >= joypads_.size()) return;
  std::uint8_t buttons = 0;
  if (gamepad.connected) {
    const auto pressed = [&](std::size_t index) { return index < gamepad.buttons.size() && gamepad.buttons[index]; };
    const auto axis = [&](std::size_t index) { return index < gamepad.axes.size() ? gamepad.axes[index] : 0.0; };
    // 浏览器标准映射：下方的键当作 B，右侧的键当作 A。
    if (pressed(GAMEPAD_EAST)) buttons |= hw::pad_button::A;
    if (pressed(GAMEPAD_SOUTH)) buttons |= hw::pad_button::B;
    if (pressed(GAMEPAD_SELECT)) buttons |= hw::pad_button::SELECT;
    if (pressed(GAMEPAD_START)) buttons |= hw::pad_button::START;
    if (pressed(GAMEPAD_UP) || axis(1) < -AXIS_THRESHOLD) buttons |= hw::pad_button::UP;
    if (pressed(GAMEPAD_DOWN) || axis(1) > AXIS_THRESHOLD) buttons |= hw::pad_button::DOWN;
    if (pressed(GAMEPAD_LEFT) || axis(0) < -AXIS_THRESHOLD) buttons |= hw::pad_button::LEFT;
    if (pressed(GAMEPAD_RIGHT) || axis(0) > AXIS_THRESHOLD) buttons |= hw::pad_button::RIGHT;
  }
  joypads_[gamepad.index].setButtons(buttons);
}

// ---------------------------------------------------------------- 通用 DMA 的存储

std::uint8_t Sb2000Machine::readSource(bool extendedRam, std::uint32_t offset) {
  const std::vector<std::uint8_t>& source = extendedRam ? program_.extendedRam() : bios_;
  return offset < source.size() ? source[offset] : std::uint8_t{0};
}

void Sb2000Machine::writeWork(std::uint16_t address, std::uint8_t value) {
  // 真机实测：目标在程序窗口（$8000 以上）时写不进去。
  if (address < 0x8000U) cpuBus_.write(address, value);
}

// ---------------------------------------------------------------- 调试接口

std::vector<hw::MemorySpace> Sb2000Machine::memorySpaces() const {
  return {
      {"cpu", "CPU 总线 $0000-$FFFF：读不产生副作用，寄存器类地址读出 0；写经真实的总线，与 CPU 写一样有副作用", CPU_SPACE_SIZE, true},
      {"ppu", "显存总线 $0000-$FFFF：$0000-$7FFF 字符 RAM，$8000-$FFFF 扩展显存的 32K 窗口（页由 $4300 选择）；读不产生副作用", VIDEO_SPACE_SIZE, true},
      {"board.ram", "内部 RAM（2K，$0000-$1FFF 每 2K 镜像一次）", INTERNAL_RAM_SIZE, true},
      {"board.externalRam", "外部 RAM（8K，$6000-$7FFF）", EXTERNAL_RAM_SIZE, true},
      {"board.bios", "BIOS ROM（512KB，只读，不含 iNES 文件头）", Sb2000Machine::BIOS_SIZE, false},
      {"board.extendedRam", "扩展 RAM（512KB，程序窗口的页号不小于 $80 时用）", Sb2000ProgramMemory::EXTENDED_RAM_SIZE, true},
      {"board.characterRam", "字符 RAM（32KB，显存地址 $0000-$7FFF）", Sb2000VideoMemory::CHARACTER_RAM_SIZE, true},
      {"board.extendedVram", "扩展显存（512KB，每页 32KB，显存地址 $8000-$FFFF 经 $4300 选页）", Sb2000VideoMemory::EXTENDED_VRAM_SIZE, true},
      {"chip.oam", "PPU 的精灵属性存储（OAM）", OAM_SIZE, true},
      {"chip.palette", "PPU 的调色板（64 项，每项低 6 位）", PALETTE_SIZE, true},
  };
}

void Sb2000Machine::readMemory(std::string_view space, std::uint32_t address, std::span<std::uint8_t> out) {
  const auto copyFrom = [&](const std::vector<std::uint8_t>& bytes) {
    checkRange(space, bytes.size(), address, out.size());
    std::copy_n(bytes.begin() + address, out.size(), out.begin());
  };
  if (space == "cpu") {
    checkRange(space, CPU_SPACE_SIZE, address, out.size());
    for (std::size_t index = 0; index < out.size(); ++index) out[index] = cpuBus_.peek(static_cast<std::uint16_t>(address + index));
  } else if (space == "ppu") {
    checkRange(space, VIDEO_SPACE_SIZE, address, out.size());
    for (std::size_t index = 0; index < out.size(); ++index) out[index] = videoBus_.peek(static_cast<std::uint16_t>(address + index));
  } else if (space == "board.ram") {
    checkRange(space, INTERNAL_RAM_SIZE, address, out.size());
    ram_.readBlock(address, out);
  } else if (space == "board.externalRam") {
    checkRange(space, EXTERNAL_RAM_SIZE, address, out.size());
    externalRam_.readBlock(address, out);
  } else if (space == "board.bios") {
    copyFrom(bios_);
  } else if (space == "board.extendedRam") {
    copyFrom(program_.extendedRam());
  } else if (space == "board.characterRam") {
    copyFrom(video_.characterRam());
  } else if (space == "board.extendedVram") {
    copyFrom(video_.extendedVram());
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

void Sb2000Machine::writeMemory(std::string_view space, std::uint32_t address, std::span<const std::uint8_t> bytes) {
  // 调试写入不触发读写断点，也不进记录。
  const CpuDebugKit::Suspend suspend(debug_, true);
  const auto copyTo = [&](std::vector<std::uint8_t>& target) {
    checkRange(space, target.size(), address, bytes.size());
    std::copy(bytes.begin(), bytes.end(), target.begin() + address);
  };
  if (space == "cpu") {
    checkRange(space, CPU_SPACE_SIZE, address, bytes.size());
    for (std::size_t index = 0; index < bytes.size(); ++index) cpuBus_.write(static_cast<std::uint16_t>(address + index), bytes[index]);
  } else if (space == "ppu") {
    checkRange(space, VIDEO_SPACE_SIZE, address, bytes.size());
    for (std::size_t index = 0; index < bytes.size(); ++index) videoBus_.write(static_cast<std::uint16_t>(address + index), bytes[index]);
  } else if (space == "board.ram") {
    checkRange(space, INTERNAL_RAM_SIZE, address, bytes.size());
    for (std::size_t index = 0; index < bytes.size(); ++index) ram_.write(address + index, bytes[index]);
  } else if (space == "board.externalRam") {
    checkRange(space, EXTERNAL_RAM_SIZE, address, bytes.size());
    for (std::size_t index = 0; index < bytes.size(); ++index) externalRam_.write(address + index, bytes[index]);
  } else if (space == "board.bios") {
    throw shared::CoreError(hw::ERROR_SPACE_READ_ONLY, "空间 board.bios 是只读的");
  } else if (space == "board.extendedRam") {
    copyTo(program_.extendedRam());
  } else if (space == "board.characterRam") {
    copyTo(video_.characterRam());
  } else if (space == "board.extendedVram") {
    copyTo(video_.extendedVram());
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

hw::AddressTarget Sb2000Machine::resolve(std::string_view space, std::uint32_t address) const {
  if (space == "cpu") {
    checkRange(space, CPU_SPACE_SIZE, address, 1);
    if (address < 0x2000U) return {"ram", "board.ram", address & (INTERNAL_RAM_SIZE - 1), "内部 RAM，$0000-$1FFF 每 2K 镜像一次"};
    if (address < 0x2080U) return {"register", "chip.ppu", address - 0x2000U, "图形端口（$2000-$2008）与调色板（$2040-$207F）"};
    if (address < 0x4000U) return {"none", "", 0, "没有器件，读到总线上最近的值"};
    if (address <= 0x4013U || address == 0x4015U) return {"register", "chip.psg", address - 0x4000U, "PSG 寄存器"};
    if (address == 0x4014U) return {"register", "chip.dma", 0, "精灵 DMA"};
    if (address <= 0x4017U) return {"register", "chip.pad", address - 0x4016U, "手柄口"};
    if (address >= 0x4020U && address <= 0x4022U) return {"register", "chip.keyboard", address - 0x4020U, "键盘接口"};
    if (address == 0x4023U || address == 0x4024U) return {"register", "chip.mouse", address - 0x4023U, "鼠标接口"};
    if (address == 0x4025U) return {"register", "chip.printer", 0, "并口数据"};
    if (address == 0x4026U) return {"register", "chip.expio", 0, "扩展 I/O"};
    if (address == 0x4027U) return {"register", "chip.dac", 0, "DAC"};
    if (address == 0x4031U) return {"register", "chip.sysinit", 0, "初始设置"};
    if (address == 0x4032U || address == 0x4033U) return {"register", "chip.irq", address - 0x4032U, "中断屏蔽与状态"};
    if (address >= 0x4034U && address <= 0x4036U) return {"register", "chip.timer", address - 0x4034U, "定时器"};
    if (address >= 0x4040U && address <= 0x4047U) return {"register", "chip.paging", address - 0x4040U, "程序存储器分页"};
    if (address >= 0x4048U && address <= 0x404FU) return {"register", "chip.dma", address - 0x4048U + 1, "通用 DMA"};
    if (address == 0x4080U || address == 0x4081U) return {"register", "chip.rtc", address - 0x4080U, "实时时钟"};
    if (address >= 0x4100U && address < 0x4400U) return {"register", "board.superIo", address - 0x4100U, "超级 I/O（显存页 $4300、语音端口 $4302）"};
    if (address < 0x4400U) return {"none", "", 0, "没有器件，读到总线上最近的值"};
    if (address < 0x6000U) return {"none", "", 0, "6576 模式下不是 RAM，读出 $68，写入无效"};
    if (address < 0x8000U) return {"ram", "board.externalRam", address & (EXTERNAL_RAM_SIZE - 1), "外部 RAM（8K）"};
    const logic::hardware::PagingUm6576::Location location = paging_.resolve(static_cast<std::uint16_t>(address));
    if (location.ram) return {"ram", "board.extendedRam", location.offset, "程序窗口，当前对应扩展 RAM"};
    return {"rom", "board.bios", location.offset, "程序窗口，当前对应 BIOS ROM"};
  }
  if (space == "ppu") {
    checkRange(space, VIDEO_SPACE_SIZE, address, 1);
    if (address < Sb2000VideoMemory::CHARACTER_RAM_SIZE) return {"ram", "board.characterRam", address, "字符 RAM（32KB）"};
    return {"ram", "board.extendedVram", static_cast<std::size_t>(video_.bank()) * Sb2000VideoMemory::WINDOW_SIZE + (address & 0x7FFFU), "扩展显存的窗口，页由 $4300 选择"};
  }
  unknownSpace(space);
}

std::vector<std::string> Sb2000Machine::stateModules() const { return {"cpu", "ppu", "paging"}; }

std::vector<hw::StateField> Sb2000Machine::readState(std::string_view module) const {
  if (module == "cpu") return debug_.readCpuState();
  if (module == "ppu") {
    return {field("scanline", ppu_.nextLine(), "下一个要处理的扫描线（0 到 311，241 是垂直消隐的第一行）"),
            field("frame", static_cast<std::int64_t>(ppu_.frameCount()), "已完成的帧数"),
            field("vramAddress", ppu_.vramAddress(), "当前的显存地址"),
            field("control", ppu_.control(), "$2000 的值"),
            field("mask", ppu_.mask(), "$2001 的值"),
            field("status", ppu_.status(), "$2002 的状态位（位 7 垂直消隐、位 6 精灵 0 命中、位 5 精灵溢出）"),
            field("graphicMode", ppu_.reg2008(), "$2008 的值")};
  }
  if (module == "paging") {
    std::vector<hw::StateField> fields;
    for (unsigned window = 0; window < logic::hardware::PagingUm6576::WINDOW_COUNT; ++window) {
      static constexpr const char* NAMES[] = {"window0", "window1", "window2", "window3", "window4", "window5", "window6", "window7"};
      fields.push_back(field(NAMES[window], paging_.bank(window), "程序窗口的页号（$4040-$4047，小于 $80 是 BIOS ROM，不小于 $80 是扩展 RAM）"));
    }
    fields.push_back(field("videoBank", video_.bank(), "扩展显存的页（$4300 的低 4 位）"));
    return fields;
  }
  throw shared::CoreError(hw::ERROR_INVALID_STATE, "没有这个状态模块：" + std::string(module));
}

void Sb2000Machine::writeState(std::string_view module, std::span<const hw::StateField> fields) {
  if (module == "ppu" || module == "paging") throw shared::CoreError(hw::ERROR_INVALID_STATE, std::string(module) + " 的状态只读");
  if (module != "cpu") throw shared::CoreError(hw::ERROR_INVALID_STATE, "没有这个状态模块：" + std::string(module));
  debug_.writeCpuState(fields);
}

hw::StepResult Sb2000Machine::step(hw::StepUnit unit, std::uint64_t count) {
  hw::StepResult result;
  if (!poweredOn_ || count == 0) return result;
  // 单步忽略断点，但照常进记录。
  const CpuDebugKit::Suspend suspend(debug_, false);
  debug_.clearStopState();
  const std::uint64_t startCycles = cpu_.registers().cycles;
  const std::uint64_t startFrames = ppu_.frameCount();
  // 每次 run(1) 执行恰好一条指令（含随后响应的中断序列）。
  const auto instruction = [&] {
    debug_.beforeInstruction();
    cpu_.run(1);
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
      int last = ppu_.nextLine();
      for (std::uint64_t remaining = count; remaining > 0;) {
        instruction();
        if (ppu_.nextLine() != last) {
          last = ppu_.nextLine();
          --remaining;
        }
      }
      break;
    }
    case hw::StepUnit::Frame:
      while (ppu_.frameCount() - startFrames < count) instruction();
      break;
  }
  psg_.catchUp(scheduler_.now());
  result.cycles = cpu_.registers().cycles - startCycles;
  result.frames = ppu_.frameCount() - startFrames;
  return result;
}

}  // namespace ember::bootstrap
