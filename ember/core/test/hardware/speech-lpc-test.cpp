/*
 * ------------------------------------------------------------------------
 * 名称：speech-lpc-test.cpp
 * 说明：学习机语音芯片的测试：状态位、命令字节、缓冲、帧与采样的时间、播放结束与确认、数据不够时的等待、复位、输出的确定性。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：码流用测试里的位打包函数按“能量、重复位、音高、K1-K10”的顺序生成，字节按位反序使用；
 *       时间用 26,601,712 Hz 的主时钟：一个采样约 2660 个主时钟周期，一帧 200 个采样（20 毫秒）。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>

#include "logic/hardware/scheduler/master-scheduler.hpp"
#include "logic/hardware/speech-lpc/speech-lpc.hpp"

namespace {

namespace hw = ember::contracts::hardware;
using ember::logic::hardware::MasterScheduler;
using ember::logic::hardware::SpeechLpc;

constexpr double MASTER_HZ = 26'601'712.0;
constexpr std::uint16_t PORT = 0x4302;

/** 把毫秒换成主时钟周期数。 */
constexpr hw::MasterTime milliseconds(double value) { return static_cast<hw::MasterTime>(value * MASTER_HZ / 1000.0); }

/** 一个 (值, 位数) 的字段，值的高位在前。 */
using Field = std::pair<unsigned, unsigned>;

/** 把字段依次排成位序列，再按“每个字节从最低位起取”打包成码流字节。 */
std::vector<std::uint8_t> pack(const std::vector<Field>& fields) {
  std::vector<unsigned> bits;
  for (const auto& [value, count] : fields) {
    for (unsigned index = 0; index < count; ++index) bits.push_back((value >> (count - 1 - index)) & 1U);
  }
  std::vector<std::uint8_t> bytes((bits.size() + 7) / 8, 0);
  for (std::size_t index = 0; index < bits.size(); ++index) bytes[index / 8] = static_cast<std::uint8_t>(bytes[index / 8] | (bits[index] << (index % 8)));
  return bytes;
}

/** 一个浊音帧（D6 码表）：能量索引 8，音高索引 40，K 索引取中间值。 */
std::vector<Field> voicedFrameD6() {
  return {{8, 4}, {0, 1}, {40, 7}, {30, 6}, {23, 6}, {20, 5}, {17, 5}, {8, 4}, {8, 4}, {8, 4}, {4, 3}, {4, 3}, {4, 3}};
}

/** 一个浊音帧（PE 码表）：音高 6 位，K1、K2 5 位，K3、K4 4 位。 */
std::vector<Field> voicedFramePe() {
  return {{8, 4}, {0, 1}, {20, 6}, {16, 5}, {16, 5}, {8, 4}, {8, 4}, {8, 4}, {8, 4}, {8, 4}, {4, 3}, {4, 3}, {4, 3}};
}

std::vector<Field> concat(std::vector<Field> first, const std::vector<Field>& second) {
  first.insert(first.end(), second.begin(), second.end());
  return first;
}

struct SpeechBench {
  SpeechBench() : speech(scheduler, MASTER_HZ) {
    speech.onLevel = [this](double level) { levels.push_back(level); };
  }
  void send(const std::vector<std::uint8_t>& bytes) {
    for (const std::uint8_t byte : bytes) speech.write(PORT, byte);
  }
  void run(double ms) { scheduler.advance(milliseconds(ms)); }
  std::uint8_t status() { return speech.read(PORT).value; }
  MasterScheduler scheduler;
  SpeechLpc speech;
  std::vector<double> levels;
};

/** 命令字节加一组帧（字段连在一起打包），最后是结束帧。 */
std::vector<std::uint8_t> stream(std::uint8_t command, const std::vector<Field>& frames) {
  std::vector<std::uint8_t> bytes = {command};
  const auto body = pack(concat(frames, {{15, 4}}));
  bytes.insert(bytes.end(), body.begin(), body.end());
  return bytes;
}

TEST(SpeechLpc, StartsReadyAndNotEnded) {
  SpeechBench bench;
  EXPECT_EQ(bench.status(), 0x80);
  EXPECT_FALSE(bench.speech.ended());
  EXPECT_FALSE(bench.speech.playing());
}

