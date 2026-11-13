/*
 * ------------------------------------------------------------------------
 * 名称：audio-test.cpp
 * 说明：声音输出的测试：环形缓冲（读写顺序、回绕、写满丢弃最新、清空、跨线程）、回调一侧的渲染器（音量、静音、淡入淡出、缓冲不足补静音与衰减、
 *       预填充、按水位微调重采样比并保持水位稳定）、空输出的统计、系统输出的打开（有或没有声卡都不崩溃）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：渲染器的测试直接调用 render，不需要声卡；“水位”测试用确定的模拟：生产者与消费者每 10 毫秒各走一步，生产者的速度比消费者快或慢千分之几。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <numeric>
#include <thread>
#include <vector>

#include "platform/audio/audio-renderer.hpp"
#include "platform/audio/miniaudio-output.hpp"
#include "platform/audio/null-audio-output.hpp"
#include "platform/audio/sample-ring-buffer.hpp"

namespace {

using ember::platform::AudioRenderer;
using ember::platform::AudioRendererOptions;
using ember::platform::NullAudioOutput;
using ember::platform::SampleRingBuffer;

// ---------------------------------------------------------------- 环形缓冲

TEST(SampleRingBuffer, CapacityIsRoundedUpToAPowerOfTwo) {
  EXPECT_EQ(SampleRingBuffer(1000).capacity(), 1024U);
  EXPECT_EQ(SampleRingBuffer(1024).capacity(), 1024U);
  EXPECT_EQ(SampleRingBuffer(1).capacity(), 1U);
}

TEST(SampleRingBuffer, ReadsBackWhatWasWrittenInOrderAcrossTheWrapPoint) {
  SampleRingBuffer ring(8);
  std::vector<float> written;
  std::vector<float> readBack;
  float next = 0;
  for (int round = 0; round < 50; ++round) {
    std::vector<float> chunk(5);
    for (auto& value : chunk) value = next++;
    ASSERT_EQ(ring.write(chunk), 5U);
    written.insert(written.end(), chunk.begin(), chunk.end());
    std::vector<float> out(5);
    ASSERT_EQ(ring.read(out), 5U);
    readBack.insert(readBack.end(), out.begin(), out.end());
  }
  EXPECT_EQ(readBack, written);
  EXPECT_EQ(ring.size(), 0U);
}

TEST(SampleRingBuffer, WhenFullItKeepsTheOldestAndRefusesTheRest) {
  SampleRingBuffer ring(4);
  const std::vector<float> data = {1, 2, 3, 4, 5, 6};
  EXPECT_EQ(ring.write(data), 4U) << "只放得下 4 个，丢弃多出的最新采样";
  EXPECT_EQ(ring.size(), 4U);
  EXPECT_EQ(ring.write(data), 0U);
  std::vector<float> out(6, -1);
  EXPECT_EQ(ring.read(out), 4U);
  EXPECT_EQ(std::vector<float>(out.begin(), out.begin() + 4), (std::vector<float>{1, 2, 3, 4}));
  EXPECT_EQ(ring.read(out), 0U) << "空了读不到";
}

TEST(SampleRingBuffer, DiscardAllEmptiesTheBuffer) {
  SampleRingBuffer ring(16);
  const std::vector<float> data(10, 0.5F);
  ring.write(data);
  ring.discardAll();
  EXPECT_EQ(ring.size(), 0U);
  std::vector<float> out(4);
  EXPECT_EQ(ring.read(out), 0U);
  EXPECT_EQ(ring.write(data), 10U);
  EXPECT_EQ(ring.size(), 10U);
}

TEST(SampleRingBuffer, WorksBetweenOneProducerThreadAndOneConsumerThread) {
  SampleRingBuffer ring(256);
  constexpr int TOTAL = 200000;
  std::atomic<bool> failed{false};
  std::thread consumer([&] {
    float expected = 0;
    int received = 0;
    std::vector<float> chunk(37);
    while (received < TOTAL) {
      const std::size_t count = ring.read(chunk);
      for (std::size_t i = 0; i < count; ++i) {
        if (chunk[i] != expected) failed = true;
        expected += 1;
      }
      received += static_cast<int>(count);
    }
  });
  float next = 0;
  int sent = 0;
  while (sent < TOTAL) {
    std::vector<float> chunk(std::min(53, TOTAL - sent));
    for (auto& value : chunk) value = next + static_cast<float>(&value - chunk.data());
    const std::size_t accepted = ring.write(chunk);
    next += static_cast<float>(accepted);
    sent += static_cast<int>(accepted);
  }
  consumer.join();
  EXPECT_FALSE(failed.load());
}

// ---------------------------------------------------------------- 渲染器

/** 渲染 frames 个采样。 */
std::vector<float> render(AudioRenderer& renderer, std::size_t frames) {
  std::vector<float> out(frames, -9.0F);
  renderer.render(out.data(), frames);
  return out;
}

