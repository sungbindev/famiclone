/*
 * ------------------------------------------------------------------------
 * 名称：dma-2a03-test.cpp
 * 说明：精灵 DMA 的测试：暂停 513 或 514 个周期、按页复制 256 个字节到 $2004、传输期间 CPU 不访问总线、复位取消传输。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：测试台只接 CPU、调度器、三根信号线、内部 RAM、程序区、$2004 的记录设备与 DMA，不用 PPU。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <vector>

#include "contracts/hardware/bus-device.hpp"
#include "logic/hardware/bus/mapped-bus.hpp"
#include "logic/hardware/cpu-6502/cpu-6502.hpp"
#include "logic/hardware/dma-2a03/dma-2a03.hpp"
#include "logic/hardware/lines/wired-or-line.hpp"
#include "logic/hardware/scheduler/master-scheduler.hpp"

namespace {

namespace hw = ember::contracts::hardware;
using ember::logic::hardware::CpuAddressBus;
using ember::logic::hardware::Cpu6502;
using ember::logic::hardware::Dma2a03;
using ember::logic::hardware::MasterScheduler;
using ember::logic::hardware::WiredOrIrqLine;
using ember::logic::hardware::WiredOrRdyLine;

/** 内部 RAM（$0000-$1FFF，不镜像）。 */
class Ram final : public hw::BusDevice {
 public:
  Ram() : bytes(0x2000, 0) {}
  hw::BusRead read(std::uint16_t address) override { return hw::driveBus(bytes[address]); }
  void write(std::uint16_t address, std::uint8_t value) override { bytes[address] = value; }
  std::vector<std::uint8_t> bytes;
};

/** 程序区（$8000-$FFFF）：记录取指与读数据的次数，用来确认暂停期间 CPU 不访问总线。 */
class Program final : public hw::BusDevice {
 public:
  Program() : bytes(0x8000, 0xEA) {}
  hw::BusRead read(std::uint16_t address) override {
    ++reads;
    return hw::driveBus(bytes[address - 0x8000U]);
  }
  void write(std::uint16_t /*address*/, std::uint8_t /*value*/) override {}
  std::vector<std::uint8_t> bytes;
  int reads{0};
};

/** $2004 的记录设备：记录每次写入的值与时间。 */
class OamSink final : public hw::BusDevice {
 public:
  explicit OamSink(const hw::Clock& clock) : clock_(clock) {}
  hw::BusRead read(std::uint16_t /*address*/) override { return hw::releaseBus(); }
  void write(std::uint16_t /*address*/, std::uint8_t value) override {
    values.push_back(value);
    times.push_back(clock_.now());
  }
  std::vector<std::uint8_t> values;
  std::vector<hw::MasterTime> times;

 private:
  const hw::Clock& clock_;
};

class DmaRig {
 public:
  DmaRig() : sink(scheduler), dma(bus, scheduler, rdy, hw::ClockDomain{12}), cpu(bus, scheduler, hw::ClockDomain{12}, irq, nmi, rdy) {
    bus.mount(0x0000, 0x1FFF, ram);
    bus.mount(0x2004, 0x2004, sink);
    bus.mount(0x4014, 0x4014, dma);
    bus.mount(0x8000, 0xFFFF, program);
    // 页 2 的内容：每个字节是 index ^ 0x5A。
    for (int i = 0; i < 256; ++i) ram.bytes[0x200 + static_cast<std::size_t>(i)] = static_cast<std::uint8_t>(i ^ 0x5A);
  }

  /** 从 $8000 起写入程序，复位向量指向 $8000，并上电复位。 */
  void load(const std::vector<std::uint8_t>& code) {
    for (std::size_t i = 0; i < code.size(); ++i) program.bytes[i] = code[i];
    program.bytes[0xFFFC - 0x8000] = 0x00;
    program.bytes[0xFFFD - 0x8000] = 0x80;
    cpu.reset(true);
  }

  MasterScheduler scheduler;
  CpuAddressBus bus;
  WiredOrIrqLine irq;
  WiredOrIrqLine nmi;
  WiredOrRdyLine rdy;
  Ram ram;
  Program program;
  OamSink sink;
  Dma2a03 dma;
  Cpu6502 cpu;
};

