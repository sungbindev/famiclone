/*
 * ------------------------------------------------------------------------
 * 名称：um6576-registers-test.cpp
 * 说明：6576 芯片组的小型功能模块测试：初始设置、中断屏蔽与状态、定时器、实时时钟、程序存储器分页、扩展 I/O、DAC。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：每个模块单独接测试台，不用 CPU；定时器靠调度器推进时间。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include "contracts/hardware/interrupt-sink.hpp"
#include "contracts/hardware/parallel-port.hpp"
#include "logic/hardware/dac-um6576/dac-um6576.hpp"
#include "logic/hardware/expio-um6576/expio-um6576.hpp"
#include "logic/hardware/irq-um6576/irq-um6576.hpp"
#include "logic/hardware/lines/wired-or-line.hpp"
#include "logic/hardware/paging-um6576/paging-um6576.hpp"
#include "logic/hardware/rtc-um6576/rtc-um6576.hpp"
#include "logic/hardware/scheduler/master-scheduler.hpp"
#include "logic/hardware/sysinit-um6576/sysinit-um6576.hpp"
#include "logic/hardware/timer-um6576/timer-um6576.hpp"

namespace {

namespace hw = ember::contracts::hardware;
using ember::logic::hardware::DacUm6576;
using ember::logic::hardware::ExpioUm6576;
using ember::logic::hardware::IrqUm6576;
using ember::logic::hardware::MasterScheduler;
using ember::logic::hardware::PagingUm6576;
using ember::logic::hardware::RtcUm6576;
using ember::logic::hardware::SysinitUm6576;
using ember::logic::hardware::TimerUm6576;
using ember::logic::hardware::WiredOrIrqLine;

/** 记录请求的中断汇集器。 */
class RecordingSink final : public hw::InterruptSink {
 public:
  void raise(std::uint8_t sourceMask) override { raised.push_back(sourceMask); }
  std::vector<std::uint8_t> raised;
};

/** 可以设置状态线电平、记录输出线变化的并口。 */
class FakeParallelPort final : public hw::ParallelPort {
 public:
  void writeData(std::uint8_t value) override { data = value; }
  void setStrobeN(bool high) override { strobeN = high; }
  void setSelectPrinterN(bool high) override { selectN = high; }
  void setLineFeedN(bool high) override { lineFeedN = high; }
  void setReset(bool high) override { resetLine = high; }
  [[nodiscard]] bool ack() const override { return ackLevel; }
  [[nodiscard]] bool paperOut() const override { return paperOutLevel; }
  [[nodiscard]] bool select() const override { return selectLevel; }
  [[nodiscard]] bool error() const override { return false; }
  [[nodiscard]] bool busyN() const override { return true; }
  void reset() override {}

  std::uint8_t data{0};
  bool strobeN{true};
  bool selectN{true};
  bool lineFeedN{true};
  bool resetLine{false};
  bool ackLevel{true};
  bool paperOutLevel{true};
  bool selectLevel{true};
};

TEST(SysinitUm6576, TwoKeysInOrderEnterTheMode) {
  SysinitUm6576 sysinit;
  EXPECT_FALSE(sysinit.enabled());
  sysinit.write(0x4031, 0x65);
  EXPECT_EQ(sysinit.state(), 1U);
  EXPECT_FALSE(sysinit.enabled());
  sysinit.write(0x4031, 0x76);
  EXPECT_TRUE(sysinit.enabled());
}

TEST(SysinitUm6576, AnyOtherValueInTheMiddleStartsOver) {
  SysinitUm6576 sysinit;
  sysinit.write(0x4031, 0x65);
  sysinit.write(0x4031, 0x65);
  sysinit.write(0x4031, 0x76);
  EXPECT_FALSE(sysinit.enabled()) << "第二次写入不是 $76，回到未进入的状态，随后的 $76 不起作用";
  sysinit.write(0x4031, 0x65);
  sysinit.write(0x4031, 0x76);
  EXPECT_TRUE(sysinit.enabled());
}

