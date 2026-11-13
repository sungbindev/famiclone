/*
 * ------------------------------------------------------------------------
 * 名称：local-output-service-test.cpp
 * 说明：输出设置服务的测试：初始值、部分更新保持其他项、越界参数被拒绝且不改变已有设置、变化应用到声音输出、没有声音输出时照常保存。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <memory>

#include "logic/output/local-output-service.hpp"
#include "platform/audio/null-audio-output.hpp"
#include "shared/errors/core-error.hpp"

namespace {

using ember::logic::output::LocalOutputService;
using ember::platform::NullAudioOutput;

TEST(LocalOutputService, StartsAtFullVolumeUnmutedAndAppliesItToTheAudioOutput) {
  auto audio = std::make_shared<NullAudioOutput>();
  audio->setVolume(3, true);
  const LocalOutputService service(audio);
  EXPECT_EQ(service.volume().volume, 100);
  EXPECT_FALSE(service.volume().muted);
  EXPECT_EQ(audio->volume(), 100U);
  EXPECT_FALSE(audio->muted());
}

TEST(LocalOutputService, UpdatesOnlyTheGivenSettingsAndAppliesThemToTheAudioOutput) {
  auto audio = std::make_shared<NullAudioOutput>();
  LocalOutputService service(audio);

  auto result = service.setVolume(40, std::nullopt);
  EXPECT_EQ(result.volume, 40);
  EXPECT_FALSE(result.muted);
  EXPECT_EQ(audio->volume(), 40U);

  result = service.setVolume(std::nullopt, true);
  EXPECT_EQ(result.volume, 40);
  EXPECT_TRUE(result.muted);
  EXPECT_TRUE(audio->muted());
  EXPECT_EQ(audio->volume(), 40U);

  result = service.setVolume(0, false);
  EXPECT_EQ(result.volume, 0);
  EXPECT_FALSE(result.muted);

  result = service.setVolume(std::nullopt, std::nullopt);
  EXPECT_EQ(result.volume, 0);
  EXPECT_EQ(service.volume().volume, 0);
}

TEST(LocalOutputService, RejectsOutOfRangeVolumesWithoutChangingTheSettings) {
  auto audio = std::make_shared<NullAudioOutput>();
  LocalOutputService service(audio);
  service.setVolume(55, true);
  EXPECT_THROW(service.setVolume(101, false), ember::shared::SchemaError);
  EXPECT_THROW(service.setVolume(-1, false), ember::shared::SchemaError);
  EXPECT_EQ(service.volume().volume, 55);
  EXPECT_TRUE(service.volume().muted);
  EXPECT_EQ(audio->volume(), 55U);
  EXPECT_TRUE(audio->muted());
}

TEST(LocalOutputService, KeepsTheSettingsEvenWithoutAnAudioOutput) {
  LocalOutputService service(nullptr);
  EXPECT_EQ(service.setVolume(20, true).volume, 20);
  EXPECT_TRUE(service.volume().muted);
}

}  // namespace
