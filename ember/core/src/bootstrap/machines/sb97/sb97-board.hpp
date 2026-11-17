/*
 * ------------------------------------------------------------------------
 * 名称：sb97-board.hpp
 * 说明：主机型号 sb97（小霸王 97 学习机）主板上的存储与整机寄存器：1MB 的 BIOS ROM、8K 的 RAM、8K 的字符 RAM、4K 的名称表显存，以及 $5000 起的几个整机寄存器。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-17
 * 备注：这些是主机型号自己的胶合逻辑（不属于任何芯片模块），只由 sb97.cpp 创建并挂到总线上；地址译码由 BIOS 的反汇编推断，没有经过真机核对，见 machines/sb97/disassembly/docs/README.md；
 *       CPU 地址 $4020-$FFFF：
 *         $4020-$480C、$480E-$4FFF、$5100-$51FF、$5400-$54FF、$5800-$5FFF 没有器件（开放总线），$480D 写：键盘灯（只写，忽略）；
 *         $5000 写：选择 $8000-$BFFF 的 16K 页（0 到 63），$5200 写：整机控制位（只存，含义待核对），
 *         $5300 读写：声音数据口的存根（读时位 7 为 1，表示总是就绪，写入忽略），
 *         $5500 写：软驱控制器的数据率，$5501 写：软驱控制器的 DOR，$5505 写：软驱数据，$5604 读：软驱主状态，$5605 读：软驱数据；
 *         $6000-$7FFF：8K 的 RAM；
 *         $8000-$BFFF：ROM 的一个 16K 页，$C000-$FFFF：ROM 的第 0 页（固定）；
 *       PPU 地址：$0000-$1FFF 是 8K 的字符 RAM，$2000-$2FFF 是 4 个名称表（4K 显存，四屏，$3000-$3EFF 是它的镜像）。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "contracts/hardware/bus-device.hpp"
#include "contracts/hardware/interrupt-sink.hpp"
#include "contracts/hardware/irq-line.hpp"
#include "logic/hardware/fdc-upd765/fdc-upd765.hpp"

namespace ember::bootstrap {

/** 软驱控制器的 INT 脚：电平接 CPU 的 IRQ 线，控制器发出请求时拉低，固件读过结果后（控制器撤销中断）由主板重新检查。 */
class Sb97FdcInterrupt final : public contracts::hardware::InterruptSink {
 public:
  /**
   * @param irq CPU 的 IRQ 线，本对象在其上登记一个来源。
   * @param fdc 软驱控制器，只读它的中断状态。
   */
  Sb97FdcInterrupt(contracts::hardware::IrqLine& irq, const logic::hardware::FdcUpd765& fdc) : irq_(irq), source_(irq.addSource()), fdc_(fdc) {}

  void raise(std::uint8_t /*sourceMask*/) override { irq_.set(source_, true); }

  /** 按控制器当前的中断状态与 DOR 的中断允许位重新定电平。 */
  void refresh() { irq_.set(source_, fdc_.irqRaised() && (fdc_.dor() & DOR_IRQ_ENABLE) != 0); }

  /** 复位：释放 IRQ 线。 */
  void release() { irq_.set(source_, false); }

 private:
  /** DOR 的位 3：允许中断与 DMA 请求。 */
  static constexpr std::uint8_t DOR_IRQ_ENABLE = 0x08;

  contracts::hardware::IrqLine& irq_;
  contracts::hardware::SignalSource source_;
  const logic::hardware::FdcUpd765& fdc_;
};

/** 小霸王 97 主板。 */
class Sb97Board final {
 public:
  /** BIOS ROM 的容量（字节）。 */
  static constexpr std::size_t ROM_SIZE = 1024 * 1024;
  /** 程序页的大小（字节）与页数。 */
  static constexpr std::size_t PAGE_SIZE = 16 * 1024;
  static constexpr std::size_t PAGE_COUNT = ROM_SIZE / PAGE_SIZE;
  /** $6000-$7FFF 的 RAM 容量（字节）。 */
  static constexpr std::size_t RAM_SIZE = 8 * 1024;
  /** 字符 RAM 的容量（字节）。 */
  static constexpr std::size_t CHARACTER_RAM_SIZE = 8 * 1024;
  /** 名称表显存的容量（字节）：4 个名称表各 1K。 */
  static constexpr std::size_t NAMETABLE_SIZE = 4096;