TEST(SpeechLpc, ABogusCommandByteIsDiscarded) {
  SpeechBench bench;
  bench.speech.write(PORT, 0x55);
  EXPECT_EQ(bench.speech.pendingBytes(), 0U);
  EXPECT_FALSE(bench.speech.playing());
  bench.run(50);
  EXPECT_TRUE(bench.levels.empty());
}

TEST(SpeechLpc, ASilentStreamEndsAfterOneFrameAndTheAcknowledgeByteRestarts) {
  SpeechBench bench;
  // $0A 命令，一个字节 $00 是两个静音帧，$0F 的低 4 位是结束帧。
  bench.send({0x0A, 0x00, 0x0F});
  EXPECT_TRUE(bench.speech.playing());
  EXPECT_FALSE(bench.speech.ended());
  bench.run(19.5);
  EXPECT_FALSE(bench.speech.ended()) << "第一帧 20 毫秒还没有合成完";
  bench.run(1.0);
  EXPECT_TRUE(bench.speech.ended());
  EXPECT_EQ(bench.speech.framesSynthesized(), 1U);
  EXPECT_EQ(bench.status(), 0x8F) << "缓冲未满，低 4 位为 $F";
  EXPECT_FALSE(bench.speech.playing());

  bench.speech.write(PORT, 0x0F);
  EXPECT_FALSE(bench.speech.ended());
  EXPECT_EQ(bench.status(), 0x80);
  EXPECT_EQ(bench.speech.pendingBytes(), 0U);
}

TEST(SpeechLpc, BytesAfterTheEndAreDroppedUntilTheAcknowledge) {
  SpeechBench bench;
  bench.send({0x0A, 0x00, 0x0F});
  bench.run(25);
  ASSERT_TRUE(bench.speech.ended());
  bench.speech.write(PORT, 0x55);
  bench.speech.write(PORT, 0x0A);
  EXPECT_TRUE(bench.speech.ended()) << "不是 $0F 的字节只被丢弃";
  EXPECT_EQ(bench.speech.pendingBytes(), 0U);
  bench.speech.write(PORT, 0x0F);
  EXPECT_FALSE(bench.speech.ended());
  // 重新开始后需要新的命令字节。
  bench.send({0x0A, 0x00, 0x0F});
  bench.run(25);
  EXPECT_TRUE(bench.speech.ended());
}

TEST(SpeechLpc, ABufferThatIsFullClearsTheReadyBit) {
  SpeechBench bench;
  bench.send({0x0A, 0x00});
  ASSERT_TRUE(bench.speech.playing());
  for (unsigned index = 0; index < SpeechLpc::FIFO_CAPACITY; ++index) bench.speech.write(PORT, 0x00);
  EXPECT_EQ(bench.speech.pendingBytes(), SpeechLpc::FIFO_CAPACITY);
  EXPECT_EQ(bench.status() & 0x80, 0) << "满了就不能再写";
  bench.speech.write(PORT, 0x11);
  EXPECT_EQ(bench.speech.pendingBytes(), SpeechLpc::FIFO_CAPACITY) << "满时再写的字节被丢弃";
  bench.run(21);
  EXPECT_EQ(bench.status() & 0x80, 0x80) << "合成器取走一帧的数据后又可以写";
}

TEST(SpeechLpc, WaitsForDataWhenTheNextFrameIsMissing) {
  SpeechBench bench;
  bench.send({0x0A, 0x00});
  bench.run(60);
  EXPECT_TRUE(bench.speech.playing()) << "没有后续数据时一直等待";
  EXPECT_FALSE(bench.speech.ended());
  EXPECT_EQ(bench.speech.framesSynthesized(), 1U);
  bench.speech.write(PORT, 0x0F);
  EXPECT_TRUE(bench.speech.ended()) << "数据到了立刻读下一帧";
}

