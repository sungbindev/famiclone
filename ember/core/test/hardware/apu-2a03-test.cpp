/*
 * ------------------------------------------------------------------------
 * 名称：apu-2a03-test.cpp
 * 说明：APU 的单元测试：各通道的寄存器行为（长度计数器、包络、方波与扫频、三角波、噪声、DMC）、帧计数器与帧 IRQ 的时序、$4015 状态、
 *       DMC 的取样 DMA（暂停 CPU 的周期数、IRQ、速率）、混音输出范围与无输入时输出为 0、带限采样器的采样数、阶跃响应与对超奈奎斯特频率的抑制。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：通道类直接测试；整个 APU 用最小的测试台（调度器、总线、线、64K 内存里的采样）测试，时间按 CPU 周期推进，
 *       “一次写入”占一个 CPU 周期（写发生在周期的中点），APU 在被访问前追赶到当时的时间。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <vector>

#include "contracts/hardware/bus-device.hpp"
#include "logic/hardware/apu-2a03/apu-2a03.hpp"
#include "logic/hardware/apu-2a03/apu-channels.hpp"
#include "logic/hardware/apu-2a03/apu-tables.hpp"
#include "logic/hardware/apu-2a03/band-limited-sampler.hpp"
#include "logic/hardware/bus/mapped-bus.hpp"
#include "logic/hardware/lines/wired-or-line.hpp"
#include "logic/hardware/scheduler/master-scheduler.hpp"
#include "shared/errors/core-error.hpp"

namespace {

namespace hw = ember::contracts::hardware;
namespace apu = ember::logic::hardware::apu;
using ember::logic::hardware::Apu2a03;

constexpr double CPU_HZ = 21'477'272.0 / 12.0;

// ---------------------------------------------------------------- 通道

/** 载入长度计数器并让延迟的载入生效。 */
template <typename Channel>
void enableWithLength(Channel& channel, std::uint8_t lengthIndex, unsigned reg) {
  channel.length().setEnabled(true);
  channel.write(reg, static_cast<std::uint8_t>(lengthIndex << 3));
  channel.endCycle();
}

TEST(ApuLengthCounter, LoadsFromTheTableCountsDownAndRespectsHalt) {
  apu::LengthCounter length;
  length.setEnabled(true);
  length.load(1);
  EXPECT_FALSE(length.active()) << "载入在下一个周期末才生效";
  length.endCycle();
  EXPECT_EQ(length.value(), apu::LENGTH_TABLE[1]);

  for (int i = 0; i < 254; ++i) length.clock();
  EXPECT_FALSE(length.active());

  length.load(3);
  length.endCycle();
  EXPECT_EQ(length.value(), 2);
  length.setHalt(true);
  length.clock();
  EXPECT_EQ(length.value(), 1) << "暂停位在下一个周期末才生效，这一次时钟仍然减 1";
  length.endCycle();
  length.clock();
  EXPECT_EQ(length.value(), 1);
}

TEST(ApuLengthCounter, DisabledCountersStayZeroAndIgnoreLoads) {
  apu::LengthCounter length;
  length.setEnabled(true);
  length.load(0);
  length.endCycle();
  ASSERT_TRUE(length.active());
  length.setEnabled(false);
  EXPECT_FALSE(length.active());
  length.load(0);
  length.endCycle();
  EXPECT_FALSE(length.active());
}

TEST(ApuLengthCounter, ALoadInTheSameCycleAsAClockIsIgnoredWhenTheCounterWasRunning) {
  apu::LengthCounter length;
  length.setEnabled(true);
  length.load(1);
  length.endCycle();
  ASSERT_EQ(length.value(), 254);
  length.load(3);  // 载入与时钟落在同一个周期
  length.clock();
  length.endCycle();
  EXPECT_EQ(length.value(), 253) << "计数被时钟改变过，载入作废";

  apu::LengthCounter zero;
  zero.setEnabled(true);
  zero.load(3);
  zero.clock();
  zero.endCycle();
  EXPECT_EQ(zero.value(), 2) << "计数为 0 时时钟没有改变它，载入生效";
}

TEST(ApuEnvelope, DecaysFromFifteenAndLoopsOnlyWhenAsked) {
  apu::Envelope envelope;
  envelope.write(0x00);  // 衰减，分频周期 0
  envelope.restart();
  envelope.clock(false);
  EXPECT_EQ(envelope.volume(), 15);
  for (int expected = 14; expected >= 0; --expected) {
    envelope.clock(false);
    EXPECT_EQ(envelope.volume(), expected);
  }
  envelope.clock(false);
  EXPECT_EQ(envelope.volume(), 0) << "不循环时停在 0";
  envelope.clock(true);
  EXPECT_EQ(envelope.volume(), 15) << "循环时回到 15";

  apu::Envelope slow;
  slow.write(0x02);  // 分频周期 2：每 3 次时钟衰减一步
  slow.restart();
  slow.clock(false);
  EXPECT_EQ(slow.volume(), 15);
  slow.clock(false);
  slow.clock(false);
  EXPECT_EQ(slow.volume(), 15);
  slow.clock(false);
  EXPECT_EQ(slow.volume(), 14);

  apu::Envelope constant;
  constant.write(0x1A);  // 固定音量 10
  EXPECT_EQ(constant.volume(), 10);
}