TEST(SysinitUm6576, PowerResetLeavesTheModeButSoftResetKeepsIt) {
  SysinitUm6576 sysinit;
  sysinit.write(0x4031, 0x65);
  sysinit.write(0x4031, 0x76);
  sysinit.reset(false);
  EXPECT_TRUE(sysinit.enabled());
  sysinit.reset(true);
  EXPECT_FALSE(sysinit.enabled());
}

TEST(IrqUm6576, AllSourcesAreMaskedAfterReset) {
  WiredOrIrqLine line;
  IrqUm6576 irq(line);
  irq.reset(true);
  EXPECT_EQ(irq.read(0x4032).value, 0xFF);
  irq.raise(IrqUm6576::SOURCE_TIMER);
  EXPECT_EQ(irq.read(0x4033).value, 0x00) << "被屏蔽的来源，请求被丢弃";
  EXPECT_FALSE(line.active());
}

TEST(IrqUm6576, UnmaskedRequestSetsStatusAndRaisesTheLine) {
  WiredOrIrqLine line;
  IrqUm6576 irq(line);
  irq.reset(true);
  irq.write(0x4032, 0x7F);
  irq.raise(IrqUm6576::SOURCE_TIMER);
  EXPECT_EQ(irq.read(0x4033).value, 0x80);
  EXPECT_TRUE(line.active());
  EXPECT_EQ(irq.read(0x4033).value, 0x80) << "读状态不清除它";
}

TEST(IrqUm6576, WritingTheMaskClearsTheMaskedStatusBits) {
  WiredOrIrqLine line;
  IrqUm6576 irq(line);
  irq.reset(true);
  irq.write(0x4032, 0x00);
  irq.raise(IrqUm6576::SOURCE_TIMER);
  irq.raise(IrqUm6576::SOURCE_KEYBOARD);
  EXPECT_EQ(irq.read(0x4033).value, 0xA0);
  irq.write(0x4032, 0x80);
  EXPECT_EQ(irq.read(0x4033).value, 0x20) << "屏蔽的位同时从状态里清除";
  EXPECT_TRUE(line.active()) << "还有待处理的来源，线保持有效";
  irq.write(0x4032, 0xA0);
  EXPECT_EQ(irq.read(0x4033).value, 0x00);
  EXPECT_FALSE(line.active());
}

TEST(IrqUm6576, StatusRegisterCannotBeWritten) {
  WiredOrIrqLine line;
  IrqUm6576 irq(line);
  irq.reset(true);
  irq.write(0x4032, 0x00);
  irq.raise(IrqUm6576::SOURCE_MOUSE);
  irq.write(0x4033, 0x00);
  EXPECT_EQ(irq.read(0x4033).value, 0x40);
  EXPECT_EQ(irq.read(0x4032).value, 0x00);
}

/** 定时器测试台：CPU 周期 15 个主时钟周期，扫描行 1705 个。 */
struct TimerBench {
  TimerBench() : timer(scheduler, sink, 15, 1705) { timer.reset(true); }
  MasterScheduler scheduler;
  RecordingSink sink;
  TimerUm6576 timer;
};

TEST(TimerUm6576, ContinuousTimerUnderflowsEveryPeriod) {
  TimerBench bench;
  bench.timer.write(0x4035, 2);
  bench.timer.write(0x4034, 0xC0);  // 允许、连续、CPU 周期、预分频 1
  EXPECT_EQ(bench.timer.periodTicks(), 45);
  bench.scheduler.advance(44);
  EXPECT_TRUE(bench.sink.raised.empty());
  bench.scheduler.advance(1);
  EXPECT_EQ(bench.sink.raised, std::vector<std::uint8_t>({0x80}));
  bench.scheduler.advance(90);
  EXPECT_EQ(bench.sink.raised.size(), 3U);
}