TEST(SpeechLpc, VoicedFramesProduceSoundAtTenKilohertz) {
  SpeechBench bench;
  const auto frame = voicedFrameD6();
  // 三个帧加结束帧：开头读两帧，第一帧合成完读第三帧，第二帧合成完读到结束帧，共合成两帧（40 毫秒）。
  bench.send(stream(0x0A, concat(concat(frame, frame), frame)));
  bench.run(19.5);
  EXPECT_EQ(bench.speech.framesSynthesized(), 0U);
  bench.run(1.0);
  EXPECT_EQ(bench.speech.framesSynthesized(), 1U);
  EXPECT_FALSE(bench.speech.ended());
  bench.run(19.0);
  EXPECT_FALSE(bench.speech.ended());
  bench.run(1.0);
  EXPECT_TRUE(bench.speech.ended());
  EXPECT_EQ(bench.speech.framesSynthesized(), 2U);

  double peak = 0;
  for (const double level : bench.levels) {
    ASSERT_TRUE(std::isfinite(level));
    ASSERT_LE(std::fabs(level), 1.0);
    peak = std::max(peak, std::fabs(level));
  }
  EXPECT_GT(peak, 0.01) << "浊音帧有声音";
  EXPECT_GT(bench.levels.size(), 100U) << "多个采样的电平发生变化";
  EXPECT_EQ(bench.levels.back(), 0.0) << "结束后输出回到静音";
}

TEST(SpeechLpc, ThePeCommandByteSelectsTheOtherCodeTables) {
  SpeechBench bench;
  const auto frame = voicedFramePe();
  bench.send(stream(0x0B, concat(concat(frame, frame), frame)));
  bench.run(70);
  EXPECT_TRUE(bench.speech.ended());
  EXPECT_EQ(bench.speech.framesSynthesized(), 2U);
  double peak = 0;
  for (const double level : bench.levels) peak = std::max(peak, std::fabs(level));
  EXPECT_GT(peak, 0.01);
}

TEST(SpeechLpc, ResetStopsPlaybackAndSilencesTheOutput) {
  SpeechBench bench;
  const auto frame = voicedFrameD6();
  bench.send(stream(0x0A, concat(frame, concat(frame, frame))));
  bench.run(5);
  ASSERT_TRUE(bench.speech.playing());
  bench.speech.reset(false);
  EXPECT_FALSE(bench.speech.playing());
  EXPECT_EQ(bench.speech.pendingBytes(), 0U);
  EXPECT_EQ(bench.levels.back(), 0.0);
  const std::size_t count = bench.levels.size();
  bench.run(100);
  EXPECT_EQ(bench.levels.size(), count) << "复位后不再有采样事件";
  EXPECT_EQ(bench.status(), 0x80);
}

TEST(SpeechLpc, TheSameStreamAlwaysGivesTheSameSamples) {
  const auto frame = voicedFrameD6();
  const auto bytes = stream(0x0A, concat(concat(frame, frame), frame));
  SpeechBench first;
  SpeechBench second;
  first.send(bytes);
  second.send(bytes);
  first.run(70);
  second.run(70);
  EXPECT_EQ(first.levels, second.levels);
}

/** 步步高变体的测试台。 */
struct BbkSpeechBench {
  BbkSpeechBench() : speech(scheduler, MASTER_HZ, SpeechLpc::Variant::Bbk) {}
  void send(const std::vector<std::uint8_t>& bytes) {
    for (const std::uint8_t byte : bytes) speech.write(PORT, byte);
  }
  MasterScheduler scheduler;
  SpeechLpc speech;
};

TEST(SpeechLpcBbk, StatusAlwaysHasTheLowNibbleSetAndBitSevenMeansNotFull) {
  BbkSpeechBench bench;
  EXPECT_EQ(bench.speech.read(PORT).value, 0x8F);
  // 没有命令字节时写入的字节被丢弃，缓冲不会满。
  for (unsigned index = 0; index < 20; ++index) bench.speech.write(PORT, 0x00);
  EXPECT_EQ(bench.speech.read(PORT).value, 0x8F);
}

TEST(SpeechLpcBbk, OnlyTheD6CommandByteStartsPlayback) {
  const auto frame = voicedFrameD6();
  BbkSpeechBench rejected;
  rejected.send(stream(0x0A, concat(frame, frame)));
  EXPECT_FALSE(rejected.speech.playing()) << "$0A 是小霸王的命令字节";
  BbkSpeechBench accepted;
  accepted.send(stream(0xD6, concat(frame, concat(frame, frame))));
  EXPECT_TRUE(accepted.speech.playing());
}