/** 让方波通道连续运行 steps 步，返回每一步的输出（周期 period 时每步 2×(period+1) 个 CPU 周期）。 */
std::vector<int> pulseSteps(apu::PulseChannel& pulse, unsigned period, int steps) {
  std::vector<int> outputs;
  for (int step = 0; step < steps; ++step) {
    for (unsigned cycle = 0; cycle < (period + 1) * 2; ++cycle) pulse.clockTimer();
    outputs.push_back(pulse.output());
  }
  return outputs;
}

TEST(ApuPulse, EachDutyHasTheExpectedNumberOfHighStepsPerCycle) {
  const int expectedHigh[4] = {1, 2, 4, 6};
  for (unsigned duty = 0; duty < 4; ++duty) {
    apu::PulseChannel pulse(false);
    enableWithLength(pulse, 1, 3);
    pulse.write(0, static_cast<std::uint8_t>((duty << 6) | 0x3F));  // 暂停位、固定音量 15
    pulse.write(2, 0x10);
    pulse.write(3, 0x08);
    pulse.endCycle();
    pulse.endCycle();
    const auto outputs = pulseSteps(pulse, 0x10, 64);
    const int high = static_cast<int>(std::count(outputs.begin(), outputs.end(), 15));
    EXPECT_EQ(high, expectedHigh[duty] * 8) << "占空比 " << duty;
    EXPECT_EQ(static_cast<int>(std::count(outputs.begin(), outputs.end(), 0)) + high, 64);
  }
}

TEST(ApuPulse, IsSilentWithoutALengthCounterOrWhenThePeriodIsBelowEight) {
  apu::PulseChannel pulse(false);
  pulse.write(0, 0x9F);
  pulse.write(2, 0x40);
  for (const auto output : pulseSteps(pulse, 0x40, 16)) EXPECT_EQ(output, 0) << "没有长度计数器";

  enableWithLength(pulse, 1, 3);
  pulse.write(0, 0xBF);
  bool anyHigh = false;
  for (const auto output : pulseSteps(pulse, 0x40, 16)) anyHigh = anyHigh || output != 0;
  EXPECT_TRUE(anyHigh);

  pulse.write(2, 0x07);
  for (const auto output : pulseSteps(pulse, 7, 16)) EXPECT_EQ(output, 0) << "周期小于 8";
}

TEST(ApuPulse, SweepRaisesOrLowersThePeriodAndTheTwoChannelsDifferInNegateByOne) {
  auto sweepOnce = [](bool first, std::uint8_t sweep) {
    apu::PulseChannel pulse(first);
    pulse.write(2, 0x00);
    pulse.write(3, 0x01);  // 周期 0x100
    pulse.write(1, sweep);
    pulse.clockSweep();
    return pulse.period();
  };
  EXPECT_EQ(sweepOnce(true, 0x81), 0x180U) << "向上：加上 period >> 1";
  EXPECT_EQ(sweepOnce(false, 0x81), 0x180U);
  EXPECT_EQ(sweepOnce(true, 0x89), 0x7FU) << "方波 1 的负向扫频用反码，多减 1";
  EXPECT_EQ(sweepOnce(false, 0x89), 0x80U);
  EXPECT_EQ(sweepOnce(true, 0x01), 0x100U) << "没有使能时不改变周期";
  EXPECT_EQ(sweepOnce(true, 0x80), 0x100U) << "移位为 0 时不改变周期";
}

TEST(ApuPulse, SweepOverflowMutesTheChannelEvenWhenTheSweepIsDisabled) {
  apu::PulseChannel pulse(false);
  enableWithLength(pulse, 1, 3);
  pulse.write(0, 0xBF);
  pulse.write(2, 0x00);
  pulse.write(3, 0x04);  // 周期 0x400
  pulse.write(1, 0x00);  // 向上、移位 0：目标周期 0x800 超出 0x7FF
  pulse.endCycle();
  for (const auto output : pulseSteps(pulse, 0x400, 16)) EXPECT_EQ(output, 0);
  pulse.write(1, 0x08);  // 负向：目标周期小于当前周期，不静音
  bool anyHigh = false;
  for (const auto output : pulseSteps(pulse, 0x400, 16)) anyHigh = anyHigh || output != 0;
  EXPECT_TRUE(anyHigh);
}

TEST(ApuTriangle, FollowsTheSequenceOnlyWhileBothCountersAreNonZero) {
  apu::TriangleChannel triangle;
  triangle.length().setEnabled(true);
  triangle.write(0, 0x04);  // 控制位为 0，线性计数器重载值 4
  triangle.write(2, 0x02);  // 周期 2：每 3 个 CPU 周期一步
  triangle.write(3, 0x18);  // 长度 2，线性计数器重载标志
  triangle.endCycle();
  EXPECT_EQ(triangle.output(), 15) << "序列从 15 开始";
  for (int i = 0; i < 12; ++i) triangle.clockTimer();
  EXPECT_EQ(triangle.output(), 15) << "线性计数器还是 0，序列不动";

  triangle.clockLinear();  // 重载为 4
  for (int i = 0; i < 3; ++i) triangle.clockTimer();
  EXPECT_EQ(triangle.output(), 14);
  for (int i = 0; i < 3; ++i) triangle.clockTimer();
  EXPECT_EQ(triangle.output(), 13);

  for (int i = 0; i < 4; ++i) triangle.clockLinear();  // 4 → 0
  const int held = triangle.output();
  for (int i = 0; i < 30; ++i) triangle.clockTimer();
  EXPECT_EQ(triangle.output(), held) << "线性计数器为 0 时输出保持";
}

