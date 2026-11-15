/*
 * ------------------------------------------------------------------------
 * 名称：tape-deck-test.cpp
 * 说明：磁带机的测试：数据线的施密特比较与取反、播放的时间、停止后继续、边沿事件的时刻与间隔、磁带放完与重播、立体声磁带的声音、单声道磁带无声、录音的采样。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：主时钟取 96,000,000 Hz，每个磁带采样（96 kHz）正好 1000 个主时钟周期，帧号与时间互相换算没有误差；用内存里的假来源与假去向。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "logic/hardware/scheduler/master-scheduler.hpp"
#include "logic/hardware/tape-deck/tape-deck.hpp"

namespace {

namespace hw = ember::contracts::hardware;
using ember::logic::hardware::MasterScheduler;
using ember::logic::hardware::TapeDeck;

constexpr double MASTER_HZ = 96'000'000.0;
/** 一个磁带采样的主时钟周期数。 */
constexpr hw::MasterTime FRAME_TICKS = 1000;

/** 内存里的磁带：每帧 (左, 右)。 */
class FakeSource final : public hw::TapeSource {
 public:
  FakeSource(std::vector<std::pair<std::int16_t, std::int16_t>> frames, bool stereoTape) : frames_(std::move(frames)), stereo_(stereoTape) {}

  std::size_t read(std::int16_t* interleaved, std::size_t frames) override {
    std::size_t count = 0;
    while (count < frames && cursor_ < frames_.size()) {
      interleaved[count * 2] = frames_[cursor_].first;
      interleaved[count * 2 + 1] = frames_[cursor_].second;
      ++count;
      ++cursor_;
    }
    return count;
  }
  void rewind() override {
    cursor_ = 0;
    ++rewinds;
  }
  [[nodiscard]] double durationSeconds() const override { return static_cast<double>(frames_.size()) / SAMPLE_RATE; }
  [[nodiscard]] bool stereo() const override { return stereo_; }

  int rewinds{0};

 private:
  std::vector<std::pair<std::int16_t, std::int16_t>> frames_;
  bool stereo_;
  std::size_t cursor_{0};
};

/** 收集录音的去向。 */
struct FakeSink final : hw::TapeSink {
  explicit FakeSink(std::vector<std::int16_t>* target, bool* finishedFlag) : samples(target), finished(finishedFlag) {}
  void write(const std::int16_t* data, std::size_t count) override { samples->insert(samples->end(), data, data + count); }
  void finish() override { *finished = true; }
  std::vector<std::int16_t>* samples;
  bool* finished;
};

/** 数据线（右声道）：前 low 帧为 value 的负值，之后 high 帧为正值，依次交替。 */
std::vector<std::pair<std::int16_t, std::int16_t>> squareWave(const std::vector<std::size_t>& runs, std::int16_t amplitude = 10000, std::int16_t left = 0) {
  std::vector<std::pair<std::int16_t, std::int16_t>> frames;
  bool high = false;
  for (const std::size_t run : runs) {
    for (std::size_t index = 0; index < run; ++index) frames.emplace_back(left, high ? amplitude : static_cast<std::int16_t>(-amplitude));
    high = !high;
  }
  return frames;
}

struct DeckBench {
  DeckBench() : deck(scheduler, MASTER_HZ) {}
  void advanceFrames(std::int64_t frames) { scheduler.advance(frames * FRAME_TICKS); }
  MasterScheduler scheduler;
  TapeDeck deck;
};

TEST(TapeDeck, StartsEmptyAndStopped) {
  DeckBench bench;
  EXPECT_FALSE(bench.deck.loaded());
  EXPECT_EQ(bench.deck.mode(), hw::TapeMode::Stopped);
  EXPECT_EQ(bench.deck.positionSeconds(), 0.0);
  bench.deck.play();
  EXPECT_EQ(bench.deck.mode(), hw::TapeMode::Stopped) << "没有磁带不能播放";
}

TEST(TapeDeck, TheDataLineFollowsTheRightChannelOnlyWhilePlaying) {
  DeckBench bench;
  bench.deck.load(std::make_unique<FakeSource>(squareWave({100, 100, 100}), false), "data.wav");
  EXPECT_TRUE(bench.deck.loaded());
  EXPECT_EQ(bench.deck.name(), "data.wav");
  EXPECT_FALSE(bench.deck.level());
  bench.advanceFrames(150);
  EXPECT_FALSE(bench.deck.level()) << "还没有播放，信号线不动";
  bench.deck.play();
  EXPECT_EQ(bench.deck.mode(), hw::TapeMode::Playing);
  bench.advanceFrames(50);
  EXPECT_FALSE(bench.deck.level()) << "第 50 帧还在低电平";
  bench.advanceFrames(51);
  EXPECT_TRUE(bench.deck.level()) << "第 101 帧翻成高电平";
  bench.advanceFrames(100);
  EXPECT_FALSE(bench.deck.level()) << "第 201 帧翻回低电平";
}

