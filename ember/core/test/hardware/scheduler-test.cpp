/*
 * ------------------------------------------------------------------------
 * 名称：scheduler-test.cpp
 * 说明：时钟与调度器的测试：事件按时间与登记顺序触发、回调时的时钟、取消、回调中登记事件、过去的事件、非法推进，以及时钟域的换算。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <functional>
#include <utility>
#include <vector>

#include "contracts/hardware/hardware-error.hpp"
#include "logic/hardware/scheduler/master-scheduler.hpp"
#include "shared/errors/core-error.hpp"

namespace {

namespace hw = ember::contracts::hardware;
using ember::logic::hardware::MasterScheduler;
using ember::shared::CoreError;

/** 记录收到的事件；可以在回调里执行一段动作。 */
class RecordingListener final : public hw::EventListener {
 public:
  explicit RecordingListener(MasterScheduler& scheduler) : scheduler_(scheduler) {}

  void onEvent(hw::MasterTime when, std::uint32_t tag) override {
    received.push_back({when, scheduler_.now(), tag});
    if (action) action(tag);
  }

  struct Received {
    hw::MasterTime when;
    hw::MasterTime clockNow;
    std::uint32_t tag;
  };
  std::vector<Received> received;
  std::function<void(std::uint32_t)> action;

 private:
  MasterScheduler& scheduler_;
};

std::vector<std::uint32_t> tagsOf(const RecordingListener& listener) {
  std::vector<std::uint32_t> tags;
  for (const auto& item : listener.received) tags.push_back(item.tag);
  return tags;
}

TEST(Scheduler, FiresEventsInTimeOrderAndAdvancesClockToTarget) {
  MasterScheduler scheduler;
  RecordingListener listener(scheduler);
  scheduler.scheduleAt(30, listener, 3);
  scheduler.scheduleAt(10, listener, 1);
  scheduler.scheduleAt(20, listener, 2);

  scheduler.advance(25);

  EXPECT_EQ(tagsOf(listener), (std::vector<std::uint32_t>{1, 2}));
  EXPECT_EQ(scheduler.now(), 25);
  EXPECT_EQ(scheduler.pendingCount(), 1U);

  scheduler.advance(5);
  EXPECT_EQ(tagsOf(listener), (std::vector<std::uint32_t>{1, 2, 3}));
  EXPECT_EQ(scheduler.now(), 30);
}

TEST(Scheduler, ClockEqualsEventTimeInsideCallback) {
  MasterScheduler scheduler;
  RecordingListener listener(scheduler);
  scheduler.scheduleAt(7, listener, 1);
  scheduler.scheduleAt(19, listener, 2);

  scheduler.advance(100);

  ASSERT_EQ(listener.received.size(), 2U);
  EXPECT_EQ(listener.received[0].when, 7);
  EXPECT_EQ(listener.received[0].clockNow, 7);
  EXPECT_EQ(listener.received[1].clockNow, 19);
  EXPECT_EQ(scheduler.now(), 100);
}

TEST(Scheduler, EventsAtTheSameTimeFireInRegistrationOrder) {
  MasterScheduler scheduler;
  RecordingListener listener(scheduler);
  scheduler.scheduleAt(5, listener, 10);
  scheduler.scheduleAt(5, listener, 11);
  scheduler.scheduleAt(5, listener, 12);

  scheduler.advance(5);

  EXPECT_EQ(tagsOf(listener), (std::vector<std::uint32_t>{10, 11, 12}));
}

TEST(Scheduler, EventExactlyAtTargetFiresButLaterOneDoesNot) {
  MasterScheduler scheduler;
  RecordingListener listener(scheduler);
  scheduler.scheduleAt(10, listener, 1);
  scheduler.scheduleAt(11, listener, 2);

  scheduler.advance(10);

  EXPECT_EQ(tagsOf(listener), (std::vector<std::uint32_t>{1}));
}

