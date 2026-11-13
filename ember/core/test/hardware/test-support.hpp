/*
 * ------------------------------------------------------------------------
 * 名称：test-support.hpp
 * 说明：硬件测试的公共辅助：记录访问的 64K 内存、装配好的 CPU 测试台、按时间点改变信号线的事件。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：测试台把 CPU、总线、三根信号线与内存按真机的方式连起来；内存记录每次访问的类型、地址、数据与发生时的主时钟。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>
#include <initializer_list>
#include <vector>

#include "contracts/hardware/bus-device.hpp"
#include "contracts/hardware/scheduler.hpp"
#include "logic/hardware/bus/mapped-bus.hpp"
#include "logic/hardware/cpu-6502/cpu-6502.hpp"
#include "logic/hardware/lines/wired-or-line.hpp"
#include "logic/hardware/scheduler/master-scheduler.hpp"

namespace ember::test {

namespace hw = ember::contracts::hardware;

/** 覆盖整个 64K 地址空间的内存，记录每次访问。 */
class RecordingMemory final : public hw::BusDevice {
 public:
  /** 一次总线访问。 */
  struct Access {
    bool write;
    std::uint16_t address;
    std::uint8_t value;
    hw::MasterTime time;
  };

  explicit RecordingMemory(const hw::Clock& clock) : clock_(clock), bytes_(0x10000, 0) {}

  hw::BusRead read(std::uint16_t address) override {
    const std::uint8_t value = bytes_[address];
    log.push_back(Access{false, address, value, clock_.now()});
    return hw::driveBus(value);
  }

  void write(std::uint16_t address, std::uint8_t value) override {
    log.push_back(Access{true, address, value, clock_.now()});
    bytes_[address] = value;
  }

  /** 不经总线直接写入，不记录。 */
  void poke(std::uint16_t address, std::uint8_t value) { bytes_[address] = value; }

  /** 从地址起依次写入一串字节，不记录。 */
  void load(std::uint16_t address, std::initializer_list<std::uint8_t> data) {
    for (const std::uint8_t byte : data) bytes_[address++] = byte;
  }

  /** 不经总线直接读取，不记录。 */
  [[nodiscard]] std::uint8_t peek(std::uint16_t address) const { return bytes_[address]; }

  std::vector<Access> log;

 private:
  const hw::Clock& clock_;
  std::vector<std::uint8_t> bytes_;
};

/** 按时间点把信号线置有效或无效：登记事件时标记非 0 为有效，0 为无效。 */
class LineEvent final : public hw::EventListener {
 public:
  LineEvent(hw::SignalLine& line, hw::SignalSource source) : line_(line), source_(source) {}

  void onEvent(hw::MasterTime /*when*/, std::uint32_t tag) override { line_.set(source_, tag != 0); }

 private:
  hw::SignalLine& line_;
  hw::SignalSource source_;
};

/** CPU 测试台。 */
class CpuRig {
 public:
  /** @param ticksPerCycle 每个 CPU 周期的主时钟周期数，默认与 NTSC 经典 FC 一致。 */
  explicit CpuRig(hw::MasterTime ticksPerCycle = 12)
      : memory(scheduler), cpu(bus, scheduler, hw::ClockDomain{ticksPerCycle}, irq, nmi, rdy), irqSource(irq.addSource()), nmiSource(nmi.addSource()), rdySource(rdy.addSource()) {
    bus.mount(0x0000, 0xFFFF, memory);
  }

  /** 设置复位向量并上电复位，之后的访问记录从零开始。 */
  void startAt(std::uint16_t pc) {
    memory.poke(0xFFFC, static_cast<std::uint8_t>(pc & 0xFF));
    memory.poke(0xFFFD, static_cast<std::uint8_t>(pc >> 8));
    cpu.reset(true);
    memory.log.clear();
  }

  /** 设置 IRQ 与 NMI 向量。 */
  void setVectors(std::uint16_t irqHandler, std::uint16_t nmiHandler) {
    memory.poke(0xFFFE, static_cast<std::uint8_t>(irqHandler & 0xFF));
    memory.poke(0xFFFF, static_cast<std::uint8_t>(irqHandler >> 8));
    memory.poke(0xFFFA, static_cast<std::uint8_t>(nmiHandler & 0xFF));
    memory.poke(0xFFFB, static_cast<std::uint8_t>(nmiHandler >> 8));
  }

  /** 运行一条指令（含随后响应的中断），返回消耗的周期数。 */
  std::uint32_t step() { return cpu.run(1); }

  ember::logic::hardware::MasterScheduler scheduler;
  ember::logic::hardware::CpuAddressBus bus;
  ember::logic::hardware::WiredOrIrqLine irq;
  ember::logic::hardware::WiredOrIrqLine nmi;
  ember::logic::hardware::WiredOrRdyLine rdy;
  RecordingMemory memory;
  ember::logic::hardware::Cpu6502 cpu;
  hw::SignalSource irqSource;
  hw::SignalSource nmiSource;
  hw::SignalSource rdySource;
};

}  // namespace ember::test
