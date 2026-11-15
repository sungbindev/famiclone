/*
 * ------------------------------------------------------------------------
 * 名称：local-output-service-test.cpp
 * 说明：输出设置服务的测试：初始值、部分更新保持其他项、越界参数被拒绝且不改变已有设置、变化应用到声音输出、没有声音输出时照常保存；接入首选项：启动时读取、音量保存（静音不保存）、首选项变化立即应用、不形成环。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <variant>

#include "bootstrap/modules/output.hpp"
#include "infra/preferences/preference-registry.hpp"
#include "logic/output/local-output-service.hpp"
#include "platform/audio/null-audio-output.hpp"
#include "shared/errors/core-error.hpp"

namespace {

using ember::infra::preferences::PreferenceRegistry;
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

/** 已登记声音设置项的首选项服务（不读不写文件）。 */
std::shared_ptr<PreferenceRegistry> soundPreferences() {
  auto preferences = std::make_shared<PreferenceRegistry>(ember::infra::preferences::PreferenceRegistryOptions{});
  ember::bootstrap::registerSoundSettings(*preferences);
  return preferences;
}

TEST(LocalOutputServicePreferences, StartsFromTheSavedVolumeTheStartMutedSettingAndTheBufferProfile) {
  auto preferences = soundPreferences();
  preferences->set("sound.volume", std::int64_t{35});
  preferences->set("sound.startMuted", true);
  preferences->set("sound.buffer", std::string("stable"));
  auto audio = std::make_shared<NullAudioOutput>();
  const LocalOutputService service(audio, preferences);
  EXPECT_EQ(service.volume().volume, 35);
  EXPECT_TRUE(service.volume().muted);
  EXPECT_EQ(audio->volume(), 35U);
  EXPECT_TRUE(audio->muted());
  EXPECT_EQ(audio->bufferProfile(), ember::platform::AudioBufferProfile::Stable);
}

TEST(LocalOutputServicePreferences, TheVolumeIsSavedWhenItChangesButTheMuteStateIsNot) {
  auto preferences = soundPreferences();
  auto audio = std::make_shared<NullAudioOutput>();
  LocalOutputService service(audio, preferences);
  service.setVolume(60, true);
  EXPECT_EQ(std::get<std::int64_t>(preferences->value("sound.volume")), 60);
  EXPECT_EQ(std::get<bool>(preferences->value("sound.startMuted")), false);
  service.setVolume(std::nullopt, false);
  EXPECT_EQ(std::get<std::int64_t>(preferences->value("sound.volume")), 60);
}

TEST(LocalOutputServicePreferences, ChangesMadeInThePreferencesAreAppliedRightAway) {
  auto preferences = soundPreferences();
  auto audio = std::make_shared<NullAudioOutput>();
  auto service = std::make_shared<LocalOutputService>(audio, preferences);
  preferences->subscribe([service](const std::string& id, const ember::contracts::preferences::SettingValue& value) { service->applyPreference(id, value); });
  preferences->set("sound.buffer", std::string("low"));
  EXPECT_EQ(audio->bufferProfile(), ember::platform::AudioBufferProfile::LowLatency);
  preferences->set("sound.volume", std::int64_t{10});
  EXPECT_EQ(service->volume().volume, 10);
  EXPECT_EQ(audio->volume(), 10U);
  // 面板改音量保存回去时不会形成环。
  service->setVolume(77, std::nullopt);
  EXPECT_EQ(std::get<std::int64_t>(preferences->value("sound.volume")), 77);
  EXPECT_EQ(audio->volume(), 77U);
}

TEST(LocalOutputServicePreferences, UnknownBufferValuesCountAsStandard) {
  EXPECT_EQ(LocalOutputService::profileFor("low"), ember::platform::AudioBufferProfile::LowLatency);
  EXPECT_EQ(LocalOutputService::profileFor("standard"), ember::platform::AudioBufferProfile::Standard);
  EXPECT_EQ(LocalOutputService::profileFor("stable"), ember::platform::AudioBufferProfile::Stable);
  EXPECT_EQ(LocalOutputService::profileFor(""), ember::platform::AudioBufferProfile::Standard);
}

}  // namespace
