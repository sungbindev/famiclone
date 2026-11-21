/*
 * ------------------------------------------------------------------------
 * 名称：cpu-debug-kit.hpp
 * 说明：以 6502 为 CPU 的主机共用的调试部件：断点、记录、代码数据标记、反汇编、CPU 状态的读写，以及内存空间访问的公共检查。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：主机型号（fc、sb2000、bbg）各自拥有一个，只负责把自己的内存空间与芯片状态接到调试接口上；
 *       断点与记录靠 CPU 总线上的探针与每条指令之前的钩子生效，规则见 contracts/hardware/machine-debugger.hpp。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "contracts/hardware/machine-debugger.hpp"
#include "logic/hardware/bus/mapped-bus.hpp"
#include "logic/hardware/cpu-6502/cpu-6502.hpp"
#include "logic/hardware/debug/breakpoint-table.hpp"
#include "logic/hardware/debug/code-data-log.hpp"
#include "logic/hardware/debug/cpu-disassembler.hpp"
#include "logic/hardware/debug/probe-hub.hpp"
#include "logic/hardware/debug/recorder-table.hpp"

namespace ember::bootstrap {

namespace debugging {

/** 检查访问范围，越界时抛出 hardware.spaceOutOfRange。 */
void checkRange(std::string_view space, std::size_t size, std::uint32_t address, std::size_t length);

/** 抛出 hardware.unknownSpace。 */
[[noreturn]] void unknownSpace(std::string_view space);

/** 组一项状态。 */
contracts::hardware::StateField field(const char* name, std::int64_t value, const char* description);

}  // namespace debugging

/** 以 6502 为 CPU 的主机共用的调试部件。 */
class CpuDebugKit final : private contracts::hardware::InterruptObserver {
 public:
  /**
   * @param bus CPU 总线，探针装在它上面。
   * @param cpu CPU，本对象登记为它的中断观察者。
   */
  CpuDebugKit(logic::hardware::CpuAddressBus& bus, logic::hardware::Cpu6502& cpu);

  CpuDebugKit(const CpuDebugKit&) = delete;
  CpuDebugKit& operator=(const CpuDebugKit&) = delete;

  // ------------------------------------------------ 运行时的钩子

  /**
   * 运行一条指令之前调用：这个地址上有执行断点就返回 true，主机应该停下（指令没有执行）；
   * 刚命中的地址在下一次运行时越过一次，不重复命中。
   */
  bool stopAtExecuteBreakpoint();

  /** 即将执行一条指令：通知记录表与代码数据标记。 */
  void beforeInstruction();

  /** 已经有断点命中，等着主机取走。 */
  [[nodiscard]] bool hasPendingHit() const { return breakpoints_.hasPending(); }

  /** 上电、复位、改 CPU 寄存器、单步之前：清掉“要越过的断点地址”与未取走的命中。 */
  void clearStopState();

  /** 上电与复位时：清掉未取走的命中。 */
  void discardPendingHit() { breakpoints_.discardPending(); }

  /** 不带副作用地读 CPU 总线。 */
  [[nodiscard]] std::uint8_t peek(std::uint16_t address) const { return bus_.peek(address); }

  /** 构造时暂停断点（alsoRecording 为 true 时连记录一起暂停），析构时恢复：让调试访问不触发读写断点，调试写内存也不进记录。 */
  class Suspend {
   public:
    Suspend(CpuDebugKit& kit, bool alsoRecording);
    ~Suspend();
    Suspend(const Suspend&) = delete;
    Suspend& operator=(const Suspend&) = delete;

   private:
    CpuDebugKit& kit_;
    logic::hardware::ProbeHub::Suspension before_;
  };

  // ------------------------------------------------ MachineDebugger 里与 CPU 总线相关的部分

  contracts::hardware::Breakpoint addBreakpoint(contracts::hardware::BreakpointKind kind, std::uint16_t first, std::uint16_t last);
  bool removeBreakpoint(std::uint32_t id);
  std::size_t clearBreakpoints();
  [[nodiscard]] std::vector<contracts::hardware::Breakpoint> breakpoints() const { return breakpoints_.list(); }
  std::optional<contracts::hardware::BreakpointHit> takeBreakpointHit() { return breakpoints_.take(); }

  std::uint32_t startRecorder(const contracts::hardware::RecorderSpec& spec);
  contracts::hardware::RecorderOutput stopRecorder(std::uint32_t id);

  [[nodiscard]] std::vector<contracts::hardware::DisassembledInstruction> disassemble(std::uint16_t address, std::uint32_t count);

  void startCdl(std::size_t size, contracts::hardware::CdlResolver resolver) { cdl_.start(size, std::move(resolver)); }
  void stopCdl() { cdl_.stop(); }
  [[nodiscard]] std::optional<std::vector<std::uint8_t>> cdlFlags() const;

  /** CPU 状态模块 cpu 的读。 */
  [[nodiscard]] std::vector<contracts::hardware::StateField> readCpuState() const;

  /** CPU 状态模块 cpu 的写：先检查，再整体写入。 */
  void writeCpuState(std::span<const contracts::hardware::StateField> fields);

 private:
  // InterruptObserver
  void onInterrupt(contracts::hardware::InterruptKind kind, std::uint16_t returnAddress, std::uint16_t handler) override;

  /** 有读写断点或 I/O 记录时在 CPU 总线上安装探针，没有时去掉。 */
  void refreshProbe();

  logic::hardware::CpuAddressBus& bus_;
  logic::hardware::Cpu6502& cpu_;
  logic::hardware::BreakpointTable breakpoints_;
  /** 无副作用地读 CPU 总线。 */
  logic::hardware::PeekFunction peek_;
  logic::hardware::RecorderTable recorders_;
  logic::hardware::ProbeHub probeHub_;
  logic::hardware::CodeDataLog cdl_;
  /** 刚命中执行断点的地址：下一次运行先越过它，不再重复命中。 */
  std::optional<std::uint16_t> skipExecuteAt_;
};

}  // namespace ember::bootstrap
