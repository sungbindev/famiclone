/*
 * ------------------------------------------------------------------------
 * 名称：psg-um6576-test.cpp
 * 说明：6576 PSG 的测试：寄存器与 $4015 状态、各通道发声、长度计数器到期、采样数量、DAC 电平混入。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：时钟用 CPU 16 分频、1662607 Hz 的 PAL 时序；声音只核对有无与大致数量，不核对波形细节。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

#include "logic/hardware/psg-um6576/psg-um6576.hpp"
#include "logic/hardware/scheduler/master-scheduler.hpp"
#include "shared/audio/band-limited-sampler.hpp"

namespace {

namespace hw = ember::contracts::hardware;
using ember::logic::hardware::MasterScheduler;
using ember::logic::hardware::PsgUm6576;

constexpr double CPU_HZ = 1'662'607.0;
/** 0.1 秒的主时钟周期数。 */
constexpr hw::MasterTime TENTH_SECOND = 16 * 166'261;

struct PsgBench {
  PsgBench() : psg(scheduler, hw::ClockDomain{16}, CPU_HZ) { psg.reset(true); }
  /** 推进时间并取走采样。 */
  std::vector<float> run(hw::MasterTime ticks) {
    scheduler.advance(ticks);
    std::vector<float> samples;
    psg.takeAudio(samples);
    return samples;
  }
  MasterScheduler scheduler;
  PsgUm6576 psg;
};

float peak(const std::vector<float>& samples) {
  float value = 0;
  for (const float sample : samples) value = std::max(value, std::fabs(sample));
  return value;
}

/** 后半段的峰值：避开三角波的静止电平经高通产生的起始瞬态。 */
float tailPeak(const std::vector<float>& samples) {
  float value = 0;
  for (std::size_t index = samples.size() / 2; index < samples.size(); ++index) value = std::max(value, std::fabs(samples[index]));
  return value;
}

TEST(PsgUm6576, ProducesAboutTheRightNumberOfSamples) {
  PsgBench bench;
  const auto samples = bench.run(16 * 1'662'607);
  EXPECT_NEAR(static_cast<double>(samples.size()), 48000.0 - ember::shared::BandLimitedSampler::HALF_WIDTH, 3.0) << "\u5e26\u9650\u91c7\u6837\u5668\u6709 16 \u4e2a\u91c7\u6837\u7684\u56fa\u5b9a\u5ef6\u8fdf";
}

/** \u7528 DAC \u7535\u5e73\u6bcf halfPeriodCycles \u4e2a CPU \u5468\u671f\u7ffb\u8f6c\u4e00\u6b21\u7684\u65b9\u6ce2\u9a71\u52a8 PSG 0.1 \u79d2\uff0c\u8fd4\u56de\u540e\u534a\u6bb5\u7684\u5747\u65b9\u6839\u3002 */
double dacSquareWaveRms(std::int64_t halfPeriodCycles) {
  PsgBench bench;
  std::vector<float> samples;
  std::uint8_t level = 0;
  for (std::int64_t cycle = 0; cycle < TENTH_SECOND / 16; cycle += halfPeriodCycles) {
    level = level == 0 ? 255 : 0;
    bench.psg.setDacLevel(level);
    bench.scheduler.advance(16 * halfPeriodCycles);
  }
  bench.psg.takeAudio(samples);
  double sum = 0;
  std::size_t count = 0;
  for (std::size_t index = samples.size() / 2; index < samples.size(); ++index, ++count) sum += static_cast<double>(samples[index]) * samples[index];
  return std::sqrt(sum / static_cast<double>(count));
}

TEST(PsgUm6576, FrequenciesAboveNyquistAreRemovedInsteadOfFoldingBackAsHiss) {
  // \u6bcf 300 \u4e2a\u5468\u671f\u7ffb\u8f6c\uff1a\u7ea6 2.8 kHz\uff1b\u6bcf 30 \u4e2a\u5468\u671f\u7ffb\u8f6c\uff1a\u7ea6 27.7 kHz\uff0c\u9ad8\u4e8e\u5948\u594e\u65af\u7279\u9891\u7387\u3002
  const double audible = dacSquareWaveRms(300);
  const double ultrasonic = dacSquareWaveRms(30);
  EXPECT_GT(audible, 0.1);
  EXPECT_LT(ultrasonic, audible * 0.05);
}

