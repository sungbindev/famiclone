/*
 * ------------------------------------------------------------------------
 * 名称：cpu-6502.hpp
 * 说明：经典 FC 的 6502 CPU 核：全部官方指令与 nestest 用到的非官方指令，周期精确，无十进制模式。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：每个总线周期（含空读、假写）都经 CpuBus 完成并推进主时钟，总线访问发生在周期的中点；
 *       NMI 与 IRQ 在每个周期结束时采样，用倒数第二个周期的结果决定指令结束后是否响应；RDY 有效时在读周期暂停。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>

#include "contracts/hardware/clock.hpp"
#include "contracts/hardware/cpu-bus.hpp"
#include "contracts/hardware/cpu.hpp"
#include "contracts/hardware/irq-line.hpp"
#include "contracts/hardware/rdy-line.hpp"
#include "logic/hardware/cpu-6502/opcode-table.hpp"

namespace ember::logic::hardware {

/** 6502 CPU 核。 */
class Cpu6502 final : public contracts::hardware::Cpu {
 public:
  /**
   * @param bus CPU 总线，必须活到本对象销毁。
   * @param clock 主时钟，CPU 每个周期推进它。
   * @param domain CPU 的时钟域，每个 CPU 周期占的主时钟周期数必须大于 0。
   * @param irq IRQ 线。
   * @param nmi NMI 线。
   * @param rdy RDY 线。
   */
  Cpu6502(contracts::hardware::CpuBus& bus, contracts::hardware::Clock& clock, contracts::hardware::ClockDomain domain, contracts::hardware::IrqLine& irq, contracts::hardware::IrqLine& nmi, contracts::hardware::RdyLine& rdy);

  void reset(bool power) override;

  std::uint32_t run(std::uint32_t cycles) override;

  [[nodiscard]] contracts::hardware::CpuRegisters registers() const override;

 private:
  /** 内存操作的访问方式：决定带索引的寻址是否一定做一次修正前的空读。 */
  enum class Access : std::uint8_t { Read, Write, Modify };

  // 总线周期
  std::uint8_t readCycle(std::uint16_t address);
  void writeCycle(std::uint16_t address, std::uint8_t value);
  void stallWhileHalted();
  void finishCycle();
  std::uint8_t fetch();
  void push(std::uint8_t value);
  std::uint8_t pull();

  // 指令与中断
  void executeInstruction();
  void interrupt(bool software);
  std::uint16_t effectiveAddress(Mode mode, Access access);
  std::uint16_t indexedAddress(std::uint16_t base, std::uint8_t index, Access access);
  void executeImplied(Op op);
  void executeRead(Op op, std::uint8_t value);
  std::uint8_t storeValue(Op op) const;
  std::uint8_t modifyValue(Op op, std::uint8_t value);
  void jsr();
  void rts();
  void rti();
  void jmp(Mode mode);
  void branch(bool taken);

  // 标志与运算
  [[nodiscard]] bool flag(std::uint8_t mask) const { return (status_ & mask) != 0; }
  void setFlag(std::uint8_t mask, bool on);
  void setZeroNegative(std::uint8_t value);
  void addWithCarry(std::uint8_t value);
  void compare(std::uint8_t reg, std::uint8_t value);
  std::uint8_t shiftLeft(std::uint8_t value);
  std::uint8_t shiftRight(std::uint8_t value);
  std::uint8_t rotateLeft(std::uint8_t value);
  std::uint8_t rotateRight(std::uint8_t value);

  contracts::hardware::CpuBus& bus_;
  contracts::hardware::Clock& clock_;
  contracts::hardware::ClockDomain domain_;
  contracts::hardware::IrqLine& irq_;
  contracts::hardware::IrqLine& nmi_;
  contracts::hardware::RdyLine& rdy_;

  std::uint16_t pc_{0};
  std::uint8_t a_{0};
  std::uint8_t x_{0};
  std::uint8_t y_{0};
  std::uint8_t sp_{0};
  /** 状态寄存器，只保存 C、Z、I、D、V、N 六位。 */
  std::uint8_t status_{0};
  std::uint64_t cycles_{0};

  // 中断采样：每个周期结束时更新，“prev”是上一个周期结束时的值。
  bool nmiLevel_{false};
  bool needNmi_{false};
  bool prevNeedNmi_{false};
  bool runIrq_{false};
  bool prevRunIrq_{false};
};

}  // namespace ember::logic::hardware
