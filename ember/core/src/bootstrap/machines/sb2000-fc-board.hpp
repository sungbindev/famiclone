/*
 * ------------------------------------------------------------------------
 * 名称：sb2000-fc-board.hpp
 * 说明：主机型号 sb2000 在 FC 兼容模式下的主板逻辑：MMC3 式的程序与图案映射、扫描线 IRQ 计数器、FC 模式的程序窗口与显存总线，以及按模式转发的总线设备。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：这是 sb2000 主板自己的逻辑，只由 sb2000.cpp 创建并挂到总线上，不是 6576 芯片的模块，其他学习机机型没有；
 *       FC 兼容模式与 MMC3 的行为没有在真机上实测过，见 docs/02 第 6.8 节；
 *       窗口：扩展 RAM 与扩展显存都按 128K 分成 4 个窗口，$5006 选窗口，MMC3 的寄存器只能在窗口内选页；
 *       IRQ：与真 MMC3 一样数图案地址线 A12 的上升沿，A12 先低过一定数量的显存访问才算一次，用来滤掉每个图块内取指令与属性造成的短暂低电平；
 *       FC 模式下图案区只读（写入被丢弃），名称表在字符 RAM 的 $2000-$27FF，按 $A000 的镜像位连接。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "bootstrap/machines/sb2000-board.hpp"
#include "contracts/hardware/bus-device.hpp"
#include "contracts/hardware/irq-line.hpp"

namespace ember::bootstrap {

/** MMC3 式的映射寄存器与扫描线 IRQ 计数器（不含存储）。 */
class Mmc3Mapper {
 public:
  /** 一个窗口的大小（字节）：128K，程序 16 页（每页 8K）或图案 128 页（每页 1K）。 */
  static constexpr std::size_t WINDOW_SIZE = 128 * 1024;
  /** A12 要先低过这么多次显存访问，下一次上升沿才计数。 */
  static constexpr unsigned A12_LOW_ACCESSES = 4;

  /** @param irq IRQ 线，本对象在其上登记一个来源。 */
  explicit Mmc3Mapper(contracts::hardware::IrqLine& irq) : irq_(irq), source_(irq.addSource()) {}

  /** 上电：全部寄存器清零，窗口 0，垂直镜像，IRQ 关闭。 */
  void reset() {
    selector_ = 0;
    registers_.fill(0);
    window_ = 0;
    horizontalMirroring_ = false;
    resetIrq();
  }

  /** 清 IRQ 计数器与允许位，撤销 IRQ（进入 FC 模式时）。 */
  void resetIrq() {
    latch_ = 0;
    counter_ = 0;
    reload_ = false;
    enabled_ = false;
    a12High_ = false;
    lowAccesses_ = 0;
    irq_.set(source_, false);
  }

  /** 写 $5006：选 128K 窗口，同时关闭并撤销 IRQ。 */
  void selectWindow(std::uint8_t value) {
    window_ = static_cast<unsigned>(((value >> 2) & 2U) | ((value >> 1) & 1U));
    enabled_ = false;
    irq_.set(source_, false);
  }

  /** FC 模式下对 $8000-$FFFF 的写入：按地址的位 14-13 与位 0 分到各 MMC3 寄存器。 */
  void writeRegister(std::uint16_t address, std::uint8_t value) {
    const bool odd = (address & 1U) != 0;
    switch ((address >> 13) & 3U) {
      case 0:
        if (odd) registers_[selector_ & 7U] = value;
        else selector_ = value;
        break;
      case 1:
        if (!odd) horizontalMirroring_ = (value & 1U) != 0;
        break;
      case 2:
        if (odd) {
          counter_ = 0;
          reload_ = true;
        } else {
          latch_ = value;
        }
        break;
      default:
        enabled_ = odd;
        if (!odd) irq_.set(source_, false);
        break;
    }
  }

