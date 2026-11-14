/*
 * ------------------------------------------------------------------------
 * 名称：recorder-table.hpp
 * 说明：记录表：同时进行的几项记录（指令跟踪、I/O 跟踪、中断跟踪、调用记录），内容记在内存里，结束时取走。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：只在模拟线程使用，不加锁；各主机型号共用。指令边界的通知由主机在执行每条指令之前发（onInstruction），
 *       总线访问经探针（onIo）、中断经 CPU 的中断通知（onInterrupt）。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "contracts/hardware/access-probe.hpp"
#include "contracts/hardware/cpu.hpp"
#include "contracts/hardware/machine-debugger.hpp"
#include "logic/hardware/debug/cpu-disassembler.hpp"

namespace ember::logic::hardware {

/** 记录表。 */
class RecorderTable {
 public:
  /** 同时进行的记录上限。 */
  static constexpr std::size_t MAX_RECORDERS = 8;
  /** 单项记录的行数上限。 */
  static constexpr std::uint32_t MAX_LINES = 1'000'000;

  /** @param peek 无副作用地读 CPU 总线，用来写反汇编、栈与内存段。 */
  explicit RecorderTable(PeekFunction peek);

  /**
   * 开始一项记录。
   * @return 记录编号。
   * @throws shared::CoreError hardware.recorderLimit。
   */
  std::uint32_t start(const contracts::hardware::RecorderSpec& spec);

  /** 结束一项记录并取走内容。@throws shared::CoreError hardware.noRecorder。 */
  contracts::hardware::RecorderOutput stop(std::uint32_t id);

  /** 丢弃全部记录。 */
  void clear();

  /** 要不要在每条指令之前通知（指令、调用记录与 I/O 记录都要知道当前的 PC 与周期）。 */
  [[nodiscard]] bool needsInstructionHook() const { return needsInstruction_; }
  /** 要不要总线探针。 */
  [[nodiscard]] bool recordsIo() const { return recordsIo_; }
  /** 要不要中断通知。 */
  [[nodiscard]] bool recordsIrq() const { return recordsIrq_; }

  /** 即将执行 registers.pc 处的指令。 */
  void onInstruction(const contracts::hardware::CpuRegisters& registers);

  /** CPU 响应了一次中断。 */
  void onInterrupt(contracts::hardware::InterruptKind kind, std::uint16_t returnAddress, std::uint16_t handler, std::uint64_t cycles);

  /** 一次总线访问完成。 */
  void onIo(contracts::hardware::AccessKind kind, std::uint16_t address, std::uint8_t value);

 private:
  struct Recorder {
    std::uint32_t id{0};
    contracts::hardware::RecorderSpec spec;
    contracts::hardware::RecorderOutput output;
    /** Instruction 且 unique 时：已经记过的地址。 */
    std::vector<bool> seen;
  };

  /** 行数满了返回 true，并标记少记了。 */
  static bool full(Recorder& recorder);
  [[nodiscard]] std::string instructionLine(const contracts::hardware::CpuRegisters& registers) const;
  [[nodiscard]] std::string callLine(const contracts::hardware::CpuRegisters& registers, const contracts::hardware::RecorderSpec& spec) const;
  void refreshFlags();

  PeekFunction peek_;
  std::vector<Recorder> recorders_;
  std::uint32_t nextId_{1};
  bool needsInstruction_{false};
  bool recordsIo_{false};
  bool recordsIrq_{false};
  /** 最近一条指令开始时的 PC 与周期数，I/O 记录用它说明是哪条指令访问的。 */
  std::uint16_t lastPc_{0};
  std::uint64_t lastCycles_{0};
};

}  // namespace ember::logic::hardware