TEST(PsgUm6576, SilentWhenNothingIsEnabled) {
  PsgBench bench;
  bench.psg.write(0x4000, 0xBF);
  bench.psg.write(0x4002, 0x40);
  bench.psg.write(0x4003, 0x08);
  const auto samples = bench.run(TENTH_SECOND);
  EXPECT_LT(tailPeak(samples), 1e-3F) << "没有写 $4015 使能，通道不发声";
}

TEST(PsgUm6576, PulseChannelMakesSound) {
  PsgBench bench;
  bench.psg.write(0x4015, 0x01);
  bench.psg.write(0x4000, 0xBF);  // 占空比 2，长度暂停，固定音量 15
  bench.psg.write(0x4002, 0x40);
  bench.psg.write(0x4003, 0x08);
  const auto samples = bench.run(TENTH_SECOND);
  EXPECT_GT(peak(samples), 0.05F);
  EXPECT_EQ(bench.psg.read(0x4015).value, 0x01);
}

TEST(PsgUm6576, VeryShortPulsePeriodIsMuted) {
  PsgBench bench;
  bench.psg.write(0x4015, 0x01);
  bench.psg.write(0x4000, 0xBF);
  bench.psg.write(0x4002, 0x04);
  bench.psg.write(0x4003, 0x08);
  const auto samples = bench.run(TENTH_SECOND);
  EXPECT_LT(tailPeak(samples), 1e-3F) << "周期小于 8 的方波静音";
}

TEST(PsgUm6576, LengthCounterEndsTheNote) {
  PsgBench bench;
  bench.psg.write(0x4015, 0x01);
  bench.psg.write(0x4000, 0x9F);  // 不暂停长度计数器
  bench.psg.write(0x4002, 0x40);
  bench.psg.write(0x4003, 0x18);  // 长度表索引 3，值 2
  EXPECT_TRUE(bench.psg.channelActive(0));
  bench.run(TENTH_SECOND);
  EXPECT_FALSE(bench.psg.channelActive(0));
  EXPECT_EQ(bench.psg.read(0x4015).value, 0x00);
}

TEST(PsgUm6576, DisablingAChannelClearsItsLength) {
  PsgBench bench;
  bench.psg.write(0x4015, 0x04);
  bench.psg.write(0x4008, 0xFF);
  bench.psg.write(0x400A, 0x30);
  bench.psg.write(0x400B, 0x08);
  EXPECT_EQ(bench.psg.read(0x4015).value, 0x04);
  bench.psg.write(0x4015, 0x00);
  EXPECT_EQ(bench.psg.read(0x4015).value, 0x00);
}

TEST(PsgUm6576, TriangleChannelMakesSound) {
  PsgBench bench;
  bench.psg.write(0x4015, 0x04);
  bench.psg.write(0x4008, 0xFF);  // 控制位置位，线性计数器重载值 127
  bench.psg.write(0x400A, 0x40);
  bench.psg.write(0x400B, 0x08);
  const auto samples = bench.run(TENTH_SECOND);
  EXPECT_GT(peak(samples), 0.02F);
}

TEST(PsgUm6576, NoiseChannelMakesSound) {
  PsgBench bench;
  bench.psg.write(0x4015, 0x08);
  bench.psg.write(0x400C, 0x3F);
  bench.psg.write(0x400E, 0x04);
  bench.psg.write(0x400F, 0x08);
  const auto samples = bench.run(TENTH_SECOND);
  EXPECT_GT(peak(samples), 0.02F);
}

TEST(PsgUm6576, DacLevelStepsTheOutput) {
  PsgBench bench;
  bench.run(TENTH_SECOND / 10);
  bench.psg.setDacLevel(255);
  const auto samples = bench.run(TENTH_SECOND / 10);
  ASSERT_FALSE(samples.empty());
  EXPECT_GT(peak(samples), 0.3F) << "DAC 电平的阶跃先被听到，随后被高通滤掉";
  const float last = samples.back();
  EXPECT_LT(std::fabs(last), peak(samples)) << "直流分量逐渐衰减";
}

TEST(PsgUm6576, ResetSilencesEverything) {
  PsgBench bench;
  bench.psg.write(0x4015, 0x0F);
  bench.psg.write(0x4000, 0xBF);
  bench.psg.write(0x4002, 0x40);
  bench.psg.write(0x4003, 0x08);
  bench.psg.reset(true);
  EXPECT_EQ(bench.psg.read(0x4015).value, 0x00);
  EXPECT_LT(tailPeak(bench.run(TENTH_SECOND)), 1e-3F);
}

}  // namespace
