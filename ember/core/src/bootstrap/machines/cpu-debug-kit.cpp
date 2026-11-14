/*
 * ------------------------------------------------------------------------
 * 名称：cpu-debug-kit.cpp
 * 说明：以 6502 为 CPU 的主机共用的调试部件的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * ------------------------------------------------------------------------
 */

#include "bootstrap/machines/cpu-debug-kit.hpp"

#include <cstdio>
#include <string>
#include <utility>

#include "contracts/hardware/hardware-error.hpp"
#include "shared/errors/core-error.hpp"

namespace ember::bootstrap {

namespace hw = contracts::hardware;

namespace debugging {

void checkRange(std::string_view space, std::size_t size, std::uint32_t address, std::size_t length) {
  if (address > size || length > size - address) {
    throw shared::CoreError(hw::ERROR_SPACE_OUT_OF_RANGE, "空间 " + std::string(space) + " 的大小是 " + std::to_string(size) + " 字节，访问 " + std::to_string(address) + " 起的 " + std::to_string(length) + " 字节超出范围");
  }
}

void unknownSpace(std::string_view space) { throw shared::CoreError(hw::ERROR_UNKNOWN_SPACE, "没有这个内存空间：" + std::string(space)); }

hw::StateField field(const char* name, std::int64_t value, const char* description) { return hw::StateField{name, value, description}; }

}  // namespace debugging

using debugging::field;

CpuDebugKit::CpuDebugKit(logic::hardware::CpuAddressBus& bus, logic::hardware::Cpu6502& cpu)
    : bus_(bus),
      cpu_(cpu),
      peek_([this](std::uint16_t address) { return bus_.peek(address); }),
      recorders_(peek_),
      probeHub_(breakpoints_, recorders_) {
  cpu_.setInterruptObserver(this);
}

bool CpuDebugKit::stopAtExecuteBreakpoint() {
  if (!breakpoints_.hasExecute()) return false;
  const std::uint16_t pc = cpu_.registers().pc;
  const bool skip = skipExecuteAt_ == pc;
  skipExecuteAt_.reset();
  if (skip || !breakpoints_.hitsExecute(pc)) return false;
  skipExecuteAt_ = pc;
  return true;
}

void CpuDebugKit::beforeInstruction() {
  const bool recording = recorders_.needsInstructionHook();
  if (!recording && !cdl_.active()) return;
  const hw::CpuRegisters registers = cpu_.registers();
  if (recording) recorders_.onInstruction(registers);
  if (cdl_.active()) cdl_.onInstruction(registers, peek_);
}

void CpuDebugKit::clearStopState() {
  skipExecuteAt_.reset();
  breakpoints_.discardPending();
}

CpuDebugKit::Suspend::Suspend(CpuDebugKit& kit, bool alsoRecording) : kit_(kit), before_(kit.probeHub_.suspend(true, alsoRecording)) {}

CpuDebugKit::Suspend::~Suspend() { kit_.probeHub_.restore(before_); }

void CpuDebugKit::refreshProbe() { bus_.setProbe(probeHub_.needed() ? &probeHub_ : nullptr); }

hw::Breakpoint CpuDebugKit::addBreakpoint(hw::BreakpointKind kind, std::uint16_t first, std::uint16_t last) {
  const hw::Breakpoint added = breakpoints_.add(kind, first, last);
  refreshProbe();
  return added;
}

bool CpuDebugKit::removeBreakpoint(std::uint32_t id) {
  const bool removed = breakpoints_.remove(id);
  refreshProbe();
  return removed;
}

std::size_t CpuDebugKit::clearBreakpoints() {
  const std::size_t removed = breakpoints_.clear();
  refreshProbe();
  return removed;
}

std::uint32_t CpuDebugKit::startRecorder(const hw::RecorderSpec& spec) {
  const std::uint32_t id = recorders_.start(spec);
  refreshProbe();
  return id;
}

hw::RecorderOutput CpuDebugKit::stopRecorder(std::uint32_t id) {
  hw::RecorderOutput output = recorders_.stop(id);
  refreshProbe();
  return output;
}

std::vector<hw::DisassembledInstruction> CpuDebugKit::disassemble(std::uint16_t address, std::uint32_t count) {
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

std::optional<std::vector<std::uint8_t>> CpuDebugKit::cdlFlags() const {
  if (!cdl_.exists()) return std::nullopt;
  return cdl_.flags();
}

void CpuDebugKit::onInterrupt(hw::InterruptKind kind, std::uint16_t returnAddress, std::uint16_t handler) {
  if (recorders_.recordsIrq()) recorders_.onInterrupt(kind, returnAddress, handler, cpu_.registers().cycles);
}

std::vector<hw::StateField> CpuDebugKit::readCpuState() const {
  const hw::CpuRegisters registers = cpu_.registers();
  return {field("pc", registers.pc, "程序计数器，指向下一条要执行的指令"),
          field("a", registers.a, "累加器"),
          field("x", registers.x, "X 变址寄存器"),
          field("y", registers.y, "Y 变址寄存器"),
          field("sp", registers.sp, "栈指针，栈在 $0100-$01FF"),
          field("status", registers.status, "状态寄存器（N V - - D I Z C，位 5 恒为 1，位 4 恒为 0）"),
          field("cycles", static_cast<std::int64_t>(registers.cycles), "自上电以来的 CPU 周期数（只读）")};
}

void CpuDebugKit::writeCpuState(std::span<const hw::StateField> fields) {
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

}  // namespace ember::bootstrap