  /** CPU 地址（不小于 $8000）在扩展 RAM 里的偏移。 */
  [[nodiscard]] std::size_t programOffset(std::uint16_t address) const {
    const bool swapped = (selector_ & 0x40U) != 0;
    unsigned page = 0;
    switch ((address >> 13) & 3U) {
      case 0:
        page = swapped ? SECOND_LAST_PAGE : (registers_[6] & 0x0FU);
        break;
      case 1:
        page = registers_[7] & 0x0FU;
        break;
      case 2:
        page = swapped ? (registers_[6] & 0x0FU) : SECOND_LAST_PAGE;
        break;
      default:
        page = LAST_PAGE;
        break;
    }
    return window_ * WINDOW_SIZE + page * PROGRAM_PAGE_SIZE + (address & (PROGRAM_PAGE_SIZE - 1U));
  }

  /** 图案地址（$0000-$1FFF）在扩展显存里的偏移。 */
  [[nodiscard]] std::size_t characterOffset(std::uint16_t address) const {
    const unsigned slot = (address >> 10) & 7U;
    // 选择位 7 把两个半区对调：先换算成不反转时的槽位。
    const unsigned index = (selector_ & 0x80U) != 0 ? (slot ^ 4U) : slot;
    unsigned kilobyte = 0;
    if (index < 4) kilobyte = (registers_[index < 2 ? 0 : 1] & 0x7EU) + (index & 1U);
    else kilobyte = registers_[index - 2] & 0x7FU;
    return window_ * WINDOW_SIZE + kilobyte * CHARACTER_PAGE_SIZE + (address & (CHARACTER_PAGE_SIZE - 1U));
  }

  /** 名称表地址（$2000-$3EFF，$3000 起镜像 $2000 起）在字符 RAM 里的偏移。 */
  [[nodiscard]] std::size_t nametableOffset(std::uint16_t address) const {
    const unsigned masked = address & 0x2FFFU;
    const unsigned page = horizontalMirroring_ ? ((masked >> 11) & 1U) : ((masked >> 10) & 1U);
    return NAMETABLE_BASE + page * CHARACTER_PAGE_SIZE + (masked & (CHARACTER_PAGE_SIZE - 1U));
  }

  /** PPU 对显存总线的一次访问：A12 先低够次数再变高算一次时钟。 */
  void onVideoAddress(std::uint16_t address) {
    if ((address & 0x1000U) != 0) {
      if (!a12High_ && lowAccesses_ >= A12_LOW_ACCESSES) clockCounter();
      a12High_ = true;
      lowAccesses_ = 0;
    } else {
      a12High_ = false;
      if (lowAccesses_ < A12_LOW_ACCESSES) ++lowAccesses_;
    }
  }

  // 测试与调试用
  [[nodiscard]] unsigned window() const { return window_; }
  [[nodiscard]] std::uint8_t selector() const { return selector_; }
  [[nodiscard]] std::uint8_t registerValue(unsigned index) const { return registers_.at(index); }
  [[nodiscard]] bool horizontalMirroring() const { return horizontalMirroring_; }
  [[nodiscard]] std::uint8_t irqLatch() const { return latch_; }
  [[nodiscard]] std::uint8_t irqCounter() const { return counter_; }
  [[nodiscard]] bool irqEnabled() const { return enabled_; }

 private:
  static constexpr std::size_t PROGRAM_PAGE_SIZE = 8 * 1024;
  static constexpr std::size_t CHARACTER_PAGE_SIZE = 1024;
  static constexpr unsigned SECOND_LAST_PAGE = 14;
  static constexpr unsigned LAST_PAGE = 15;
  /** 名称表在字符 RAM 里的起点。 */
  static constexpr std::size_t NAMETABLE_BASE = 0x2000;

  /** 一次 A12 上升沿：计数器为 0 或要求重载时装入锁存值，否则减 1；为 0 且允许时拉起 IRQ。 */
  void clockCounter() {
    if (counter_ == 0 || reload_) {
      counter_ = latch_;
      reload_ = false;
    } else {
      --counter_;
    }
    if (counter_ == 0 && enabled_) irq_.set(source_, true);
  }