TEST(ApuTriangle, UltrasonicPeriodsDoNotAdvanceTheSequence) {
  apu::TriangleChannel triangle;
  triangle.length().setEnabled(true);
  triangle.write(0, 0xFF);
  triangle.write(2, 0x01);
  triangle.write(3, 0x08);
  triangle.endCycle();
  triangle.clockLinear();
  const int before = triangle.output();
  for (int i = 0; i < 200; ++i) triangle.clockTimer();
  EXPECT_EQ(triangle.output(), before);
}

/** 参考的噪声移位寄存器：每次时钟后返回位 0。 */
class ReferenceNoise {
 public:
  explicit ReferenceNoise(bool shortMode) : shortMode_(shortMode) {}
  unsigned step() {
    const unsigned feedback = (shift_ & 1U) ^ ((shift_ >> (shortMode_ ? 6U : 1U)) & 1U);
    shift_ = static_cast<std::uint16_t>((shift_ >> 1) | (feedback << 14));
    return shift_ & 1U;
  }

 private:
  bool shortMode_;
  std::uint16_t shift_{1};
};

TEST(ApuNoise, TheShiftRegisterFollowsTheLongAndShortModeSequences) {
  for (const bool shortMode : {false, true}) {
    apu::NoiseChannel noise;
    enableWithLength(noise, 1, 3);
    noise.write(0, 0x3F);  // 暂停位、固定音量 15
    noise.write(2, static_cast<std::uint8_t>(shortMode ? 0x80 : 0x00));  // 周期表第 0 项：4 个 CPU 周期
    noise.write(3, 0x08);
    noise.endCycle();
    // 之前的写入没有推进定时器，所以第一次移位在第 4 次 clockTimer。
    ReferenceNoise reference(shortMode);
    for (int step = 0; step < 400; ++step) {
      for (int i = 0; i < 4; ++i) noise.clockTimer();
      const unsigned bit = reference.step();
      EXPECT_EQ(noise.output(), bit == 0 ? 15 : 0) << (shortMode ? "短模式" : "长模式") << " 第 " << step << " 步";
    }
  }
}

TEST(ApuNoise, PeriodTableSetsTheShiftRate) {
  for (unsigned index = 0; index < 16; ++index) {
    apu::NoiseChannel noise;
    enableWithLength(noise, 1, 3);
    noise.write(0, 0x3F);
    noise.write(2, static_cast<std::uint8_t>(index));
    noise.write(3, 0x08);
    noise.endCycle();
    // 周期寄存器的新值在下一次定时器到期后才用：先跑过第一次到期，再数两次到期之间的周期数。
    for (unsigned i = 0; i < 4; ++i) noise.clockTimer();
    ReferenceNoise reference(false);
    reference.step();
    std::vector<unsigned> expectedBits;
    for (int i = 0; i < 30; ++i) expectedBits.push_back(reference.step());
    for (int step = 0; step < 30; ++step) {
      for (unsigned i = 0; i < apu::NOISE_PERIOD_TABLE[index]; ++i) noise.clockTimer();
      EXPECT_EQ(noise.output(), expectedBits[static_cast<std::size_t>(step)] == 0 ? 15 : 0) << "周期表第 " << index << " 项，第 " << step << " 步";
    }
  }
}

// ---------------------------------------------------------------- DMC 通道

/** 测量 DMC 在速率 index 下，字节 0xFF 的各位使输出电平连续上升的间隔（CPU 周期）。 */
std::vector<int> dmcRiseIntervals(unsigned index) {
  apu::DmcChannel dmc;
  dmc.reset(true);
  dmc.write(0, static_cast<std::uint8_t>(index));
  dmc.write(2, 0x00);
  dmc.write(3, 0xFF);
  dmc.setEnabled(true, 0);
  EXPECT_TRUE(dmc.takeDmaRequest());
  dmc.deliverSample(0xFF);
  std::vector<int> intervals;
  int last = -1;
  const int limit = static_cast<int>(apu::DMC_RATE_TABLE[0]) * 20;
  for (int cycle = 0; cycle < limit; ++cycle) {
    const auto before = dmc.output();
    dmc.clock();
    if (dmc.output() > before) {
      if (last >= 0) intervals.push_back(cycle - last);
      last = cycle;
    }
  }
  return intervals;
}

TEST(ApuDmcChannel, TheSixteenRatesSpaceTheOutputStepsByTheTable) {
  for (unsigned index = 0; index < 16; ++index) {
    const auto intervals = dmcRiseIntervals(index);
    ASSERT_GE(intervals.size(), 5U) << "速率 " << index;
    for (const int interval : intervals) EXPECT_EQ(interval, apu::DMC_RATE_TABLE[index]) << "速率 " << index;
  }
}