TEST(TapeDeck, TheComparatorHoldsTheValueBetweenItsThresholds) {
  DeckBench bench;
  // -10000 -> 1000（在两个门限之间） -> 10000 -> 1000 -> -10000
  std::vector<std::pair<std::int16_t, std::int16_t>> frames;
  for (const std::int16_t value : {std::int16_t{-10000}, std::int16_t{1000}, std::int16_t{10000}, std::int16_t{1000}, std::int16_t{-10000}}) {
    for (int index = 0; index < 10; ++index) frames.emplace_back(0, value);
  }
  bench.deck.load(std::make_unique<FakeSource>(frames, false), "t");
  bench.deck.play();
  bench.advanceFrames(15);
  EXPECT_FALSE(bench.deck.level()) << "低电平后进入门限之间，保持低";
  bench.advanceFrames(10);
  EXPECT_TRUE(bench.deck.level());
  bench.advanceFrames(10);
  EXPECT_TRUE(bench.deck.level()) << "高电平后进入门限之间，保持高";
  bench.advanceFrames(10);
  EXPECT_FALSE(bench.deck.level());
}

TEST(TapeDeck, TheComparatorCanInvertTheLevel) {
  DeckBench bench;
  bench.deck.setComparator(hw::TapeComparator{-999, -999, true});
  bench.deck.load(std::make_unique<FakeSource>(squareWave({10, 10}), false), "t");
  EXPECT_FALSE(bench.deck.level()) << "没有放磁带时按静音（采样 0，高于门限）算，取反后为低";
  bench.deck.play();
  bench.advanceFrames(1);
  EXPECT_TRUE(bench.deck.level()) << "低采样经取反是高";
  bench.advanceFrames(10);
  EXPECT_FALSE(bench.deck.level()) << "高采样经取反是低";
}

TEST(TapeDeck, StoppingKeepsThePositionAndPlayResumesFromThere) {
  DeckBench bench;
  bench.deck.load(std::make_unique<FakeSource>(squareWave({100, 100, 100}), false), "t");
  bench.deck.play();
  bench.advanceFrames(150);
  EXPECT_TRUE(bench.deck.level());
  bench.deck.stop();
  EXPECT_EQ(bench.deck.mode(), hw::TapeMode::Stopped);
  const double stopped = bench.deck.positionSeconds();
  EXPECT_NEAR(stopped, 151.0 / 96000.0, 2.0 / 96000.0);
  bench.advanceFrames(500);
  EXPECT_NEAR(bench.deck.positionSeconds(), stopped, 1e-9) << "停止后位置不动";
  bench.deck.play();
  bench.advanceFrames(60);
  EXPECT_FALSE(bench.deck.level()) << "继续播放：第 201 帧翻回低电平";
}

TEST(TapeDeck, EdgeNotificationsComeAtTheTimeOfTheEdgeWithTheSampleSpacing) {
  DeckBench bench;
  std::vector<std::pair<bool, std::uint32_t>> edges;
  bench.deck.setEdgeListener([&](bool level, std::uint32_t samples) { edges.emplace_back(level, samples); });
  bench.deck.load(std::make_unique<FakeSource>(squareWave({100, 200, 300, 50}), false), "t");
  bench.deck.play();
  bench.advanceFrames(99);
  EXPECT_TRUE(edges.empty());
  bench.advanceFrames(1);
  ASSERT_EQ(edges.size(), 1U);
  EXPECT_EQ(edges[0], std::make_pair(true, 100U));
  bench.advanceFrames(200);
  ASSERT_EQ(edges.size(), 2U);
  EXPECT_EQ(edges[1], std::make_pair(false, 200U));
  bench.advanceFrames(300);
  ASSERT_EQ(edges.size(), 3U);
  EXPECT_EQ(edges[2], std::make_pair(true, 300U));
  bench.advanceFrames(100);
  EXPECT_EQ(bench.deck.mode(), hw::TapeMode::Stopped) << "放完自动停止";
  EXPECT_EQ(edges.size(), 3U) << "最后一段高电平一直到磁带结束，没有新的翻转";
}

TEST(TapeDeck, ALongSilenceIsScannedInSteps) {
  DeckBench bench;
  std::vector<std::pair<bool, std::uint32_t>> edges;
  bench.deck.setEdgeListener([&](bool level, std::uint32_t samples) { edges.emplace_back(level, samples); });
  bench.deck.load(std::make_unique<FakeSource>(squareWave({250000, 10}), false), "t");
  bench.deck.play();
  bench.advanceFrames(249999);
  EXPECT_TRUE(edges.empty());
  bench.advanceFrames(1);
  ASSERT_EQ(edges.size(), 1U);
  EXPECT_EQ(edges[0], std::make_pair(true, 250000U));
}