  contracts::hardware::IrqLine& irq_;
  contracts::hardware::SignalSource source_;
  std::uint8_t selector_{0};
  std::array<std::uint8_t, 8> registers_{};
  unsigned window_{0};
  bool horizontalMirroring_{false};
  std::uint8_t latch_{0};
  std::uint8_t counter_{0};
  bool reload_{false};
  bool enabled_{false};
  bool a12High_{false};
  unsigned lowAccesses_{0};
};

/** FC 模式的程序窗口 $8000-$FFFF：读来自扩展 RAM 的 MMC3 页，写是 MMC3 寄存器。 */
class Sb2000FcProgram final : public contracts::hardware::BusDevice {
 public:
  Sb2000FcProgram(const std::vector<std::uint8_t>& extendedRam, Mmc3Mapper& mapper) : extendedRam_(extendedRam), mapper_(mapper) {}

  contracts::hardware::BusRead read(std::uint16_t address) override { return contracts::hardware::driveBus(extendedRam_[mapper_.programOffset(address)]); }

  contracts::hardware::BusRead peek(std::uint16_t address) override { return read(address); }

  void write(std::uint16_t address, std::uint8_t value) override { mapper_.writeRegister(address, value); }

 private:
  const std::vector<std::uint8_t>& extendedRam_;
  Mmc3Mapper& mapper_;
};

/** FC 模式的显存总线：图案区读自扩展显存的 MMC3 页（只读），名称表在字符 RAM；每次访问都让映射器观察 A12。 */
class Sb2000FcVideo final : public contracts::hardware::BusDevice {
 public:
  Sb2000FcVideo(Sb2000VideoMemory& memory, Mmc3Mapper& mapper) : memory_(memory), mapper_(mapper) {}

  contracts::hardware::BusRead read(std::uint16_t address) override {
    const std::uint16_t masked = address & 0x3FFFU;
    mapper_.onVideoAddress(masked);
    return contracts::hardware::driveBus(byteAt(masked));
  }

  contracts::hardware::BusRead peek(std::uint16_t address) override { return contracts::hardware::driveBus(byteAt(address & 0x3FFFU)); }

  void write(std::uint16_t address, std::uint8_t value) override {
    const std::uint16_t masked = address & 0x3FFFU;
    mapper_.onVideoAddress(masked);
    // 图案区是只读的（真机实测：写入后读回 $00）。
    if (masked >= 0x2000U) memory_.characterRam()[mapper_.nametableOffset(masked)] = value;
  }

 private:
  [[nodiscard]] std::uint8_t byteAt(std::uint16_t masked) const {
    if (masked < 0x2000U) return memory_.extendedVram()[mapper_.characterOffset(masked)];
    return memory_.characterRam()[mapper_.nametableOffset(masked)];
  }

  Sb2000VideoMemory& memory_;
  Mmc3Mapper& mapper_;
};

/** 按模式转发的总线设备：6576 模式交给 native，FC 兼容模式交给 fc；读、调试读与写都一样。 */
class Sb2000ModeSwitch final : public contracts::hardware::BusDevice {
 public:
  /**
   * @param fcMode FC 模式标志，由主机型号持有。
   * @param native 6576 模式下的设备。
   * @param fc FC 兼容模式下的设备。
   */
  Sb2000ModeSwitch(const bool& fcMode, contracts::hardware::BusDevice& native, contracts::hardware::BusDevice& fc) : fcMode_(fcMode), native_(native), fc_(fc) {}

  contracts::hardware::BusRead read(std::uint16_t address) override { return current().read(address); }

  contracts::hardware::BusRead peek(std::uint16_t address) override { return current().peek(address); }

  void write(std::uint16_t address, std::uint8_t value) override { current().write(address, value); }

 private:
  contracts::hardware::BusDevice& current() { return fcMode_ ? fc_ : native_; }

  const bool& fcMode_;
  contracts::hardware::BusDevice& native_;
  contracts::hardware::BusDevice& fc_;
};

}  // namespace ember::bootstrap