TEST(ApuDmcChannel, DirectLoadSetsTheOutputLevelAndSamplesEndWithLoopOrIrq) {
  apu::DmcChannel dmc;
  dmc.reset(true);
  dmc.write(1, 0xFF);
  EXPECT_EQ(dmc.output(), 0x7F) << "$4011 只用低 7 位";
  dmc.write(1, 0x23);
  EXPECT_EQ(dmc.output(), 0x23);

  // 地址与长度：$4012 → $C000 + 64×值，$4013 → 16×值 + 1。
  dmc.write(2, 0x03);
  dmc.write(3, 0x00);
  dmc.write(0, 0x80);  // 使能 IRQ，不循环
  dmc.setEnabled(true, 0);
  ASSERT_TRUE(dmc.takeDmaRequest());
  EXPECT_EQ(dmc.dmaAddress(), 0xC0C0);
  EXPECT_TRUE(dmc.active());
  dmc.deliverSample(0x55);
  EXPECT_FALSE(dmc.active()) << "只有 1 个字节";
  EXPECT_TRUE(dmc.irqFlag());
  dmc.write(0, 0x00);
  EXPECT_FALSE(dmc.irqFlag()) << "关闭 IRQ 使能清除标志";

  // 循环：采样结束时重新开始，不置 IRQ。
  dmc.write(0, 0xC0);
  dmc.setEnabled(true, 0);
  ASSERT_FALSE(dmc.takeDmaRequest()) << "缓冲已满，不需要取样";
  for (int i = 0; i < 428 * 8; ++i) dmc.clock();
  ASSERT_TRUE(dmc.takeDmaRequest()) << "缓冲被移位寄存器取走后要取下一个字节";
  dmc.deliverSample(0xAA);
  EXPECT_TRUE(dmc.active()) << "循环的采样结束后重新装入地址与长度";
  EXPECT_FALSE(dmc.irqFlag());

  // 地址在 $FFFF 之后回到 $8000。
  apu::DmcChannel wrap;
  wrap.reset(true);
  wrap.write(2, 0xFF);  // $FFC0
  wrap.write(3, 0x04);  // 65 个字节
  wrap.setEnabled(true, 0);
  std::uint16_t last = 0;
  for (int i = 0; i < 65; ++i) {
    ASSERT_TRUE(wrap.takeDmaRequest()) << i;
    last = wrap.dmaAddress();
    wrap.deliverSample(0);
    for (int cycle = 0; cycle < 428 * 8; ++cycle) wrap.clock();
  }
  EXPECT_EQ(last, 0x8000 + (0xFFC0 + 64 - 0x10000)) << "第 65 个字节的地址已经绕回";
}

// ---------------------------------------------------------------- 整个 APU

/** 采样内存：$8000-$FFFF，每个字节的值由 poke 设置，默认为 0。 */
class SampleMemory final : public hw::BusDevice {
 public:
  SampleMemory() : bytes_(0x8000, 0) {}
  hw::BusRead read(std::uint16_t address) override { return hw::driveBus(bytes_[static_cast<std::size_t>(address - 0x8000)]); }
  void write(std::uint16_t, std::uint8_t) override {}
  void poke(std::uint16_t address, std::uint8_t value) { bytes_[static_cast<std::size_t>(address - 0x8000)] = value; }

 private:
  std::vector<std::uint8_t> bytes_;
};

/** APU 的最小测试台；时间按 CPU 周期推进（一个周期 12 个主时钟周期）。 */
struct ApuRig {
  ApuRig() : apu(bus, scheduler, irq, rdy, hw::ClockDomain{12}, CPU_HZ) {
    bus.mount(0x4000, 0x4013, apu);
    bus.mount(0x4015, 0x4015, apu);
    bus.mount(0x4017, 0x4017, apu);
    bus.mount(0x8000, 0xFFFF, memory);
    apu.reset(true);
  }

  /** 当前所在的 CPU 周期编号（下一次访问发生在这个周期里）。 */
  [[nodiscard]] std::int64_t cycle() const { return scheduler.now() / 12; }

  /** 空过若干个 CPU 周期。 */
  void idle(std::int64_t cycles) { scheduler.advance(12 * cycles); }

  /** 在一个 CPU 周期里写一次寄存器（访问在周期的中点）。 */
  void write(std::uint16_t address, std::uint8_t value) {
    scheduler.advance(6);
    bus.write(address, value);
    scheduler.advance(6);
  }

  /** 在一个 CPU 周期里读一次寄存器。 */
  std::uint8_t read(std::uint16_t address) {
    scheduler.advance(6);
    const std::uint8_t value = bus.read(address);
    scheduler.advance(6);
    return value;
  }

  /** 逐个周期推进，直到 IRQ 线有效或超过上限；返回有效时的周期边界编号，超限返回 -1。 */
  std::int64_t runUntilIrq(std::int64_t limitCycle) {
    while (cycle() < limitCycle) {
      idle(1);
      if (irq.active()) return cycle();
    }
    return -1;
  }

