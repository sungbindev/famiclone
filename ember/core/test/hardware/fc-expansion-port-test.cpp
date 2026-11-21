/*
 * ------------------------------------------------------------------------
 * 名称：fc-expansion-port-test.cpp
 * 说明：经典 FC 的 $4016-$4017 外设接口的测试：手柄口与 APU 的处理原样转发，磁带信号叠加在 $4016 读数的位 1，写 $4016 的位 0 是送给磁带的信号、位 2 是读回使能。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：用假的手柄口设备与真实的磁带机（内存里的磁带来源与录音去向）；主时钟取 96,000,000 Hz，每个磁带采样 1000 个主时钟周期。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "bootstrap/machines/fc/fc-expansion-port.hpp"
#include "logic/hardware/scheduler/master-scheduler.hpp"
#include "logic/hardware/tape-deck/tape-deck.hpp"

namespace {

namespace hw = ember::contracts::hardware;

constexpr double MASTER_HZ = 96'000'000.0;
constexpr hw::MasterTime FRAME_TICKS = 1000;

/** 记录写入、读出固定值的手柄口替身。 */
struct FakePad final : hw::BusDevice {
  hw::BusRead read(std::uint16_t /*address*/) override { return hw::BusRead{padValue, 0x1F}; }
  void write(std::uint16_t address, std::uint8_t value) override { writes.emplace_back(address, value); }
  std::uint8_t padValue{0x01};
  std::vector<std::pair<std::uint16_t, std::uint8_t>> writes;
};

class FakeSource final : public hw::TapeSource {
 public:
  explicit FakeSource(std::vector<std::int16_t> right) : right_(std::move(right)) {}
  std::size_t read(std::int16_t* interleaved, std::size_t frames) override {
    std::size_t count = 0;
    while (count < frames && cursor_ < right_.size()) {
      interleaved[count * 2] = 0;
      interleaved[count * 2 + 1] = right_[cursor_++];
      ++count;
    }
    return count;
  }
  void rewind() override { cursor_ = 0; }
  [[nodiscard]] double durationSeconds() const override { return 0; }
  [[nodiscard]] bool stereo() const override { return false; }

 private:
  std::vector<std::int16_t> right_;
  std::size_t cursor_{0};
};

struct RecordingSink final : hw::TapeSink {
  explicit RecordingSink(std::vector<std::int16_t>* target) : samples(target) {}
  void write(const std::int16_t* data, std::size_t count) override { samples->insert(samples->end(), data, data + count); }
  void finish() override {}
  std::vector<std::int16_t>* samples;
};

struct PortBench {
  PortBench() : deck(scheduler, MASTER_HZ), port(pad, deck, keyboard) {}
  void advanceFrames(std::int64_t frames) { scheduler.advance(frames * FRAME_TICKS); }
  ember::logic::hardware::MasterScheduler scheduler;
  ember::logic::hardware::TapeDeck deck;
  FakePad pad;
  ember::logic::hardware::KeyboardFc keyboard;
  ember::bootstrap::FcExpansionPort port;
};

TEST(FcExpansionPort, PadAndApuAccessesPassThroughUnchanged) {
  PortBench bench;
  EXPECT_EQ(bench.port.read(0x4017).value, 0x01);
  bench.port.write(0x4017, 0x40);
  ASSERT_EQ(bench.pad.writes.size(), 1U);
  EXPECT_EQ(bench.pad.writes[0], std::make_pair(std::uint16_t{0x4017}, std::uint8_t{0x40}));
}

TEST(FcExpansionPort, TheTapeSignalAppearsInBitOneOfTheFirstPortOnlyWhenEnabled) {
  PortBench bench;
  bench.deck.load(std::make_unique<FakeSource>(std::vector<std::int16_t>(100, 10000)), "t");
  bench.deck.play();
  bench.advanceFrames(10);
  EXPECT_EQ(bench.port.read(0x4016).value & 0x02, 0) << "没有写过使能位，读回恒为 0";
  bench.port.write(0x4016, 0x04);
  const hw::BusRead read = bench.port.read(0x4016);
  EXPECT_EQ(read.value & 0x03, 0x03) << "手柄的位 0 与磁带的位 1";
  EXPECT_NE(read.drivenMask & 0x02, 0);
  EXPECT_EQ(bench.port.read(0x4017).value & 0x02, 0) << "$4017 没有磁带信号";
  bench.port.write(0x4016, 0x00);
  EXPECT_EQ(bench.port.read(0x4016).value & 0x02, 0) << "使能位清零后读回恒为 0";
}

TEST(FcExpansionPort, BitZeroOfTheWriteIsRecordedAsTheTapeSignal) {
  PortBench bench;
  std::vector<std::int16_t> samples;
  bench.deck.record(std::make_unique<RecordingSink>(&samples), "r");
  bench.port.write(0x4016, 0x00);
  bench.advanceFrames(3000);
  bench.port.write(0x4016, 0x05);
  bench.advanceFrames(3000);
  bench.port.write(0x4016, 0x04);
  bench.deck.stop();
  ASSERT_EQ(samples.size(), 2000U);
  EXPECT_EQ(samples[0], 0) << "第一次拉高之前数据线不叠加";
  EXPECT_EQ(samples[1000], ember::logic::hardware::TapeDeck::RECORD_AMPLITUDE);
  EXPECT_EQ(samples[1999], ember::logic::hardware::TapeDeck::RECORD_AMPLITUDE);
}

TEST(FcExpansionPort, TheKeyboardOnlyAnswersOnceConnectedAndSharesTheEnableBitWithTheTape) {
  PortBench bench;
  bench.keyboard.setKey("KeyA", true);
  bench.port.write(0x4016, 0x05);
  bench.port.write(0x4016, 0x04);
  EXPECT_EQ(bench.port.read(0x4017).value & 0x1E, 0) << "没接键盘，不驱动 $4017 的位 1-4";
  EXPECT_EQ(bench.port.read(0x4017).drivenMask & 0x1E, 0x1E) << "手柄口自己驱动低 5 位（假手柄口的掩码）";

  bench.port.connectKeyboard(true);
  EXPECT_TRUE(bench.port.keyboardConnected());
  bench.port.write(0x4016, 0x05);
  // 选列线每由高变低一次，行加 1：走到第 6 行（A S W F2 | 3 E Z X），当前是列 0，A 在 $4017 位 4。
  for (unsigned row = 0; row < 6; ++row) {
    bench.port.write(0x4016, 0x06);
    bench.port.write(0x4016, 0x04);
  }
  EXPECT_EQ(bench.port.read(0x4017).value & 0x1E, 0x0E) << "按下的 A 读 0，其余为 1";
  bench.port.write(0x4016, 0x00);
  EXPECT_EQ(bench.port.read(0x4017).value & 0x1E, 0) << "使能位清零后读出恒为 0";
}

}  // namespace