constexpr std::uint8_t LDA_IMM = 0xA9;
constexpr std::uint8_t LDA_ZP = 0xA5;
constexpr std::uint8_t STA_ABS = 0x8D;
constexpr std::uint8_t NOP = 0xEA;

TEST(Dma2a03, StallsFor513CyclesWhenTheWriteIsOnAnEvenCycle) {
  DmaRig rig;
  // 复位后第 7 个周期开始：LDA #（2 个周期）之后 STA 从第 9 个周期开始，写发生在第 12 个周期（偶数）。
  rig.load({LDA_IMM, 0x02, STA_ABS, 0x14, 0x40, NOP, NOP});
  EXPECT_EQ(rig.cpu.run(1), 2U);
  EXPECT_EQ(rig.cpu.run(1), 4U);
  // 暂停发生在下一条指令的第一个读周期：NOP 本身 2 个周期，加上 513 个暂停周期。
  EXPECT_EQ(rig.cpu.run(1), 2U + 513U);
  EXPECT_FALSE(rig.dma.active());
  EXPECT_FALSE(rig.rdy.active());
}

TEST(Dma2a03, StallsFor514CyclesWhenTheWriteIsOnAnOddCycle) {
  DmaRig rig;
  // LDA 零页占 3 个周期：STA 从第 10 个周期开始，写发生在第 13 个周期（奇数）。
  rig.load({LDA_ZP, 0x00, STA_ABS, 0x14, 0x40, NOP, NOP});
  EXPECT_EQ(rig.cpu.run(1), 3U);
  EXPECT_EQ(rig.cpu.run(1), 4U);
  EXPECT_EQ(rig.cpu.run(1), 2U + 514U);
}

TEST(Dma2a03, CopiesTheWholePageIntoOamDataInOrderOneByteEveryTwoCycles) {
  DmaRig rig;
  rig.load({LDA_IMM, 0x02, STA_ABS, 0x14, 0x40, NOP, NOP});
  rig.cpu.run(3);
  rig.cpu.run(1);
  ASSERT_EQ(rig.sink.values.size(), 256U);
  for (std::size_t i = 0; i < 256; ++i) EXPECT_EQ(rig.sink.values[i], static_cast<std::uint8_t>(i ^ 0x5A)) << "第 " << i << " 个字节";
  // 两次写之间隔 2 个 CPU 周期（24 个主时钟周期）。
  for (std::size_t i = 1; i < 256; ++i) EXPECT_EQ(rig.sink.times[i] - rig.sink.times[i - 1], 24);
}

TEST(Dma2a03, TheCpuDoesNotTouchTheBusWhileItIsStalled) {
  DmaRig rig;
  rig.load({LDA_IMM, 0x02, STA_ABS, 0x14, 0x40, NOP, NOP});
  rig.cpu.run(1);  // LDA
  rig.cpu.run(1);  // STA：写 $4014，DMA 在下一条指令的第一个读周期开始暂停
  const int readsBefore = rig.program.reads;
  rig.cpu.run(1);
  // NOP 只需要取指与一次空读；暂停的 513 个周期里没有任何程序区访问。
  EXPECT_EQ(rig.program.reads - readsBefore, 2);
}

TEST(Dma2a03, ResetCancelsATransferInProgress) {
  DmaRig rig;
  rig.load({NOP});
  rig.dma.write(0x4014, 0x02);
  EXPECT_TRUE(rig.dma.active());
  EXPECT_TRUE(rig.rdy.active());
  rig.dma.reset(false);
  EXPECT_FALSE(rig.dma.active());
  EXPECT_FALSE(rig.rdy.active());
  rig.scheduler.advance(12 * 1000);
  EXPECT_TRUE(rig.sink.values.empty());
}

TEST(Dma2a03, ReadsDoNotDriveTheBus) {
  DmaRig rig;
  EXPECT_EQ(rig.dma.read(0x4014).drivenMask, 0);
}

TEST(Dma2a03, RejectsANonPositiveClockDivision) {
  MasterScheduler scheduler;
  CpuAddressBus bus;
  WiredOrRdyLine rdy;
  EXPECT_THROW(Dma2a03(bus, scheduler, rdy, hw::ClockDomain{0}), ember::shared::CoreError);
}

}  // namespace
