/*
 * ------------------------------------------------------------------------
 * 名称：machine.hpp
 * 说明：硬件契约：一台装配好的主机。电源、复位、运行。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：主机由 bootstrap/machines 按主机型号装配；状态快照在调试服务阶段补充。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>
#include <vector>

#include "contracts/hardware/cartridge-slot.hpp"
#include "contracts/hardware/clock.hpp"
#include "contracts/hardware/cpu.hpp"
#include "contracts/hardware/irq-output.hpp"
#include "contracts/hardware/ppu.hpp"

namespace ember::contracts::hardware {

/** 主机。 */
class Machine {
 public:
  virtual ~Machine() = default;

  /** 上电：创建或重置全部芯片与设备到上电状态，随后可以运行。 */
  virtual void powerOn() = 0;

  /** 断电：停止运行，芯片状态丢弃；卡带与介质保持不变。 */
  virtual void powerOff() = 0;

  /** 软复位：只发复位信号，各芯片按自己的复位行为处理。断电时调用无效果。 */
  virtual void softReset() = 0;

  /** 是否已上电。 */
  [[nodiscard]] virtual bool isPoweredOn() const = 0;

  /**
   * 运行指定的主时钟周期数。CPU 的指令不会被中途打断，所以实际经过的时间可能略多。
   * 断电时调用无效果。
   * @param ticks 要运行的主时钟周期数。
   */
  virtual void run(MasterTime ticks) = 0;

  /** 运行到 PPU 完成下一帧画面，CPU 的指令不会被中途打断。断电时调用无效果。 */
  virtual void runFrame() = 0;

  /** 最近一次完成的画面；没有完成过画面时宽高为 0。 */
  [[nodiscard]] virtual FrameBuffer frame() const = 0;

  /** 声音的采样率（赫兹），单声道。 */
  [[nodiscard]] virtual unsigned audioSampleRate() const = 0;

  /**
   * 取走自上次以来生成的声音采样（单声道，范围 -1 到 1）；先清空 samples 再放入。
   * 采样追赶到主机当前的时间；断电时没有新采样。没有人取走时主机只保留最近一小段。
   */
  virtual void takeAudio(std::vector<float>& samples) = 0;

  /**
   * 设置手柄口上标准手柄的按键。
   * @param port 手柄口编号，从 0 开始；没有这个口时报错。
   * @param buttons 按键掩码，位见 button-input.hpp。
   */
  virtual void setButtons(unsigned port, std::uint8_t buttons) = 0;

  /** CPU 当前的寄存器，供测试与调试读取；只在指令之间读取有意义。 */
  [[nodiscard]] virtual CpuRegisters cpuRegisters() const = 0;

  /** 主时钟。 */
  [[nodiscard]] virtual const Clock& clock() const = 0;

  /** 卡槽。 */
  virtual CartridgeSlot& cartridgeSlot() = 0;

  /** 卡槽的 IRQ 引脚：卡带经它向 CPU 发出中断；创建卡带时接入。 */
  virtual IrqOutput& cartridgeIrq() = 0;
};

}  // namespace ember::contracts::hardware