TEST(TapeDeck, ThePositionAndTheEndOfTheTape) {
  DeckBench bench;
  auto source = std::make_unique<FakeSource>(squareWave({96000, 96000}), false);
  FakeSource* raw = source.get();
  bench.deck.load(std::move(source), "two-seconds");
  EXPECT_NEAR(bench.deck.durationSeconds(), 2.0, 1e-9);
  bench.deck.play();
  bench.advanceFrames(48000);
  EXPECT_NEAR(bench.deck.positionSeconds(), 0.5, 1e-3);
  bench.advanceFrames(200000);
  EXPECT_EQ(bench.deck.mode(), hw::TapeMode::Stopped) << "放完后自动停止";
  EXPECT_NEAR(bench.deck.positionSeconds(), 2.0, 1e-9);
  bench.deck.play();
  EXPECT_EQ(raw->rewinds, 1) << "放完后再播放，从头开始";
  EXPECT_EQ(bench.deck.mode(), hw::TapeMode::Playing);
  bench.advanceFrames(1000);
  EXPECT_NEAR(bench.deck.positionSeconds(), 1000.0 / 96000.0, 2.0 / 96000.0);
}

TEST(TapeDeck, AStereoTapePlaysItsLeftChannelThroughTheMachineAudio) {
  DeckBench bench;
  bench.deck.setAudioRate(48000);
  bench.deck.load(std::make_unique<FakeSource>(squareWave({96000}, 10000, 16384), true), "stereo");
  bench.deck.play();
  bench.advanceFrames(9600);  // 0.1 秒
  std::vector<float> mix(4000, 0.25F);
  bench.deck.mixAudio(mix);
  EXPECT_NEAR(mix[100], 0.25F + 0.5F * 0.8F, 1e-3F) << "左声道 16384 是半幅，乘 0.8 加到原有的声音上";
  EXPECT_NEAR(mix[3999], 0.25F + 0.5F * 0.8F, 1e-3F);
}

TEST(TapeDeck, AMonoTapeMakesNoSound) {
  DeckBench bench;
  bench.deck.setAudioRate(48000);
  bench.deck.load(std::make_unique<FakeSource>(squareWave({96000}, 10000, 16384), false), "mono");
  bench.deck.play();
  bench.advanceFrames(9600);
  std::vector<float> mix(4000, 0.0F);
  bench.deck.mixAudio(mix);
  for (const float sample : mix) ASSERT_EQ(sample, 0.0F);
}

TEST(TapeDeck, RecordingTurnsTheDrivenLineIntoASquareWaveAt32Kilohertz) {
  DeckBench bench;
  std::vector<std::int16_t> samples;
  bool finished = false;
  bench.deck.record(std::make_unique<FakeSink>(&samples, &finished), "out.wav");
  EXPECT_EQ(bench.deck.mode(), hw::TapeMode::Recording);
  EXPECT_TRUE(bench.deck.loaded());
  bench.advanceFrames(3000);  // 31.25 毫秒 = 1000 个 32 kHz 采样，线保持低
  bench.deck.drive(true);
  bench.advanceFrames(3000);
  bench.deck.drive(false);
  bench.advanceFrames(1500);
  EXPECT_NEAR(bench.deck.positionSeconds(), 7500.0 / 96000.0, 1e-9);
  bench.deck.stop();
  EXPECT_TRUE(finished);
  EXPECT_EQ(bench.deck.mode(), hw::TapeMode::Stopped);
  EXPECT_FALSE(bench.deck.loaded());
  ASSERT_EQ(samples.size(), 2500U);
  EXPECT_EQ(samples.front(), -TapeDeck::RECORD_AMPLITUDE);
  EXPECT_EQ(samples[999], -TapeDeck::RECORD_AMPLITUDE);
  EXPECT_EQ(samples[1000], TapeDeck::RECORD_AMPLITUDE);
  EXPECT_EQ(samples[1999], TapeDeck::RECORD_AMPLITUDE);
  EXPECT_EQ(samples[2000], -TapeDeck::RECORD_AMPLITUDE);
  EXPECT_EQ(samples.back(), -TapeDeck::RECORD_AMPLITUDE);
}

TEST(TapeDeck, LoadingATapeStopsAndReplacesTheCurrentOne) {
  DeckBench bench;
  bench.deck.load(std::make_unique<FakeSource>(squareWave({1000}), false), "first");
  bench.deck.play();
  bench.advanceFrames(10);
  bench.deck.load(std::make_unique<FakeSource>(squareWave({1000}), false), "second");
  EXPECT_EQ(bench.deck.mode(), hw::TapeMode::Stopped);
  EXPECT_EQ(bench.deck.name(), "second");
  EXPECT_EQ(bench.deck.positionSeconds(), 0.0);
  bench.deck.eject();
  EXPECT_FALSE(bench.deck.loaded());
}

}  // namespace