  ember::logic::hardware::MasterScheduler scheduler;
  ember::logic::hardware::CpuAddressBus bus;
  ember::logic::hardware::WiredOrIrqLine irq;
  ember::logic::hardware::WiredOrRdyLine rdy;
  SampleMemory memory;
  Apu2a03 apu;
};

TEST(Apu2a03, StatusReadsZeroAtPowerUpAndDoesNotDriveBitFive) {
  ApuRig rig;
  const hw::BusRead status = rig.apu.read(0x4015);
  EXPECT_EQ(status.value & 0x1F, 0);
  EXPECT_EQ(status.drivenMask, 0xDF);
  EXPECT_FALSE(rig.apu.read(0x4000).drivenMask != 0) << "其他寄存器读不驱动数据线";
  EXPECT_EQ(rig.apu.read(0x4013).drivenMask, 0);
}

TEST(Apu2a03, PowerOnStartsTheFrameCounterInFourStepModeAndSetsTheFrameIrq) {
  ApuRig rig;
  const auto when = rig.runUntilIrq(40000);
  // 上电如同在复位序列开始时写入 $00：第 2 个 APU 周期生效，之后 29828 个周期置位帧 IRQ。
  EXPECT_EQ(when, 2 + 29828);
  EXPECT_EQ(rig.read(0x4015) & 0x40, 0x40);
}

TEST(Apu2a03, FrameIrqAppearsThe29831stCycleAfterAWriteWhetherTheWriteIsOnAnEvenOrOddCycle) {
  for (const std::int64_t writeCycle : {99, 100, 101, 102}) {
    ApuRig rig;
    rig.idle(writeCycle);
    rig.write(0x4017, 0x00);
    rig.read(0x4015);  // 清除上电时可能已有的标志（此时还没到）
    const auto when = rig.runUntilIrq(writeCycle + 40000);
    // 写在奇数 APU 周期生效推迟一个周期，下一个周期的写入又早一个周期，两者的结果相同。
    const std::int64_t apuCycle = writeCycle + 1;
    const std::int64_t delay = (apuCycle & 1) != 0 ? 4 : 3;
    EXPECT_EQ(when, apuCycle + delay + 29828) << "写入周期 " << writeCycle;
  }
}

TEST(Apu2a03, InhibitFlagAndFiveStepModeNeverSetTheFrameIrq) {
  for (const std::uint8_t value : std::initializer_list<std::uint8_t>{0x40, 0x80, 0xC0}) {
    ApuRig rig;
    rig.write(0x4017, value);
    rig.read(0x4015);
    EXPECT_EQ(rig.runUntilIrq(100000), -1) << "写入 " << static_cast<int>(value);
    EXPECT_EQ(rig.read(0x4015) & 0x40, 0);
  }
}

TEST(Apu2a03, ReadingTheStatusClearsTheFrameIrqAndWritingTheInhibitFlagClearsItToo) {
  ApuRig rig;
  ASSERT_GT(rig.runUntilIrq(40000), 0);
  // 标志会连续三个周期被置位，等它们过去再读。
  rig.idle(10);
  EXPECT_EQ(rig.read(0x4015) & 0x40, 0x40);
  EXPECT_EQ(rig.read(0x4015) & 0x40, 0) << "读一次就清除";
  EXPECT_FALSE(rig.irq.active());

  ASSERT_GT(rig.runUntilIrq(rig.cycle() + 60000), 0);
  rig.write(0x4017, 0x40);
  EXPECT_FALSE(rig.irq.active()) << "写入禁止位立即清除标志";
}

TEST(Apu2a03, WritingTheFrameCounterRestartsTheSequenceSoLengthClocksMoveWithIt) {
  ApuRig rig;
  rig.write(0x4015, 0x01);
  rig.write(0x4003, 0x18);  // 长度 2
  rig.idle(2);
  EXPECT_EQ(rig.read(0x4015) & 0x01, 1);
  // 不重新开始的话，半帧时钟在第 14915、29831 个周期，长度 2 会在 29831 之前用完。
  rig.idle(14000 - rig.cycle());
  rig.write(0x4017, 0x00);
  const auto restart = rig.cycle();
  rig.idle(40000 - rig.cycle());
  EXPECT_EQ(rig.read(0x4015) & 0x01, 1) << "重新开始后第一个半帧在 14913 个周期之后，第二个在 29829 之后";
  rig.idle(restart + 29829 + 100 - rig.cycle());
  EXPECT_EQ(rig.read(0x4015) & 0x01, 0);
}

TEST(Apu2a03, FiveStepModeClocksTheLengthCountersImmediatelyButFourStepModeDoesNot) {
  ApuRig rig;
  rig.write(0x4015, 0x01);
  rig.write(0x4003, 0x18);  // 长度 2
  rig.write(0x4017, 0x00);
  rig.idle(10);
  EXPECT_EQ(rig.read(0x4015) & 0x01, 1);
  rig.write(0x4017, 0x80);
  rig.idle(10);
  EXPECT_EQ(rig.read(0x4015) & 0x01, 1) << "第一次时钟：2 → 1";
  rig.write(0x4017, 0x80);
  rig.idle(10);
  EXPECT_EQ(rig.read(0x4015) & 0x01, 0) << "第二次时钟：1 → 0";
}

