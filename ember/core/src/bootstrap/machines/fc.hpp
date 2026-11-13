/*
 * ------------------------------------------------------------------------
 * 名称：fc.hpp
 * 说明：主机型号 fc（经典 FC，NTSC）的装配：2K 内部 RAM、2C02 PPU、精灵 DMA、手柄口、卡槽、2K 名称表显存，APU 以空设备占位。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：时钟：主时钟 21.477272 MHz，CPU 12 分频、PPU 4 分频；CPU 总线与 PPU 总线的译码见 fc.cpp。
 *       具体实现只在这里创建并连线，模块之间不互相引用。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <array>
#include <cstdint>

#include "contracts/hardware/machine.hpp"
#include "logic/hardware/cartridge-slot/standard-cartridge-slot.hpp"
#include "logic/hardware/cpu-6502/cpu-6502.hpp"
#include "logic/hardware/dma-2a03/dma-2a03.hpp"
#include "logic/hardware/bus/mapped-bus.hpp"
#include "logic/hardware/joypad/joypad.hpp"
#include "logic/hardware/lines/wired-or-line.hpp"
#include "logic/hardware/nametable-ram/nametable-ram.hpp"
#include "logic/hardware/null-device/null-device.hpp"
#include "logic/hardware/pad-2a03/pad-2a03.hpp"
#include "logic/hardware/ppu-2c02/ppu-2c02.hpp"
#include "logic/hardware/ram/ram-block.hpp"
#include "logic/hardware/scheduler/master-scheduler.hpp"

namespace ember::bootstrap {

/** 经典 FC（NTSC）。 */
class FcMachine final : public contracts::hardware::Machine {
 public:
  /** 每个 CPU 周期的主时钟周期数。 */
  static constexpr contracts::hardware::MasterTime CPU_TICKS_PER_CYCLE = 12;
  /** 每个 PPU 点的主时钟周期数。 */
  static constexpr contracts::hardware::MasterTime PPU_TICKS_PER_DOT = 4;
  /** 上电后 PPU 第 0 点相对时间 0 的延迟（主时钟周期数），决定 CPU 与 PPU 的相位，按 blargg 的 PPU 时序测试调定。 */
  static constexpr contracts::hardware::MasterTime PPU_PHASE_TICKS = 1;

  FcMachine();

  FcMachine(const FcMachine&) = delete;
  FcMachine& operator=(const FcMachine&) = delete;

  void powerOn() override;
  void powerOff() override;
  void softReset() override;
  [[nodiscard]] bool isPoweredOn() const override { return poweredOn_; }
  void run(contracts::hardware::MasterTime ticks) override;
  void runFrame() override;
  [[nodiscard]] contracts::hardware::FrameBuffer frame() const override { return ppu_.frame(); }
  void setButtons(unsigned port, std::uint8_t buttons) override;
  [[nodiscard]] contracts::hardware::CpuRegisters cpuRegisters() const override { return cpu_.registers(); }
  [[nodiscard]] const contracts::hardware::Clock& clock() const override { return scheduler_; }
  contracts::hardware::CartridgeSlot& cartridgeSlot() override { return slot_; }

  // 测试与调试用
  [[nodiscard]] logic::hardware::Ppu2c02& ppu() { return ppu_; }
  [[nodiscard]] const logic::hardware::Ppu2c02& ppu() const { return ppu_; }
  /** 2K 内部 RAM。 */
  [[nodiscard]] const contracts::hardware::StorageRegion& internalRam() const { return ram_; }
  /** 主板上的名称表显存（2K）。 */
  [[nodiscard]] const logic::hardware::NametableRam& nametableRam() const { return nametableRam_; }
  /** 不经 CPU 直接读 PPU 总线（图案表与名称表，不含调色板），读不改变 PPU 的状态。 */
  std::uint8_t peekPpuBus(std::uint16_t address) { return ppuBus_.read(address); }

 private:
  logic::hardware::MasterScheduler scheduler_;
  logic::hardware::WiredOrIrqLine irq_;
  logic::hardware::WiredOrIrqLine nmi_;
  logic::hardware::WiredOrRdyLine rdy_;
  logic::hardware::CpuAddressBus cpuBus_;
  logic::hardware::PpuAddressBus ppuBus_;

  logic::hardware::RamBlock ram_;
  logic::hardware::MirroredRam ramDevice_;
  logic::hardware::StandardCartridgeSlot slot_;
  logic::hardware::NametableRam nametableRam_;
  /** APU 的位置：本阶段是空设备，实现 apu-2a03 后在这里替换。 */
  logic::hardware::NullDevice apu_;
  logic::hardware::Ppu2c02 ppu_;
  logic::hardware::Dma2a03 dma_;
  logic::hardware::Pad2a03 pad_;
  std::array<logic::hardware::Joypad, logic::hardware::Pad2a03::PORT_COUNT> joypads_;
  logic::hardware::Cpu6502 cpu_;

  bool poweredOn_{false};
};

}  // namespace ember::bootstrap
