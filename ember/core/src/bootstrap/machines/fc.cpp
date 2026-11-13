/*
 * ------------------------------------------------------------------------
 * 名称：fc.cpp
 * 说明：主机型号 fc（经典 FC，NTSC）的装配、电源与运行。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：CPU 总线：$0000-$1FFF 内部 RAM（2K 镜像）、$2000-$3FFF PPU（每 8 字节镜像）、$4000-$4013 与 $4015 APU（空设备占位）、
 *       $4014 精灵 DMA、$4016-$4017 手柄口、$4020-$FFFF 卡槽；$4018-$401F 无设备（开放总线）；
 *       PPU 总线：$0000-$1FFF 图案表（卡槽）、$2000-$3EFF 名称表（按显存控制线选择主板显存或卡带），调色板在 PPU 内。
 * ------------------------------------------------------------------------
 */

#include "bootstrap/machines/fc.hpp"

#include "contracts/hardware/hardware-error.hpp"
#include "shared/errors/core-error.hpp"

namespace ember::bootstrap {

namespace hw = contracts::hardware;

namespace {

/** 内部 RAM 的容量（字节）。 */
constexpr std::size_t INTERNAL_RAM_SIZE = 2048;

}  // namespace

FcMachine::FcMachine()
    : ram_(INTERNAL_RAM_SIZE),
      ramDevice_(ram_),
      nametableRam_(slot_),
      ppu_(scheduler_, ppuBus_, nmi_, hw::ClockDomain{PPU_TICKS_PER_DOT}, PPU_PHASE_TICKS),
      dma_(cpuBus_, scheduler_, rdy_, hw::ClockDomain{CPU_TICKS_PER_CYCLE}),
      cpu_(cpuBus_, scheduler_, hw::ClockDomain{CPU_TICKS_PER_CYCLE}, irq_, nmi_, rdy_) {
  cpuBus_.mount(0x0000, 0x1FFF, ramDevice_);
  cpuBus_.mount(0x2000, 0x3FFF, ppu_);
  cpuBus_.mount(0x4000, 0x4013, apu_);
  cpuBus_.mount(0x4014, 0x4014, dma_);
  cpuBus_.mount(0x4015, 0x4015, apu_);
  cpuBus_.mount(0x4016, 0x4017, pad_);
  cpuBus_.mount(0x4020, 0xFFFF, slot_.cpuSide());

  ppuBus_.mount(0x0000, 0x1FFF, slot_.ppuSide());
  ppuBus_.mount(0x2000, 0x3EFF, nametableRam_);

  for (unsigned index = 0; index < joypads_.size(); ++index) pad_.port(index).plug(&joypads_[index]);
}

void FcMachine::powerOn() {
  // 时钟先归零并丢弃旧事件，各设备复位时再登记自己的事件；CPU 最后复位，复位序列会推进时钟。
  scheduler_.reset();
  ram_.fill(0);
  nametableRam_.fill(0);
  apu_.reset(true);
  dma_.reset(true);
  ppu_.reset(true);
  slot_.reset(true);
  poweredOn_ = true;
  cpu_.reset(true);
}

void FcMachine::powerOff() { poweredOn_ = false; }

void FcMachine::softReset() {
  if (!poweredOn_) return;
  apu_.reset(false);
  dma_.reset(false);
  ppu_.reset(false);
  slot_.reset(false);
  cpu_.reset(false);
}

void FcMachine::run(hw::MasterTime ticks) {
  if (!poweredOn_ || ticks <= 0) return;
  const hw::MasterTime cycles = (ticks + CPU_TICKS_PER_CYCLE - 1) / CPU_TICKS_PER_CYCLE;
  cpu_.run(static_cast<std::uint32_t>(cycles));
  ppu_.catchUp(scheduler_.now());
}

void FcMachine::runFrame() {
  if (!poweredOn_) return;
  const std::uint64_t target = ppu_.frameCount() + 1;
  while (ppu_.frameCount() < target) {
    cpu_.run(1);
    ppu_.catchUp(scheduler_.now());
  }
}

void FcMachine::setButtons(unsigned port, std::uint8_t buttons) {
  if (port >= joypads_.size()) throw shared::CoreError(hw::ERROR_PORT_NOT_FOUND, "手柄口只有 2 个，编号为 0 与 1");
  joypads_[port].setButtons(buttons);
}

}  // namespace ember::bootstrap