  /** 寄存器的快照，供调试。 */
  struct Registers {
    std::uint8_t programPage;
    std::uint8_t control;
  };

  /** CPU 地址对应的存储位置。 */
  struct CpuLocation {
    enum class Kind : std::uint8_t { None, Register, Rom, Ram };
    Kind kind{Kind::None};
    std::size_t offset{0};
  };

  /**
   * @param rom BIOS ROM 的内容，大小必须等于 ROM_SIZE。
   * @param fdc 软驱控制器。
   * @param fdcInterrupt 软驱控制器的 INT 脚；软驱寄存器每次读写之后重新检查它的电平。
   * @throws shared::CoreError hardware.invalidArgument：BIOS 大小不对。
   */
  Sb97Board(std::vector<std::uint8_t> rom, logic::hardware::FdcUpd765& fdc, Sb97FdcInterrupt& fdcInterrupt);

  Sb97Board(const Sb97Board&) = delete;
  Sb97Board& operator=(const Sb97Board&) = delete;

  /** CPU 总线上 $4020-$FFFF 的设备。 */
  contracts::hardware::BusDevice& cpuSide() { return cpuSide_; }
  /** PPU 总线上 $0000-$3EFF 的设备。 */
  contracts::hardware::BusDevice& videoSide() { return videoSide_; }

  /**
   * 复位：寄存器回到初值；上电时存储清零，软复位时存储保持。
   * @param power 是否上电。
   */
  void reset(bool power);

  // 调试与测试用
  [[nodiscard]] const std::vector<std::uint8_t>& rom() const { return rom_; }
  [[nodiscard]] const std::vector<std::uint8_t>& ram() const { return ram_; }
  [[nodiscard]] std::vector<std::uint8_t>& ram() { return ram_; }
  [[nodiscard]] const std::vector<std::uint8_t>& characterRam() const { return characterRam_; }
  [[nodiscard]] std::vector<std::uint8_t>& characterRam() { return characterRam_; }
  [[nodiscard]] const std::array<std::uint8_t, NAMETABLE_SIZE>& nametable() const { return nametable_; }
  [[nodiscard]] std::array<std::uint8_t, NAMETABLE_SIZE>& nametable() { return nametable_; }
  [[nodiscard]] Registers registers() const { return Registers{programPage_, control_}; }
  /** CPU 地址当前对应的存储位置。 */
  [[nodiscard]] CpuLocation locate(std::uint16_t address) const;
  /** PPU 地址（$0000-$3EFF）当前对应的位置：图案区给字符 RAM 的偏移，名称表给显存偏移。 */
  [[nodiscard]] std::size_t locateVideo(std::uint16_t address, bool& isPattern) const;

 private:
  /** CPU 侧：$4020-$FFFF。 */
  class CpuSide final : public contracts::hardware::BusDevice {
   public:
    explicit CpuSide(Sb97Board& board) : board_(board) {}
    contracts::hardware::BusRead read(std::uint16_t address) override;
    contracts::hardware::BusRead peek(std::uint16_t address) override;
    void write(std::uint16_t address, std::uint8_t value) override;

   private:
    Sb97Board& board_;
  };

  /** PPU 侧：图案区与名称表。 */
  class VideoSide final : public contracts::hardware::BusDevice {
   public:
    explicit VideoSide(Sb97Board& board) : board_(board) {}
    contracts::hardware::BusRead read(std::uint16_t address) override;
    contracts::hardware::BusRead peek(std::uint16_t address) override;
    void write(std::uint16_t address, std::uint8_t value) override;

   private:
    Sb97Board& board_;
  };

  contracts::hardware::BusRead readCpu(std::uint16_t address, bool sideEffects);
  void writeCpu(std::uint16_t address, std::uint8_t value);
  [[nodiscard]] static std::size_t nametableOffset(std::uint16_t address);

  logic::hardware::FdcUpd765& fdc_;
  Sb97FdcInterrupt& fdcInterrupt_;
  CpuSide cpuSide_{*this};
  VideoSide videoSide_{*this};

  std::vector<std::uint8_t> rom_;
  std::vector<std::uint8_t> ram_;
  std::vector<std::uint8_t> characterRam_;
  std::array<std::uint8_t, NAMETABLE_SIZE> nametable_{};

  std::uint8_t programPage_{0};
  std::uint8_t control_{0};
};

}  // namespace ember::bootstrap