TEST(Apu2a03, StatusShowsEachChannelsLengthAndDisablingClearsIt) {
  ApuRig rig;
  rig.write(0x4015, 0x0F);
  rig.write(0x4003, 0x08);
  rig.write(0x4007, 0x08);
  rig.write(0x400B, 0x08);
  rig.write(0x400F, 0x08);
  rig.idle(3);
  EXPECT_EQ(rig.read(0x4015) & 0x0F, 0x0F);
  rig.write(0x4015, 0x05);
  EXPECT_EQ(rig.read(0x4015) & 0x0F, 0x05) << "关闭的通道长度清零";
  rig.write(0x4007, 0x08);
  rig.idle(3);
  EXPECT_EQ(rig.read(0x4015) & 0x02, 0) << "关闭期间载入无效";
}

TEST(Apu2a03, HaltedChannelsKeepTheirLength) {
  ApuRig rig;
  rig.write(0x4015, 0x01);
  rig.write(0x4000, 0x20);  // 暂停位
  rig.write(0x4003, 0x18);
  rig.idle(70000);
  EXPECT_EQ(rig.read(0x4015) & 0x01, 1);
}

/** 记录一段时间里 APU 的混音电平随周期的变化次数与取值。 */
struct LevelTrace {
  std::vector<double> levels;
  [[nodiscard]] int transitions() const {
    int count = 0;
    for (std::size_t i = 1; i < levels.size(); ++i) count += std::abs(levels[i] - levels[i - 1]) > 1e-9 ? 1 : 0;
    return count;
  }
  [[nodiscard]] double minimum() const { return *std::min_element(levels.begin(), levels.end()); }
  [[nodiscard]] double maximum() const { return *std::max_element(levels.begin(), levels.end()); }
};

LevelTrace traceLevel(ApuRig& rig, std::int64_t cycles) {
  LevelTrace trace;
  for (std::int64_t i = 0; i < cycles; ++i) {
    rig.idle(1);
    rig.apu.catchUp(rig.scheduler.now());
    trace.levels.push_back(rig.apu.mixLevel());
  }
  return trace;
}

double pulseLevel(unsigned total) { return 95.88 / (8128.0 / total + 100.0); }
double tndLevel(unsigned triangle, unsigned noise, unsigned dmc) {
  const double sum = triangle / 8227.0 + noise / 12241.0 + dmc / 22638.0;
  return sum <= 0 ? 0.0 : 159.79 / (1.0 / sum + 100.0);
}

TEST(Apu2a03, MixLevelFollowsTheNonlinearMixingFormula) {
  ApuRig rig;
  const double idle = tndLevel(15, 0, 0);  // 三角波停在序列起点 15，其余为 0
  rig.idle(20);
  rig.apu.catchUp(rig.scheduler.now());
  EXPECT_NEAR(rig.apu.mixLevel(), idle, 1e-12);

  rig.write(0x4015, 0x03);
  for (const std::uint16_t base : std::initializer_list<std::uint16_t>{0x4000, 0x4004}) {
    rig.write(base, 0xBF);      // 占空比 50%、暂停、固定音量 15
    rig.write(base + 2, 0x20);  // 周期 0x20
    rig.write(base + 3, 0x08);
  }
  const auto trace = traceLevel(rig, 3000);
  EXPECT_NEAR(trace.minimum(), idle, 1e-12);
  EXPECT_NEAR(trace.maximum(), pulseLevel(30) + idle, 1e-12) << "两个方波同时为高：音量 15 + 15 = 30";
}

TEST(Apu2a03, PulseWaveTogglesAtTheRateTheTimerPeriodGives) {
  ApuRig rig;
  rig.write(0x4015, 0x01);
  rig.write(0x4000, 0xBF);   // 占空比 50%：序列 0 1 1 1 1 0 0 0
  rig.write(0x4002, 0x00);
  rig.write(0x4003, 0x09);   // 周期 0x100，长度索引 1
  const auto trace = traceLevel(rig, 8 * 514 * 5);
  // 每个序列周期 8 步 × 2×(0x100+1) = 4112 个周期，有 2 次电平变化。
  EXPECT_NEAR(trace.transitions(), 10, 1);
}