/** 往缓冲里放 count 个常数采样。 */
void fill(SampleRingBuffer& ring, std::size_t count, float value) {
  const std::vector<float> data(count, value);
  ASSERT_EQ(ring.write(data), count);
}

float maxAbs(const std::vector<float>& samples) {
  float result = 0;
  for (const float sample : samples) result = std::max(result, std::abs(sample));
  return result;
}

float maxStep(const std::vector<float>& samples) {
  float result = 0;
  for (std::size_t i = 1; i < samples.size(); ++i) result = std::max(result, std::abs(samples[i] - samples[i - 1]));
  return result;
}

TEST(AudioRenderer, VolumeCurveIsSquaredAndClamped) {
  EXPECT_DOUBLE_EQ(AudioRenderer::gainForVolume(100), 1.0);
  EXPECT_DOUBLE_EQ(AudioRenderer::gainForVolume(0), 0.0);
  EXPECT_DOUBLE_EQ(AudioRenderer::gainForVolume(50), 0.25);
  EXPECT_DOUBLE_EQ(AudioRenderer::gainForVolume(250), 1.0);
}

TEST(AudioRenderer, PlaysTheBufferAtTheSetVolumeAfterAFadeIn) {
  SampleRingBuffer ring(8192);
  AudioRenderer renderer(ring, {});
  fill(ring, 4000, 0.8F);
  renderer.setVolume(100, false);
  renderer.setPlaying(true);
  const auto first = render(renderer, 1000);
  EXPECT_EQ(first[0], 0.0F) << "淡入从静音开始";
  EXPECT_LT(first[10], 0.1F);
  EXPECT_NEAR(first[999], 0.8F, 1e-4F) << "淡入 10 毫秒（480 个采样）之后是满音量";

  renderer.setVolume(50, false);
  render(renderer, 600);
  EXPECT_NEAR(render(renderer, 100)[50], 0.8F * 0.25F, 1e-4F) << "音量 50 的增益是 0.25";

  renderer.setVolume(50, true);
  render(renderer, 600);
  EXPECT_EQ(maxAbs(render(renderer, 100)), 0.0F) << "静音";
  renderer.setVolume(50, false);
  render(renderer, 600);
  EXPECT_NEAR(render(renderer, 10)[0], 0.2F, 1e-4F) << "取消静音恢复音量";
}

TEST(AudioRenderer, VolumeChangesAreSmoothNotAJump) {
  SampleRingBuffer ring(8192);
  AudioRenderer renderer(ring, {});
  fill(ring, 8000, 1.0F);
  renderer.setVolume(100, false);
  renderer.setPlaying(true);
  render(renderer, 1500);
  renderer.setVolume(0, false);
  const auto fade = render(renderer, 1000);
  EXPECT_LE(maxStep(fade), 1.0F / 480.0F + 1e-4F) << "每个采样最多变化 1/480，10 毫秒内降到 0";
  EXPECT_EQ(fade[600], 0.0F);
}

TEST(AudioRenderer, WaitsForTheBufferToFillBeforeStarting) {
  SampleRingBuffer ring(8192);
  AudioRendererOptions options;
  options.primeFill = 1200;
  AudioRenderer renderer(ring, options);
  renderer.setPlaying(true);
  fill(ring, 1000, 0.5F);
  EXPECT_EQ(maxAbs(render(renderer, 500)), 0.0F) << "水位不到预填充量时输出静音";
  EXPECT_EQ(ring.size(), 1000U) << "也不消耗缓冲";
  fill(ring, 300, 0.5F);
  EXPECT_GT(maxAbs(render(renderer, 1000)), 0.4F);
  EXPECT_LT(ring.size(), 1300U);
}

