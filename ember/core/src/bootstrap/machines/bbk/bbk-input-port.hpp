/*
 * ------------------------------------------------------------------------
 * 名称：bbk-input-port.hpp
 * 说明：步步高的 $4016-$4017：手柄口 1 的数据线、键盘矩阵与鼠标的信号线，以及 APU 的帧计数器写入。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-16
 * 备注：读 $4016：位 0 是手柄 1 的数据线，其余位为 0，同时让鼠标记一次读 $4016；
 *       读 $4017：位 5-7 为 1，位 1-4 是键盘当前半边的 4 个键，位 0 是鼠标的串行线；步步高的 $4017 不接手柄 2；
 *       写 $4016：位 0 是手柄的锁存线，整个字节同时给键盘与鼠标；写 $4017：APU 的帧计数器；
 *       手柄口、键盘、鼠标、APU 互不引用，只在主机装配里经本设备接在一起；peek 保持默认（不驱动数据线），读 $4016 会推进手柄与鼠标，不是无副作用的。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>

#include "contracts/hardware/bus-device.hpp"
#include "contracts/hardware/controller-port.hpp"
#include "logic/hardware/keyboard-bbk/keyboard-bbk.hpp"
#include "logic/hardware/mouse-bbk/mouse-bbk.hpp"

namespace ember::bootstrap {

/** 步步高的 $4016-$4017。 */
class BbkInputPort final : public contracts::hardware::BusDevice {
 public:
  /**
   * @param pad 手柄口 1。
   * @param apu APU 设备，只接收 $4017 的写入（帧计数器）。
   * @param keyboard 步步高键盘。
   * @param mouse 步步高鼠标。
   */
  BbkInputPort(contracts::hardware::ControllerPort& pad, contracts::hardware::BusDevice& apu, logic::hardware::KeyboardBbk& keyboard, logic::hardware::MouseBbk& mouse)
      : pad_(pad), apu_(apu), keyboard_(keyboard), mouse_(mouse) {}

  contracts::hardware::BusRead read(std::uint16_t address) override {
    if (address == ADDRESS_STATUS) {
      mouse_.noteStatusRead();
      return contracts::hardware::driveBus(static_cast<std::uint8_t>(pad_.clockData() & 1U));
    }
    const std::uint8_t value = static_cast<std::uint8_t>(UNUSED_HIGH_BITS | keyboard_.read() | mouse_.readLine());
    return contracts::hardware::driveBus(value);
  }

  void write(std::uint16_t address, std::uint8_t value) override {
    if (address == ADDRESS_STATUS) {
      pad_.setLatch((value & 1U) != 0);
      keyboard_.write(value);
      mouse_.write(value);
    } else {
      apu_.write(address, value);
    }
  }

 private:
  static constexpr std::uint16_t ADDRESS_STATUS = 0x4016;
  /** 读 $4017 时位 5-7 总是 1。 */
  static constexpr std::uint8_t UNUSED_HIGH_BITS = 0xE0;

  contracts::hardware::ControllerPort& pad_;
  contracts::hardware::BusDevice& apu_;
  logic::hardware::KeyboardBbk& keyboard_;
  logic::hardware::MouseBbk& mouse_;
};

}  // namespace ember::bootstrap