TEST(SpeechLpcBbk, AfterTheEndFrameEveryByteIsIgnoredUntilReset) {
  BbkSpeechBench bench;
  bench.send(stream(0xD6, {{0, 4}, {0, 4}}));
  bench.scheduler.advance(milliseconds(200));
  bench.send({0x0F});
  EXPECT_EQ(bench.speech.pendingBytes(), 0U) << "没有确认字节，后续字节被丢弃";
  EXPECT_FALSE(bench.speech.playing());
  bench.speech.reset(false);
  bench.send(stream(0xD6, concat(voicedFrameD6(), concat(voicedFrameD6(), voicedFrameD6()))));
  EXPECT_TRUE(bench.speech.playing()) << "复位后可以重新开始";
}

/** 裕兴变体的测试台。 */
struct YuxingSpeechBench {
  YuxingSpeechBench() : speech(scheduler, MASTER_HZ, SpeechLpc::Variant::Yuxing) {
    speech.onLevel = [this](double level) { levels.push_back(level); };
  }
  void send(const std::vector<std::uint8_t>& bytes) {
    for (const std::uint8_t byte : bytes) speech.write(PORT, byte);
  }
  void run(double ms) { scheduler.advance(milliseconds(ms)); }
  MasterScheduler scheduler;
  SpeechLpc speech;
  std::vector<double> levels;
};

/** 裕兴的码流：没有命令字节，一组帧之后是结束帧。 */
std::vector<std::uint8_t> yuxingStream(const std::vector<Field>& frames) { return pack(concat(frames, {{15, 4}})); }

TEST(SpeechLpcYuxing, StartsReadyWithNoPlaybackAndNoEndedFlag) {
  YuxingSpeechBench bench;
  EXPECT_EQ(bench.speech.read(PORT).value, 0x00) << "裕兴的状态：能写时读 $00";
  EXPECT_FALSE(bench.speech.playing());
  EXPECT_FALSE(bench.speech.ended());
}

TEST(SpeechLpcYuxing, FramesAreReadWithThePeTablesWithoutAnyCommandByte) {
  YuxingSpeechBench bench;
  const auto frame = voicedFramePe();
  bench.send(yuxingStream(concat(frame, concat(frame, frame))));
  EXPECT_TRUE(bench.speech.playing());
  bench.run(70);
  double peak = 0;
  for (const double level : bench.levels) peak = std::max(peak, std::fabs(level));
  EXPECT_GT(peak, 0.01) << "浊音帧有声音";
  EXPECT_EQ(bench.speech.framesSynthesized(), 2U);
}

TEST(SpeechLpcYuxing, TheStatusReadsBusyWhenEightOrMoreBytesAreWaiting) {
  YuxingSpeechBench bench;
  const auto frame = voicedFramePe();
  // 开头两帧收齐之后开始播放，播放期间后面的字节留在缓冲里。
  std::vector<Field> frames;
  for (unsigned index = 0; index < 8; ++index) frames = concat(frames, frame);
  bench.send(yuxingStream(frames));
  ASSERT_TRUE(bench.speech.playing());
  EXPECT_GE(bench.speech.pendingBytes(), 8U);
  EXPECT_EQ(bench.speech.read(PORT).value, 0x80);
  bench.speech.reset(false);
  EXPECT_EQ(bench.speech.read(PORT).value, 0x00);
}

TEST(SpeechLpcYuxing, AfterTheEndFrameTheNextSegmentStartsAtOnceWithoutAnAcknowledgeByte) {
  YuxingSpeechBench bench;
  const auto frame = voicedFramePe();
  const auto segment = yuxingStream(concat(frame, concat(frame, frame)));
  bench.send(segment);
  bench.run(60);
  EXPECT_EQ(bench.speech.framesSynthesized(), 2U);
  EXPECT_FALSE(bench.speech.ended()) << "读到结束帧也不置播放结束";
  bench.send(segment);
  bench.run(60);
  EXPECT_FALSE(bench.speech.ended());
  EXPECT_EQ(bench.speech.framesSynthesized(), 4U) << "不需要确认字节，第二段直接播";
}

TEST(SpeechLpcYuxing, ResetStopsPlaybackAndSilencesTheOutput) {
  YuxingSpeechBench bench;
  const auto frame = voicedFramePe();
  bench.send(yuxingStream(concat(frame, concat(frame, frame))));
  bench.run(5);
  ASSERT_TRUE(bench.speech.playing());
  bench.speech.reset(false);
  EXPECT_FALSE(bench.speech.playing());
  EXPECT_EQ(bench.speech.pendingBytes(), 0U);
  EXPECT_EQ(bench.levels.back(), 0.0);
}

}  // namespace