TEST(Apu2a03, DmcDmaStallsTheCpuForFourCyclesAndDeliversTheSample) {
  ApuRig rig;
  rig.memory.poke(0xC000, 0xFF);
  rig.write(0x4010, 0x8F);  // 使能 IRQ，最快速率
  rig.write(0x4012, 0x00);  // $C000
  rig.write(0x4013, 0x00);  // 1 个字节
  rig.write(0x4015, 0x10);
  int stalledCycles = 0;
  int firstStall = -1;
  for (int cycle = 0; cycle < 20; ++cycle) {
    rig.idle(1);
    if (rig.rdy.active()) {
      if (firstStall < 0) firstStall = cycle;
      ++stalledCycles;
    }
  }
  EXPECT_EQ(stalledCycles, 4) << "取样请求之后 CPU 被暂停 4 个周期";
  EXPECT_GE(firstStall, 1);
  EXPECT_LE(firstStall, 5) << "$4015 写入后 2 到 3 个周期内发出请求";
  EXPECT_FALSE(rig.rdy.active());
  EXPECT_TRUE(rig.apu.dmcIrq()) << "最后一个字节取完时置 DMC IRQ";
  EXPECT_TRUE(rig.irq.active());
  EXPECT_EQ(rig.read(0x4015) & 0x90, 0x80) << "字节已取走，位 4 清除；位 7 为 IRQ";
  EXPECT_EQ(rig.read(0x4015) & 0x80, 0x80) << "读 $4015 不清除 DMC 的 IRQ";
  rig.write(0x4015, 0x00);
  EXPECT_EQ(rig.read(0x4015) & 0x80, 0) << "写 $4015 清除 DMC 的 IRQ";
  EXPECT_FALSE(rig.irq.active());
}

TEST(Apu2a03, DmcKeepsFetchingWhileTheSampleLastsAndStopsAtTheEnd) {
  ApuRig rig;
  rig.write(0x4010, 0x0F);  // 54 个周期一位，不使能 IRQ
  rig.write(0x4012, 0x00);
  rig.write(0x4013, 0x01);  // 17 个字节
  rig.write(0x4015, 0x10);
  int stalls = 0;
  bool wasActive = false;
  for (int cycle = 0; cycle < 54 * 8 * 20; ++cycle) {
    rig.idle(1);
    const bool active = rig.rdy.active();
    if (active && !wasActive) ++stalls;
    wasActive = active;
  }
  EXPECT_EQ(stalls, 17);
  EXPECT_EQ(rig.read(0x4015) & 0x10, 0);
  EXPECT_FALSE(rig.irq.active() && rig.apu.dmcIrq());
}

TEST(Apu2a03, DmcDirectLoadChangesTheMixedOutput) {
  ApuRig rig;
  rig.idle(20);
  rig.apu.catchUp(rig.scheduler.now());
  const double before = rig.apu.mixLevel();
  rig.write(0x4011, 0x40);
  rig.idle(3);
  rig.apu.catchUp(rig.scheduler.now());
  EXPECT_NEAR(rig.apu.mixLevel(), tndLevel(15, 0, 64), 1e-12);
  EXPECT_GT(rig.apu.mixLevel(), before);
}

TEST(Apu2a03, SilenceWithoutAnyInputAndSampleCountFollowsTheClockRatio) {
  ApuRig rig;
  rig.idle(1'789'773);  // 约 1 秒
  rig.apu.catchUp(rig.scheduler.now());
  std::vector<float> samples;
  rig.apu.takeAudio(samples);
  // 48000 / 1789772.67 的比例，带限滤波有 8 个采样的固定延迟。
  EXPECT_NEAR(static_cast<double>(samples.size()), 48000.0 - apu::BandLimitedSampler::HALF_WIDTH, 2.0);
  for (const float sample : samples) ASSERT_EQ(sample, 0.0F);

  rig.apu.takeAudio(samples);
  EXPECT_TRUE(samples.empty()) << "取走之后清空";
}

TEST(Apu2a03, OutputStaysInRangeAndHasSignalUnderHeavyRegisterActivity) {
  ApuRig rig;
  rig.memory.poke(0xC000, 0xAA);
  rig.write(0x4015, 0x1F);
  std::uint32_t seed = 12345;
  auto next = [&seed] {
    seed = seed * 1664525U + 1013904223U;
    return static_cast<std::uint8_t>(seed >> 24);
  };
  for (int i = 0; i < 6000; ++i) {
    const std::uint16_t address = static_cast<std::uint16_t>(0x4000 + next() % 0x14);
    rig.write(address, next());
    rig.idle(1 + next() % 40);
    if (i % 300 == 0) rig.write(0x4015, 0x1F);
  }
  rig.apu.catchUp(rig.scheduler.now());
  std::vector<float> samples;
  rig.apu.takeAudio(samples);
  ASSERT_GT(samples.size(), 1000U);
  float peak = 0;
  for (const float sample : samples) {
    ASSERT_TRUE(std::isfinite(sample));
    ASSERT_LE(std::abs(sample), 1.0F);
    peak = std::max(peak, std::abs(sample));
  }
  EXPECT_GT(peak, 0.01F);
}

TEST(Apu2a03, SoftResetClearsTheStatusAndKeepsTheFrameCounterMode) {
  ApuRig rig;
  rig.write(0x4015, 0x0F);
  rig.write(0x4003, 0x08);
  rig.write(0x4017, 0x80);  // 5 步模式
  rig.idle(100);
  ASSERT_EQ(rig.read(0x4015) & 0x01, 1);
  rig.apu.reset(false);
  EXPECT_EQ(rig.read(0x4015) & 0x1F, 0) << "软复位如同写 $4015 为 0";
  rig.read(0x4015);
  EXPECT_EQ(rig.runUntilIrq(rig.cycle() + 100000), -1) << "5 步模式保持，不会置帧 IRQ";
}

