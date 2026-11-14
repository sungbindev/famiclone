/*
 * ------------------------------------------------------------------------
 * 名称：preference-registry-test.cpp
 * 说明：首选项服务的测试：登记与登记检查、取值校验（种类、可选值、范围）、修改与恢复默认、事件与订阅者、保存到文件与重新读取、坏文件与坏取值的处理、写不进去时的降级。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <atomic>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <variant>
#include <vector>

#include <nlohmann/json.hpp>

#include "../support/plugin-test-support.hpp"
#include "contracts/preferences/preferences-error.hpp"
#include "infra/preferences/preference-registry.hpp"
#include "shared/errors/core-error.hpp"

namespace {

using ember::infra::preferences::PreferenceRegistry;
using ember::infra::preferences::PreferenceRegistryOptions;
namespace api = ember::contracts::preferences;

/** 一个带文件、事件记录与错误日志记录的首选项服务，登记了三种种类各一个设置项。 */
struct Rig {
  explicit Rig(const std::filesystem::path& file) : events(std::make_shared<ember::test::RecordingEventSink>()), errors(std::make_shared<ember::test::RecordingErrorLog>()) {
    PreferenceRegistryOptions options;
    options.file = file;
    options.events = events;
    options.errors = errors;
    registry = std::make_unique<PreferenceRegistry>(std::move(options));
    registry->addGroup({"sound", "声音"});
    registry->addSetting({.id = "sound.buffer",
                          .group = "sound",
                          .label = "缓冲大小",
                          .description = "说明",
                          .kind = api::SettingKind::Choice,
                          .options = {{"low", "低延迟"}, {"standard", "标准"}, {"stable", "稳定"}},
                          .defaultValue = std::string("standard")});
    registry->addSetting({.id = "sound.muted", .group = "sound", .label = "静音", .kind = api::SettingKind::Switch, .defaultValue = false});
    registry->addSetting({.id = "sound.volume", .group = "sound", .label = "音量", .kind = api::SettingKind::Integer, .minimum = 0, .maximum = 100, .hidden = true, .defaultValue = std::int64_t{100}});
  }

  std::shared_ptr<ember::test::RecordingEventSink> events;
  std::shared_ptr<ember::test::RecordingErrorLog> errors;
  std::unique_ptr<PreferenceRegistry> registry;
};

std::string codeOf(const std::function<void()>& call) {
  try {
    call();
  } catch (const ember::shared::CoreError& error) {
    return std::string(error.code());
  }
  return {};
}

TEST(PreferenceRegistry, ListsGroupsAndSettingsInRegistrationOrderWithDefaults) {
  Rig rig({});
  ASSERT_EQ(rig.registry->groups().size(), 1U);
  EXPECT_EQ(rig.registry->groups()[0].label, "声音");
  const auto entries = rig.registry->entries();
  ASSERT_EQ(entries.size(), 3U);
  EXPECT_EQ(entries[0].definition.id, "sound.buffer");
  EXPECT_EQ(std::get<std::string>(entries[0].value), "standard");
  EXPECT_EQ(std::get<bool>(entries[1].value), false);
  EXPECT_EQ(std::get<std::int64_t>(entries[2].value), 100);
  EXPECT_TRUE(entries[2].definition.hidden);
  EXPECT_FALSE(entries[0].definition.hidden);
}

TEST(PreferenceRegistry, SetChangesTheValueAndPublishesAnEventAndNotifiesSubscribers) {
  Rig rig({});
  std::vector<std::pair<std::string, api::SettingValue>> heard;
  rig.registry->subscribe([&](const std::string& id, const api::SettingValue& value) { heard.emplace_back(id, value); });
  EXPECT_EQ(std::get<std::string>(rig.registry->set("sound.buffer", std::string("low"))), "low");
  EXPECT_EQ(std::get<std::string>(rig.registry->value("sound.buffer")), "low");
  ASSERT_EQ(rig.events->events.size(), 1U);
  EXPECT_EQ(rig.events->events[0].first, "preferences.changed");
  EXPECT_EQ(rig.events->events[0].second, nlohmann::json({{"id", "sound.buffer"}, {"value", "low"}}));
  ASSERT_EQ(heard.size(), 1U);
  EXPECT_EQ(heard[0].first, "sound.buffer");
  EXPECT_EQ(std::get<std::string>(heard[0].second), "low");
}

TEST(PreferenceRegistry, SettingTheSameValueAgainDoesNothing) {
  Rig rig({});
  int heard = 0;
  rig.registry->subscribe([&](const std::string&, const api::SettingValue&) { ++heard; });
  rig.registry->set("sound.muted", false);
  rig.registry->reset("sound.buffer");
  EXPECT_EQ(heard, 0);
  EXPECT_TRUE(rig.events->events.empty());
}