TEST(TimerUm6576, CurrentValueCountsDownBetweenReads) {
  TimerBench bench;
  bench.timer.write(0x4035, 5);
  bench.timer.write(0x4034, 0xC0);
  EXPECT_EQ(bench.timer.read(0x4036).value, 5);
  bench.scheduler.advance(30);
  EXPECT_EQ(bench.timer.read(0x4036).value, 3);
  EXPECT_EQ(bench.timer.read(0x4035).value, 5) << "$4035 读回预置值";
  bench.scheduler.advance(15 * 3);
  EXPECT_EQ(bench.timer.read(0x4036).value, 0);
  bench.scheduler.advance(15);
  EXPECT_EQ(bench.timer.read(0x4036).value, 5) << "下溢之后重载预置值";
}

TEST(TimerUm6576, OneShotTimerUnderflowsOnceAndHoldsThePreload) {
  TimerBench bench;
  bench.timer.write(0x4035, 3);
  bench.timer.write(0x4034, 0x80);  // 允许、只计一次
  bench.scheduler.advance(15 * 4);
  EXPECT_EQ(bench.sink.raised.size(), 1U);
  EXPECT_EQ(bench.timer.read(0x4036).value, 3);
  bench.scheduler.advance(15 * 100);
  EXPECT_EQ(bench.sink.raised.size(), 1U) << "下溢一次之后不再计时";
  EXPECT_EQ(bench.timer.read(0x4036).value, 3);
}

TEST(TimerUm6576, LineClockAndPrescaleSetThePeriod) {
  TimerBench bench;
  bench.timer.write(0x4035, 60);
  bench.timer.write(0x4034, 0xEF);  // BIOS 开机的设置：连续、行消隐时钟、预分频 16
  EXPECT_EQ(bench.timer.periodTicks(), 1705 * 16 * 61);
  bench.scheduler.advance(1705 * 16 * 61 - 1);
  EXPECT_TRUE(bench.sink.raised.empty());
  bench.scheduler.advance(1);
  EXPECT_EQ(bench.sink.raised.size(), 1U);
}

TEST(TimerUm6576, DisabledTimerDoesNotCount) {
  TimerBench bench;
  bench.timer.write(0x4035, 4);
  bench.timer.write(0x4034, 0x40);  // 没有允许
  bench.scheduler.advance(15 * 100);
  EXPECT_TRUE(bench.sink.raised.empty());
  EXPECT_EQ(bench.timer.read(0x4036).value, 4);
}

TEST(TimerUm6576, WritingTheControlKeepsTheCurrentValue) {
  TimerBench bench;
  bench.timer.write(0x4035, 10);
  bench.timer.write(0x4034, 0xC0);
  bench.scheduler.advance(15 * 4);
  EXPECT_EQ(bench.timer.read(0x4036).value, 6);
  bench.timer.write(0x4034, 0xC0);
  EXPECT_EQ(bench.timer.read(0x4036).value, 6) << "写控制字节不重载当前值";
  bench.scheduler.advance(15 * 7);
  EXPECT_EQ(bench.sink.raised.size(), 1U) << "当前值从 6 数到下溢共 7 个预分频周期";
}

TEST(TimerUm6576, ResetStopsTheTimer) {
  TimerBench bench;
  bench.timer.write(0x4035, 1);
  bench.timer.write(0x4034, 0xC0);
  bench.timer.reset(true);
  bench.scheduler.advance(1000);
  EXPECT_TRUE(bench.sink.raised.empty());
  EXPECT_EQ(bench.timer.read(0x4034).value, 0);
}

TEST(RtcUm6576, ReadsAlwaysGiveTheMeasuredConstant) {
  RtcUm6576 rtc;
  EXPECT_EQ(rtc.read(0x4081).value, 0x60);
  rtc.write(0x4080, 0x05);
  rtc.write(0x4081, 0x99);
  EXPECT_EQ(rtc.read(0x4081).value, 0x60);
  EXPECT_EQ(rtc.index(), 0x05);
  EXPECT_EQ(rtc.data(), 0x99);
}

