/*
 * ------------------------------------------------------------------------
 * 名称：sb2000-fc-board.hpp
 * 说明：主机型号 sb2000 在 FC 兼容模式下的主板逻辑：128K 窗口选择与 MMC3 页号到扩展 RAM、扩展显存的换算、FC 模式的程序窗口与显存总线，以及按模式转发的总线设备。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：这是 sb2000 主板自己的逻辑，只由 sb2000.cpp 创建并挂到总线上，其他学习机机型没有；MMC3 是卡带上的 mapper 芯片，实现在公共的 logic/mapper-chips/mmc3，这里只做主板接线；
 *       FC 兼容模式与 MMC3 的行为没有在真机上实测过，见 docs/02 第 6.8 节；
 *       窗口：扩展 RAM 与扩展显存都按 128K 分成 4 个窗口，$5006 选窗口，MMC3 的寄存器只能在窗口内选页：程序页号线接 4 根（取页号低 4 位，每页 8K，共 16 页），图案页号线接 7 根（取低 7 位，每页 1K，共 128 页）；
 *       FC 模式下图案区只读（写入被丢弃），名称表在字符 RAM 的 $2000-$27FF，按 $A000 的镜像位连接。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "bootstrap/machines/sb2000-board.hpp"
#include "contracts/hardware/bus-device.hpp"
#include "contracts/hardware/irq-line.hpp"
#include "logic/mapper-chips/mmc3/mmc3.hpp"

namespace ember::bootstrap {

/** sb2000 的 FC 模式映射：MMC3 芯片加 128K 窗口选择，把页号换算成扩展 RAM 与扩展显存里的偏移。 */
class Sb2000FcMapper {
 public:
  /** 一个窗口的大小（字节）：128K，程序 16 页（每页 8K）或图案 128 页（每页 1K）。 */
  static constexpr std::size_t WINDOW_SIZE = 128 * 1024;
  /** A12 要先低过这么多次显存访问，下一次上升沿才计数。 */
  static constexpr unsigned A12_LOW_ACCESSES = logic::mapper_chips::Mmc3::A12_LOW_ACCESSES;

  /** @param irq IRQ 线，MMC3 在其上登记一个来源。 */
  explicit Sb2000FcMapper(contracts::hardware::IrqLine& irq) : chip_(irq) {}

  /** 上电：MMC3 回到初始值，窗口 0。 */
  void reset() {
    chip_.reset();
    window_ = 0;
  }

  /** 清 MMC3 的 IRQ 计数器与允许位，撤销 IRQ（进入 FC 模式时）。 */
  void resetIrq() { chip_.resetIrq(); }

  /** 写 $5006：选 128K 窗口，同时关闭并撤销 IRQ。 */
  void selectWindow(std::uint8_t value) {
    window_ = static_cast<unsigned>(((value >> 2) & 2U) | ((value >> 1) & 1U));
    chip_.disableIrq();
  }

  /** FC 模式下对 $8000-$FFFF 的写入：交给 MMC3 的寄存器。 */
  void writeRegister(std::uint16_t address, std::uint8_t value) { chip_.writeRegister(address, value); }

  /** CPU 地址（不小于 $8000）在扩展 RAM 里的偏移。 */
  [[nodiscard]] std::size_t programOffset(std::uint16_t address) const {
    const std::size_t page = chip_.programBank(address) & PROGRAM_BANK_MASK;
    return window_ * WINDOW_SIZE + page * PROGRAM_PAGE_SIZE + (address & (PROGRAM_PAGE_SIZE - 1U));
  }

  /** 图案地址（$0000-$1FFF）在扩展显存里的偏移。 */
  [[nodiscard]] std::size_t characterOffset(std::uint16_t address) const {
    const std::size_t kilobyte = chip_.characterBank(address) & CHARACTER_BANK_MASK;
    return window_ * WINDOW_SIZE + kilobyte * CHARACTER_PAGE_SIZE + (address & (CHARACTER_PAGE_SIZE - 1U));
  }

  /** 名称表地址（$2000-$3EFF，$3000 起镜像 $2000 起）在字符 RAM 里的偏移。 */
  [[nodiscard]] std::size_t nametableOffset(std::uint16_t address) const {
    const unsigned masked = address & 0x2FFFU;
    const unsigned page = chip_.horizontalMirroring() ? ((masked >> 11) & 1U) : ((masked >> 10) & 1U);
    return NAMETABLE_BASE + page * CHARACTER_PAGE_SIZE + (masked & (CHARACTER_PAGE_SIZE - 1U));
  }

  /** PPU 对显存总线的一次访问：交给 MMC3 观察 A12。 */
  void onVideoAddress(std::uint16_t address) { chip_.onVideoAddress(address); }

  // 测试与调试用
  [[nodiscard]] unsigned window() const { return window_; }
  [[nodiscard]] std::uint8_t selector() const { return chip_.selector(); }
  [[nodiscard]] std::uint8_t registerValue(unsigned index) const { return chip_.registerValue(index); }
  [[nodiscard]] bool horizontalMirroring() const { return chip_.horizontalMirroring(); }
  [[nodiscard]] std::uint8_t irqLatch() const { return chip_.irqLatch(); }
  [[nodiscard]] std::uint8_t irqCounter() const { return chip_.irqCounter(); }
  [[nodiscard]] bool irqEnabled() const { return chip_.irqEnabled(); }

 private:
  static constexpr std::size_t PROGRAM_PAGE_SIZE = 8 * 1024;
  static constexpr std::size_t CHARACTER_PAGE_SIZE = 1024;
  /** 板子接的页号线：程序 4 根（16 页），图案 7 根（128 页）；固定页的页号 $3E、$3F 因此落在 14、15。 */
  static constexpr unsigned PROGRAM_BANK_MASK = 0x0F;
  static constexpr unsigned CHARACTER_BANK_MASK = 0x7F;
  /** 名称表在字符 RAM 里的起点。 */
  static constexpr std::size_t NAMETABLE_BASE = 0x2000;

  logic::mapper_chips::Mmc3 chip_;
  unsigned window_{0};
};

/** FC 模式的程序窗口 $8000-$FFFF：读来自扩展 RAM 的 MMC3 页，写是 MMC3 寄存器。 */
class Sb2000FcProgram final : public contracts::hardware::BusDevice {
 public:
  Sb2000FcProgram(const std::vector<std::uint8_t>& extendedRam, Sb2000FcMapper& mapper) : extendedRam_(extendedRam), mapper_(mapper) {}

  contracts::hardware::BusRead read(std::uint16_t address) override { return contracts::hardware::driveBus(extendedRam_[mapper_.programOffset(address)]); }

  contracts::hardware::BusRead peek(std::uint16_t address) override { return read(address); }

  void write(std::uint16_t address, std::uint8_t value) override { mapper_.writeRegister(address, value); }

 private:
  const std::vector<std::uint8_t>& extendedRam_;
  Sb2000FcMapper& mapper_;
};

/** FC 模式的显存总线：图案区读自扩展显存的 MMC3 页（只读），名称表在字符 RAM；每次访问都让映射器观察 A12。 */
class Sb2000FcVideo final : public contracts::hardware::BusDevice {
 public:
  Sb2000FcVideo(Sb2000VideoMemory& memory, Sb2000FcMapper& mapper) : memory_(memory), mapper_(mapper) {}

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
  Sb2000FcMapper& mapper_;
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