TEST(PreferenceRegistry, ResetRestoresTheDefaultAndTellsEveryone) {
  Rig rig({});
  rig.registry->set("sound.volume", std::int64_t{30});
  rig.events->events.clear();
  EXPECT_EQ(std::get<std::int64_t>(rig.registry->reset("sound.volume")), 100);
  ASSERT_EQ(rig.events->events.size(), 1U);
  EXPECT_EQ(rig.events->events[0].second["value"], 100);
}

TEST(PreferenceRegistry, RejectsUnknownSettingsAndValuesThatDoNotFit) {
  Rig rig({});
  EXPECT_EQ(codeOf([&] { static_cast<void>(rig.registry->value("sound.nothing")); }), api::ERROR_UNKNOWN);
  EXPECT_EQ(codeOf([&] { rig.registry->set("sound.nothing", true); }), api::ERROR_UNKNOWN);
  EXPECT_EQ(codeOf([&] { rig.registry->reset("sound.nothing"); }), api::ERROR_UNKNOWN);
  EXPECT_EQ(codeOf([&] { rig.registry->set("sound.buffer", std::string("huge")); }), api::ERROR_INVALID_VALUE);
  EXPECT_EQ(codeOf([&] { rig.registry->set("sound.buffer", true); }), api::ERROR_INVALID_VALUE);
  EXPECT_EQ(codeOf([&] { rig.registry->set("sound.muted", std::string("yes")); }), api::ERROR_INVALID_VALUE);
  EXPECT_EQ(codeOf([&] { rig.registry->set("sound.muted", std::int64_t{1}); }), api::ERROR_INVALID_VALUE);
  EXPECT_EQ(codeOf([&] { rig.registry->set("sound.volume", std::int64_t{101}); }), api::ERROR_INVALID_VALUE);
  EXPECT_EQ(codeOf([&] { rig.registry->set("sound.volume", std::int64_t{-1}); }), api::ERROR_INVALID_VALUE);
  EXPECT_EQ(codeOf([&] { rig.registry->set("sound.volume", true); }), api::ERROR_INVALID_VALUE);
  EXPECT_TRUE(rig.events->events.empty());
  EXPECT_EQ(std::get<std::string>(rig.registry->value("sound.buffer")), "standard");
}

TEST(PreferenceRegistry, InvalidValueMessagesNameTheSettingAndTheAllowedValues) {
  Rig rig({});
  try {
    rig.registry->set("sound.buffer", std::string("huge"));
    FAIL() << "应当抛出异常";
  } catch (const ember::shared::CoreError& error) {
    const std::string text = error.what();
    EXPECT_NE(text.find("缓冲大小"), std::string::npos) << text;
    EXPECT_NE(text.find("low、standard、stable"), std::string::npos) << text;
  }
}

TEST(PreferenceRegistry, ValuesAreSavedToTheFileAndReadBackByANewInstance) {
  ember::test::TempDir directory;
  const auto file = directory.path() / "config" / "preferences.json";
  {
    Rig first(file);
    first.registry->set("sound.buffer", std::string("stable"));
    first.registry->set("sound.volume", std::int64_t{42});
  }
  std::ifstream stream(file, std::ios::binary);
  const auto document = nlohmann::json::parse(stream);
  EXPECT_EQ(document["version"], 1);
  EXPECT_EQ(document["values"]["sound.buffer"], "stable");
  EXPECT_EQ(document["values"]["sound.volume"], 42);
  EXPECT_FALSE(std::filesystem::exists(file.string() + ".tmp"));

  Rig second(file);
  EXPECT_EQ(std::get<std::string>(second.registry->value("sound.buffer")), "stable");
  EXPECT_EQ(std::get<std::int64_t>(second.registry->value("sound.volume")), 42);
  EXPECT_EQ(std::get<bool>(second.registry->value("sound.muted")), false);
  EXPECT_TRUE(second.errors->entries.empty());
}

TEST(PreferenceRegistry, StoredValuesThatDoNotFitFallBackToTheDefaultAndAreLogged) {
  ember::test::TempDir directory;
  const auto file = directory.path() / "preferences.json";
  std::ofstream(file) << R"({"version":1,"values":{"sound.buffer":"huge","sound.volume":250,"sound.muted":true,"other.unknown":1}})";
  Rig rig(file);
  EXPECT_EQ(std::get<std::string>(rig.registry->value("sound.buffer")), "standard");
  EXPECT_EQ(std::get<std::int64_t>(rig.registry->value("sound.volume")), 100);
  EXPECT_EQ(std::get<bool>(rig.registry->value("sound.muted")), true);
  ASSERT_EQ(rig.errors->entries.size(), 2U);
  EXPECT_EQ(rig.errors->entries[0].code, "preferences.valueInvalid");
}

