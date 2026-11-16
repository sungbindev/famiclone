/*
 * ------------------------------------------------------------------------
 * 名称：band-limited-sampler-test.cpp
 * 说明：带限采样器的测试：采样数、阶跃响应与直流阻断、对超奈奎斯特频率的抑制、非法频率。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-16
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

#include "shared/audio/band-limited-sampler.hpp"
#include "shared/errors/core-error.hpp"

namespace {

using ember::shared::BandLimitedSampler;

constexpr double CPU_HZ = 21'477'272.0 / 12.0;

TEST(BandLimitedSampler, OutputsNothingButSilenceWithoutDeltasAndCountsSamplesByTheRatio) {
  BandLimitedSampler sampler(CPU_HZ, 48000);
  std::vector<float> out;
  for (std::int64_t tick = 1; tick <= 178'977; ++tick) sampler.advance(tick, out);
  // 0.1 秒：4800 个采样减去 8 个采样的延迟（+1，因为样本从第 0 个算起）。
  EXPECT_NEAR(static_cast<double>(out.size()), 4800.0 - BandLimitedSampler::HALF_WIDTH, 2.0);
  EXPECT_EQ(sampler.emittedSamples(), static_cast<std::int64_t>(out.size()));
  for (const float sample : out) ASSERT_EQ(sample, 0.0F);
}

TEST(BandLimitedSampler, AStepSettlesAtTheDeltaThenTheDcBlockerRemovesIt) {
  BandLimitedSampler sampler(CPU_HZ, 48000);
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
  for (std::size_t i = 0; i + 1 < stepSample - BandLimitedSampler::HALF_WIDTH - 2; ++i) ASSERT_EQ(out[i], 0.0F) << "脉冲响应之前没有输出";
  EXPECT_LT(std::abs(out[stepSample + 4800]), 0.01F) << "100 毫秒后直流已被阻断";
  EXPECT_LT(std::abs(out.back()), 1e-6F);
}

/** 用方波（每 halfPeriodTicks 个周期翻转一次，幅度 ±0.5）驱动采样器，返回稳定后的均方根。 */
double squareWaveRms(std::int64_t halfPeriodTicks) {
  BandLimitedSampler sampler(CPU_HZ, 48000);
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
  EXPECT_THROW(BandLimitedSampler(0.0, 48000), ember::shared::CoreError);
  EXPECT_THROW(BandLimitedSampler(CPU_HZ, 0), ember::shared::CoreError);
  EXPECT_THROW(BandLimitedSampler(48000.0, 48000), ember::shared::CoreError);
}

}  // namespace
