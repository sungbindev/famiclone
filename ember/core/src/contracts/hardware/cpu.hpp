/*
 * ------------------------------------------------------------------------
 * 名称：cpu.hpp
 * 说明：硬件契约：CPU。复位、按周期数运行、读取寄存器状态；中断线与 RDY 线在构造时接入，总线经构造注入。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：CPU 每完成一次总线访问就推进主时钟；CPU 不直接依赖其他芯片，只经 CpuBus 访问内存。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>

namespace ember::contracts::hardware {

/** CPU 寄存器与累计周期数，供测试与调试读取。 */
struct CpuRegisters {
  std::uint16_t pc{0};
  std::uint8_t a{0};
  std::uint8_t x{0};
  std::uint8_t y{0};
  /** 栈指针，栈在 $0100-$01FF。 */
  std::uint8_t sp{0};
  /** 状态寄存器：位 5 恒为 1，位 4（B）恒为 0（B 只出现在压栈的副本里）。 */
  std::uint8_t status{0};
  /** 自上电以来的 CPU 周期数（含被 RDY 暂停的周期）。 */
  std::uint64_t cycles{0};
};

/** CPU。 */
class Cpu {
 public:
  virtual ~Cpu() = default;

  /**
   * 复位，执行一次完整的复位序列（含向量读取，共 7 个周期）。
   * @param power 为 true 表示上电复位，寄存器清零并重新开始计周期；为 false 表示软复位，寄存器与累计周期保留。
   */
  virtual void reset(bool power) = 0;

  /**
   * 至少运行指定的周期数。指令（含随后响应的中断序列）不会被中途打断，所以实际周期数可能多几个。
   * @param cycles 要运行的 CPU 周期数。
   * @return 实际运行的 CPU 周期数。
   */
  virtual std::uint32_t run(std::uint32_t cycles) = 0;

  /** 当前寄存器状态。只在指令之间读取有意义。 */
  [[nodiscard]] virtual CpuRegisters registers() const = 0;
};

}  // namespace ember::contracts::hardware