TEST(PagingUm6576, WindowsStartOnTheFirstPages) {
  PagingUm6576 paging;
  for (unsigned window = 0; window < 8; ++window) EXPECT_EQ(paging.read(static_cast<std::uint16_t>(0x4040 + window)).value, window);
  const auto location = paging.resolve(0xC123);
  EXPECT_FALSE(location.ram);
  EXPECT_EQ(location.offset, 4U * 0x1000 + 0x123);
}

TEST(PagingUm6576, PagesFromEightyUpAreExtendedRam) {
  PagingUm6576 paging;
  paging.write(0x4040, 0x85);
  paging.write(0x4047, 0x7F);
  const auto ram = paging.resolve(0x8ABC);
  EXPECT_TRUE(ram.ram);
  EXPECT_EQ(ram.offset, 5U * 0x1000 + 0xABC);
  const auto rom = paging.resolve(0xFFFF);
  EXPECT_FALSE(rom.ram);
  EXPECT_EQ(rom.offset, 0x7FU * 0x1000 + 0xFFF);
  EXPECT_EQ(paging.read(0x4040).value, 0x85);
}

TEST(ExpioUm6576, StatusBitsAreTheWrittenValueAndedWithThePins) {
  FakeParallelPort port;
  ExpioUm6576 expio(port);
  expio.write(0x4026, 0x3F);
  EXPECT_EQ(expio.read(0x4026).value, 0x3F) << "位 3-5 来自引脚（全为高），其余是写入值";
  port.ackLevel = false;
  port.paperOutLevel = false;
  port.selectLevel = false;
  EXPECT_EQ(expio.read(0x4026).value, 0x07);
  expio.write(0x4026, 0x00);
  port.ackLevel = true;
  EXPECT_EQ(expio.read(0x4026).value, 0x00) << "写入为 0 的位读出为 0";
}

TEST(ExpioUm6576, StrobeBitFollowsEveryWrite) {
  FakeParallelPort port;
  ExpioUm6576 expio(port);
  expio.write(0x4026, 0x04);
  EXPECT_TRUE(port.strobeN);
  expio.write(0x4026, 0x00);
  EXPECT_FALSE(port.strobeN);
}

TEST(ExpioUm6576, RisingEdgesOfTheResetBitsNotifyTheMachine) {
  FakeParallelPort port;
  ExpioUm6576 expio(port);
  int mouse = 0;
  int speech = 0;
  expio.onMouseReset = [&] { ++mouse; };
  expio.onSpeechReset = [&] { ++speech; };
  expio.write(0x4026, 0x02);
  EXPECT_EQ(mouse, 1);
  expio.write(0x4026, 0x02);
  EXPECT_EQ(mouse, 1) << "电平没有变化不通知";
  expio.write(0x4026, 0x00);
  expio.write(0x4026, 0x40);
  EXPECT_EQ(speech, 1);
  EXPECT_EQ(mouse, 1);
}

TEST(ExpioUm6576, KeyboardResetBitIsNotKept) {
  FakeParallelPort port;
  ExpioUm6576 expio(port);
  expio.write(0x4026, 0xFF);
  EXPECT_EQ(expio.read(0x4026).value & 0x80, 0);
}

TEST(DacUm6576, WritesChangeTheOutputLevel) {
  DacUm6576 dac;
  std::vector<std::uint8_t> levels;
  dac.onLevel = [&](std::uint8_t level) { levels.push_back(level); };
  dac.write(0x4027, 0x80);
  dac.write(0x4027, 0x10);
  EXPECT_EQ(levels, std::vector<std::uint8_t>({0x80, 0x10}));
  EXPECT_EQ(dac.level(), 0x10);
  EXPECT_EQ(dac.read(0x4027).drivenMask, 0);
  dac.reset(true);
  EXPECT_EQ(dac.level(), 0);
}

}  // namespace