TEST(PreferenceRegistry, ABrokenFileIsIgnoredWithAnErrorLogEntry) {
  ember::test::TempDir directory;
  for (const char* content : {"垃圾", "[]", R"({"version":2,"values":{}})", R"({"version":1})", R"({"version":1,"values":[]})"}) {
    const auto file = directory.path() / "broken.json";
    std::ofstream(file, std::ios::trunc) << content;
    Rig rig(file);
    EXPECT_EQ(std::get<std::string>(rig.registry->value("sound.buffer")), "standard") << content;
    ASSERT_EQ(rig.errors->entries.size(), 1U) << content;
    EXPECT_EQ(rig.errors->entries[0].code, "preferences.fileInvalid") << content;
  }
}

TEST(PreferenceRegistry, AFileThatCannotBeWrittenKeepsTheValueInMemoryAndLogsAnError) {
  ember::test::TempDir directory;
  // 上级路径是一个普通文件，目录建不出来。
  const auto blocker = directory.write("blocker", {1, 2, 3});
  Rig rig(blocker / "preferences.json");
  EXPECT_EQ(std::get<std::string>(rig.registry->set("sound.buffer", std::string("low"))), "low");
  EXPECT_EQ(std::get<std::string>(rig.registry->value("sound.buffer")), "low");
  ASSERT_EQ(rig.errors->entries.size(), 1U);
  EXPECT_EQ(rig.errors->entries[0].code, "preferences.saveFailed");
  EXPECT_EQ(rig.events->events.size(), 1U);
}

TEST(PreferenceRegistry, RegistrationMistakesAreProgramErrors) {
  Rig rig({});
  EXPECT_THROW(rig.registry->addGroup({"sound", "重复"}), std::logic_error);
  EXPECT_THROW(rig.registry->addGroup({"", "没有标识"}), std::logic_error);
  EXPECT_THROW(rig.registry->addSetting({.id = "sound.muted", .group = "sound", .label = "重复", .kind = api::SettingKind::Switch, .defaultValue = false}), std::logic_error);
  EXPECT_THROW(rig.registry->addSetting({.id = "video.size", .group = "video", .label = "分组没登记", .kind = api::SettingKind::Switch, .defaultValue = false}), std::logic_error);
  EXPECT_THROW(rig.registry->addSetting({.id = "other.size", .group = "sound", .label = "标识前缀不是分组", .kind = api::SettingKind::Switch, .defaultValue = false}), std::logic_error);
  EXPECT_THROW(rig.registry->addSetting({.id = "sound.nolabel", .group = "sound", .label = "", .kind = api::SettingKind::Switch, .defaultValue = false}), std::logic_error);
  EXPECT_THROW(rig.registry->addSetting({.id = "sound.empty", .group = "sound", .label = "没有可选值", .kind = api::SettingKind::Choice, .defaultValue = std::string("a")}), std::logic_error);
  EXPECT_THROW(rig.registry->addSetting({.id = "sound.twice", .group = "sound", .label = "可选值重复", .kind = api::SettingKind::Choice, .options = {{"a", "甲"}, {"a", "乙"}}, .defaultValue = std::string("a")}),
               std::logic_error);
  EXPECT_THROW(rig.registry->addSetting({.id = "sound.badDefault", .group = "sound", .label = "默认值不在可选值里", .kind = api::SettingKind::Choice, .options = {{"a", "甲"}}, .defaultValue = std::string("b")}),
               std::logic_error);
  EXPECT_THROW(rig.registry->addSetting({.id = "sound.range", .group = "sound", .label = "范围反了", .kind = api::SettingKind::Integer, .minimum = 5, .maximum = 1, .defaultValue = std::int64_t{3}}), std::logic_error);
  EXPECT_THROW(rig.registry->addSetting({.id = "sound.badInt", .group = "sound", .label = "默认值超出范围", .kind = api::SettingKind::Integer, .minimum = 0, .maximum = 10, .defaultValue = std::int64_t{11}}),
               std::logic_error);
  EXPECT_EQ(rig.registry->entries().size(), 3U);
}

TEST(PreferenceRegistry, SettingsCanBeChangedFromSeveralThreadsAtOnce) {
  // 记录型的事件发布不是线程安全的，这里不接事件。
  PreferenceRegistry registry(PreferenceRegistryOptions{});
  registry.addGroup({"sound", "声音"});
  registry.addSetting({.id = "sound.volume", .group = "sound", .label = "音量", .kind = api::SettingKind::Integer, .minimum = 0, .maximum = 100, .defaultValue = std::int64_t{100}});
  std::atomic<int> heard{0};
  registry.subscribe([&](const std::string&, const api::SettingValue&) { ++heard; });
  std::vector<std::thread> threads;
  for (int index = 0; index < 4; ++index) {
    threads.emplace_back([&, index] {
      for (int round = 0; round < 50; ++round) registry.set("sound.volume", std::int64_t{(index * 50 + round) % 100});
    });
  }
  for (auto& thread : threads) thread.join();
  const auto volume = std::get<std::int64_t>(registry.value("sound.volume"));
  EXPECT_GE(volume, 0);
  EXPECT_LE(volume, 99);
  EXPECT_GT(heard.load(), 0);
}

}  // namespace