TEST(Scheduler, CancelRemovesPendingEventOnly) {
  MasterScheduler scheduler;
  RecordingListener listener(scheduler);
  const hw::EventId first = scheduler.scheduleAt(5, listener, 1);
  const hw::EventId second = scheduler.scheduleAt(6, listener, 2);
  EXPECT_NE(first, hw::NO_EVENT);
  EXPECT_NE(first, second);

  EXPECT_TRUE(scheduler.cancel(first));
  EXPECT_FALSE(scheduler.cancel(first));
  scheduler.advance(10);

  EXPECT_EQ(tagsOf(listener), (std::vector<std::uint32_t>{2}));
  EXPECT_FALSE(scheduler.cancel(second));
  EXPECT_FALSE(scheduler.cancel(hw::NO_EVENT));
}

TEST(Scheduler, EventScheduledInsideCallbackFiresInTheSameAdvanceIfDue) {
  MasterScheduler scheduler;
  RecordingListener listener(scheduler);
  listener.action = [&](std::uint32_t tag) {
    if (tag == 1) {
      scheduler.scheduleAt(8, listener, 2);   // 在目标之内：本次推进触发
      scheduler.scheduleAt(50, listener, 3);  // 在目标之外：留到以后
    }
  };
  scheduler.scheduleAt(5, listener, 1);

  scheduler.advance(10);

  EXPECT_EQ(tagsOf(listener), (std::vector<std::uint32_t>{1, 2}));
  EXPECT_EQ(scheduler.pendingCount(), 1U);
}

TEST(Scheduler, CallbackCanCancelAnotherPendingEvent) {
  MasterScheduler scheduler;
  RecordingListener listener(scheduler);
  hw::EventId victim = hw::NO_EVENT;
  listener.action = [&](std::uint32_t tag) {
    if (tag == 1) scheduler.cancel(victim);
  };
  scheduler.scheduleAt(5, listener, 1);
  victim = scheduler.scheduleAt(6, listener, 2);

  scheduler.advance(10);

  EXPECT_EQ(tagsOf(listener), (std::vector<std::uint32_t>{1}));
}

TEST(Scheduler, EventScheduledInThePastFiresOnNextAdvanceWithoutRewindingClock) {
  MasterScheduler scheduler;
  RecordingListener listener(scheduler);
  scheduler.advance(100);
  scheduler.scheduleAt(40, listener, 1);

  EXPECT_TRUE(listener.received.empty());
  scheduler.advance(0);

  ASSERT_EQ(listener.received.size(), 1U);
  EXPECT_EQ(listener.received[0].when, 40);
  EXPECT_EQ(listener.received[0].clockNow, 100);
  EXPECT_EQ(scheduler.now(), 100);
}

TEST(Scheduler, RejectsNegativeAndReentrantAdvance) {
  MasterScheduler scheduler;
  RecordingListener listener(scheduler);

  try {
    scheduler.advance(-1);
    FAIL() << "负数推进应当报错";
  } catch (const CoreError& error) {
    EXPECT_EQ(error.code(), hw::ERROR_CLOCK_INVALID_ADVANCE);
  }

  listener.action = [&](std::uint32_t) { EXPECT_THROW(scheduler.advance(1), CoreError); };
  scheduler.scheduleAt(1, listener, 1);
  scheduler.advance(2);
  EXPECT_EQ(listener.received.size(), 1U);
  EXPECT_EQ(scheduler.now(), 2);

  // 回调里的失败不影响以后继续推进。
  EXPECT_NO_THROW(scheduler.advance(1));
}

TEST(Scheduler, ResetReturnsToPowerOnState) {
  MasterScheduler scheduler;
  RecordingListener listener(scheduler);
  scheduler.scheduleAt(50, listener, 1);
  scheduler.advance(10);

  scheduler.reset();

  EXPECT_EQ(scheduler.now(), 0);
  EXPECT_EQ(scheduler.pendingCount(), 0U);
  scheduler.advance(100);
  EXPECT_TRUE(listener.received.empty());
}

TEST(ClockDomain, ConvertsBetweenChipCyclesAndMasterTicks) {
  constexpr hw::ClockDomain cpu{12};
  constexpr hw::ClockDomain ppu{4};

  EXPECT_EQ(cpu.toMaster(3), 36);
  EXPECT_EQ(ppu.toMaster(3), 12);
  EXPECT_EQ(cpu.toCycles(36), 3);
  EXPECT_EQ(cpu.toCycles(47), 3);
  // 一个 CPU 周期等于三个 PPU 点时钟。
  EXPECT_EQ(ppu.toCycles(cpu.toMaster(1)), 3);
}

}  // namespace