TEST(AudioRenderer, UnderrunFadesTheLastSampleOutInsteadOfClickingAndCountsTheMissingSamples) {
  SampleRingBuffer ring(8192);
  AudioRenderer renderer(ring, {});
  renderer.setPlaying(true);
  fill(ring, 2000, 0.6F);
  render(renderer, 1500);  // 淡入并消耗约 1500 个，剩下约 500 个
  ASSERT_GT(renderer.currentGain(), 0.99);
  EXPECT_EQ(renderer.underrunSamples(), 0U);

  const auto starved = render(renderer, 3000);  // 缓冲被读空
  EXPECT_GT(renderer.underrunSamples(), 1000U);
  EXPECT_LE(maxStep(starved), 0.01F) << "补静音时让最后一个采样衰减，没有突然跳到 0";
  EXPECT_LT(std::abs(starved.back()), 1e-3F);
}

TEST(AudioRenderer, StoppingFadesOutAndResumingFlushesStaleSamplesThenFadesIn) {
  SampleRingBuffer ring(8192);
  AudioRenderer renderer(ring, {});
  renderer.setPlaying(true);
  fill(ring, 3000, 0.7F);
  render(renderer, 1000);

  renderer.setPlaying(false);
  const auto fadeOut = render(renderer, 1000);
  EXPECT_LE(maxStep(fadeOut), 1.0F / 480.0F * 0.7F + 1e-4F);
  EXPECT_EQ(fadeOut.back(), 0.0F);
  EXPECT_EQ(renderer.underrunSamples(), 0U) << "停止播放期间的缓冲不足不算";

  // 继续：先丢弃过时的采样，攒够后淡入。
  renderer.requestFlush();
  renderer.setPlaying(true);
  fill(ring, 100, 0.1F);
  EXPECT_EQ(maxAbs(render(renderer, 200)), 0.0F);
  EXPECT_EQ(ring.size(), 0U) << "旧采样已丢弃";
  fill(ring, 2000, 0.3F);
  const auto resumed = render(renderer, 1000);
  EXPECT_EQ(resumed[0], 0.0F);
  EXPECT_NEAR(resumed[900], 0.3F, 1e-3F);
}

TEST(AudioRenderer, ResampleRatioStaysWithinHalfAPercentAndFollowsTheFillLevel) {
  const double target = 1600;
  const double maxAdjust = 0.005;
  EXPECT_DOUBLE_EQ(AudioRenderer::ratioForFill(target, target, maxAdjust), 1.0);
  EXPECT_GT(AudioRenderer::ratioForFill(target * 1.1, target, maxAdjust), 1.0) << "水位偏高：读得快一点";
  EXPECT_LT(AudioRenderer::ratioForFill(target * 0.9, target, maxAdjust), 1.0) << "水位偏低：读得慢一点";
  EXPECT_DOUBLE_EQ(AudioRenderer::ratioForFill(target * 10, target, maxAdjust), 1.0 + maxAdjust);
  EXPECT_DOUBLE_EQ(AudioRenderer::ratioForFill(0, target, maxAdjust), 1.0 - maxAdjust);
  for (double fill = 0; fill < 9000; fill += 50) {
    const double ratio = AudioRenderer::ratioForFill(fill, target, maxAdjust);
    EXPECT_GE(ratio, 1.0 - maxAdjust);
    EXPECT_LE(ratio, 1.0 + maxAdjust);
  }
}

/**
 * 模拟生产者与消费者：每 10 毫秒生产者放入 producerPerStep 个采样（小数累加），消费者渲染 480 个采样。
 * 返回最后 10 秒的水位范围与欠载、丢弃的数量。
 */
struct DriftResult {
  std::size_t minFill{0};
  std::size_t maxFill{0};
  std::uint64_t underruns{0};
  std::uint64_t dropped{0};
};