TEST(Apu2a03, PowerOnResetCancelsAnInFlightDmcDma) {
  ApuRig rig;
  rig.write(0x4012, 0x00);
  rig.write(0x4013, 0x01);
  rig.write(0x4015, 0x10);
  rig.idle(3);
  ASSERT_TRUE(rig.rdy.active());
  rig.apu.reset(true);
  EXPECT_FALSE(rig.rdy.active());
  rig.idle(100);
  EXPECT_FALSE(rig.rdy.active()) << "复位之后不再有遗留的事件";
}

// ---------------------------------------------------------------- 带限采样器

TEST(BandLimitedSampler, OutputsNothingButSilenceWithoutDeltasAndCountsSamplesByTheRatio) {
  apu::BandLimitedSampler sampler(CPU_HZ, 48000);
  std::vector<float> out;
  for (std::int64_t tick = 1; tick <= 178'977; ++tick) sampler.advance(tick, out);
  // 0.1 秒：4800 个采样减去 8 个采样的延迟（+1，因为样本从第 0 个算起）。
  EXPECT_NEAR(static_cast<double>(out.size()), 4800.0 - apu::BandLimitedSampler::HALF_WIDTH, 2.0);
  EXPECT_EQ(sampler.emittedSamples(), static_cast<std::int64_t>(out.size()));
  for (const float sample : out) ASSERT_EQ(sample, 0.0F);
}

TEST(BandLimitedSampler, AStepSettlesAtTheDeltaThenTheDcBlockerRemovesIt) {
  apu::BandLimitedSampler sampler(CPU_HZ, 48000);
  std::vector<float> out;
  const std::int64_t stepTick = 5000;
  for (std::int64_t tick = 1; tick <= 3'000'000; ++tick) {
    if (tick == stepTick) sampler.addDelta(tick, 0.5);
    sampler.advance(tick, out);
  }
  const std::size_t stepSample = static_cast<std::size_t>(std::floor(static_cast<double>(stepTick) * 48000.0 / CPU_HZ));
  float peak = 0;
  float settled = 0;
  for (std::size_t i = stepSample; i < stepSample + 24; ++i) peak = std::max(peak, out[i]);
  for (std::size_t i = stepSample + 24; i < stepSample + 44; ++i) settled = std::max(settled, out[i]);
  // 带限滤波的阶跃响应有 10% 左右的过冲（振铃在 20 kHz 附近，听不到），脉冲响应结束后回到变化量，之后被直流阻断缓慢拉回 0。
  EXPECT_LT(peak, 0.5F * 1.15F);
  EXPECT_GT(peak, 0.5F);
  EXPECT_NEAR(settled, 0.5F, 0.05F) << "阶跃之后到达变化量";
  for (std::size_t i = 0; i + 1 < stepSample - apu::BandLimitedSampler::HALF_WIDTH - 2; ++i) ASSERT_EQ(out[i], 0.0F) << "脉冲响应之前没有输出";
  EXPECT_LT(std::abs(out[stepSample + 4800]), 0.01F) << "100 毫秒后直流已被阻断";
  EXPECT_LT(std::abs(out.back()), 1e-6F);
}

/** 用方波（每 halfPeriodTicks 个周期翻转一次，幅度 ±0.5）驱动采样器，返回稳定后的均方根。 */
double squareWaveRms(std::int64_t halfPeriodTicks) {
  apu::BandLimitedSampler sampler(CPU_HZ, 48000);
  std::vector<float> out;
  double level = 0;
  const std::int64_t total = 900'000;
  for (std::int64_t tick = 1; tick <= total; ++tick) {
    if (tick % halfPeriodTicks == 0) {
      const double target = level > 0 ? -0.5 : 0.5;
      sampler.addDelta(tick, target - level);
      level = target;
    }
    sampler.advance(tick, out);
  }
  double sum = 0;
  std::size_t count = 0;
  for (std::size_t i = out.size() / 2; i < out.size(); ++i, ++count) sum += static_cast<double>(out[i]) * out[i];
  return std::sqrt(sum / static_cast<double>(count));
}

TEST(BandLimitedSampler, FrequenciesAboveNyquistAreRemovedInsteadOfFoldingBackAsHiss) {
  // 每 298 个周期翻转：约 3.0 kHz；每 30 个周期翻转：约 29.8 kHz，高于奈奎斯特频率（24 kHz）。
  const double audible = squareWaveRms(298);
  const double ultrasonic = squareWaveRms(30);
  EXPECT_GT(audible, 0.3);
  // 不做带限（直接按采样点取值）时，29.8 kHz 会混叠成 18.2 kHz 的响亮音调；带限滤波把它压到可闻分量的 2% 以下（-34 dB）。
  EXPECT_LT(ultrasonic, audible * 0.02);
}

TEST(BandLimitedSampler, RejectsInvalidRates) {
  EXPECT_THROW(apu::BandLimitedSampler(0.0, 48000), ember::shared::CoreError);
  EXPECT_THROW(apu::BandLimitedSampler(CPU_HZ, 0), ember::shared::CoreError);
  EXPECT_THROW(apu::BandLimitedSampler(48000.0, 48000), ember::shared::CoreError);
}

}  // namespace
