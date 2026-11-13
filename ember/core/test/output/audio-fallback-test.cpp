/*
 * ------------------------------------------------------------------------
 * 名称：audio-fallback-test.cpp
 * 说明：声音输出打开失败时的降级测试：打开函数返回空指针或抛出异常时，得到空输出并写一条 output.deviceUnavailable 错误日志；打开成功时原样使用、不写日志。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <stdexcept>

#include "../support/plugin-test-support.hpp"
#include "bootstrap/modules/output.hpp"
#include "platform/audio/null-audio-output.hpp"

namespace {

using ember::bootstrap::AudioOpener;
using ember::bootstrap::openAudioOutputOrFallback;
using ember::platform::AudioOutput;
using ember::platform::NullAudioOutput;

TEST(AudioFallback, AnOpenerThatReportsAFailureGivesASilentOutputAndOneErrorLogEntry) {
  ember::test::RecordingErrorLog errors;
  const AudioOpener opener = [](unsigned, std::string& failure) -> std::shared_ptr<AudioOutput> {
    failure = "没有可用的系统声音设备";
    return nullptr;
  };
  const auto output = openAudioOutputOrFallback(opener, 48000, errors);
  ASSERT_NE(output, nullptr);
  EXPECT_FALSE(output->isSystemOutput());
  EXPECT_EQ(output->sampleRate(), 48000U);
  // 降级后的输出照常接收采样与设置，不崩溃。
  const std::vector<float> samples(100, 0.5F);
  EXPECT_EQ(output->write(samples), 100U);
  output->setVolume(10, false);
  output->setPlaying(true);

  ASSERT_EQ(errors.entries.size(), 1U);
  EXPECT_EQ(errors.entries[0].module, "output");
  EXPECT_EQ(errors.entries[0].code, "output.deviceUnavailable");
  EXPECT_EQ(errors.entries[0].context["reason"], "没有可用的系统声音设备");
}

TEST(AudioFallback, AnOpenerThatThrowsIsTreatedTheSameWay) {
  ember::test::RecordingErrorLog errors;
  const AudioOpener opener = [](unsigned, std::string&) -> std::shared_ptr<AudioOutput> { throw std::runtime_error("驱动崩溃"); };
  const auto output = openAudioOutputOrFallback(opener, 48000, errors);
  ASSERT_NE(output, nullptr);
  EXPECT_FALSE(output->isSystemOutput());
  ASSERT_EQ(errors.entries.size(), 1U);
  EXPECT_EQ(errors.entries[0].code, "output.deviceUnavailable");
  EXPECT_EQ(errors.entries[0].context["reason"], "驱动崩溃");
}

TEST(AudioFallback, ASuccessfulOpenIsUsedAsIsWithoutLogging) {
  ember::test::RecordingErrorLog errors;
  unsigned requestedRate = 0;
  const auto device = std::make_shared<NullAudioOutput>(44100);
  const AudioOpener opener = [&](unsigned sampleRate, std::string&) -> std::shared_ptr<AudioOutput> {
    requestedRate = sampleRate;
    return device;
  };
  const auto output = openAudioOutputOrFallback(opener, 44100, errors);
  EXPECT_EQ(output, device);
  EXPECT_EQ(requestedRate, 44100U);
  EXPECT_TRUE(errors.entries.empty());
}

}  // namespace