DriftResult simulateDrift(double producerPerStep, double maxRatioAdjust, int steps) {
  SampleRingBuffer ring(8192);
  AudioRendererOptions options;
  options.maxRatioAdjust = maxRatioAdjust;
  AudioRenderer renderer(ring, options);
  renderer.setPlaying(true);
  DriftResult result;
  result.minFill = ring.capacity();
  double carry = 0;
  std::vector<float> chunk;
  std::vector<float> out(480);
  for (int step = 0; step < steps; ++step) {
    carry += producerPerStep;
    const auto count = static_cast<std::size_t>(carry);
    carry -= static_cast<double>(count);
    chunk.assign(count, 0.25F);
    const std::size_t accepted = ring.write(chunk);
    result.dropped += count - accepted;
    renderer.render(out.data(), out.size());
    if (step >= steps - 1000) {
      result.minFill = std::min(result.minFill, ring.size());
      result.maxFill = std::max(result.maxFill, ring.size());
    }
  }
  result.underruns = renderer.underrunSamples();
  return result;
}

TEST(AudioRenderer, FillLevelStaysStableWhenTheProducerRunsFasterOrSlowerThanTheDevice) {
  for (const double drift : {1.003, 0.997, 1.0}) {
    const auto result = simulateDrift(480.0 * drift, 0.005, 6000);  // 60 秒
    EXPECT_EQ(result.dropped, 0U) << "漂移 " << drift;
    EXPECT_EQ(result.underruns, 0U) << "漂移 " << drift;
    EXPECT_GT(result.minFill, 600U) << "漂移 " << drift;
    EXPECT_LT(result.maxFill, 2800U) << "漂移 " << drift;
  }
}

TEST(AudioRenderer, WithoutTheCorrectionAFastProducerOverflowsTheBuffer) {
  const auto result = simulateDrift(480.0 * 1.003, 0.0, 6000);
  EXPECT_GT(result.dropped, 0U);
}

// ---------------------------------------------------------------- 输出

TEST(NullAudioOutput, CountsWrittenSamplesAndRemembersTheSettings) {
  NullAudioOutput output(44100);
  EXPECT_EQ(output.sampleRate(), 44100U);
  EXPECT_FALSE(output.isSystemOutput());
  const std::vector<float> data(300, 0.1F);
  EXPECT_EQ(output.write(data), 300U);
  EXPECT_EQ(output.write(data), 300U);
  EXPECT_EQ(output.stats().writtenSamples, 600U);
  EXPECT_EQ(output.bufferedSamples(), 0U);

  output.setVolume(70, true);
  EXPECT_EQ(output.volume(), 70U);
  EXPECT_TRUE(output.muted());
  output.setVolume(500, false);
  EXPECT_EQ(output.volume(), 100U);
  EXPECT_FALSE(output.muted());

  EXPECT_FALSE(output.playing());
  output.setPlaying(true);
  EXPECT_TRUE(output.playing());
  const auto flushes = output.flushCount();
  output.flush();
  EXPECT_EQ(output.flushCount(), flushes + 1);
}

TEST(MiniaudioOutput, OpensTheSystemDeviceOrReportsWhyNotWithoutCrashing) {
  std::string failure;
  const auto output = ember::platform::MiniaudioOutput::open(48000, failure);
  if (output == nullptr) {
    // 没有声卡的环境：给出原因，由调用方降级为空输出。
    EXPECT_FALSE(failure.empty());
    GTEST_SKIP() << "没有可用的声音设备：" << failure;
  }
  EXPECT_TRUE(output->isSystemOutput());
  EXPECT_EQ(output->sampleRate(), 48000U);
  // 设备在运行：写入的采样会被回调取走，设置与清空都不会出错；这里不放出声音（音量 0）。
  output->setVolume(0, true);
  output->setPlaying(true);
  const std::vector<float> silence(2400, 0.0F);
  EXPECT_EQ(output->write(silence), 2400U);
  EXPECT_EQ(output->stats().writtenSamples, 2400U);
  output->flush();
  output->setPlaying(false);
}

}  // namespace
