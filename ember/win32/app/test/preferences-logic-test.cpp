/*
 * ------------------------------------------------------------------------
 * 名称：preferences-logic-test.cpp
 * 说明：首选项模块逻辑的测试：纯规则（设置项与机型的读取、显示的分组与行、“机型与 BIOS”分组的位置、整数校验、BIOS 状态文字）、首选项控制器（读取、保存成功与失败、输入校验、恢复默认、选择文件夹、事件、核心重启、断开）、
 *       “机型与 BIOS”控制器（读取、添加、替换、移除、打开目录、事件）与首选项模块的登记项。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-20
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <algorithm>

#include "logic/preferences/bios-controller.hpp"
#include "logic/preferences/preferences-controller.hpp"
#include "logic/preferences/preferences-module.hpp"
#include "logic/preferences/preferences-rules.hpp"
#include "support/fakes.hpp"

namespace ember::win32 {
namespace {

using contracts::CoreReply;
using contracts::PreferenceControl;
using contracts::PreferencesLoad;
using nlohmann::json;

json setting(const std::string& id, const std::string& group, const std::string& kind, const json& value, const json& defaultValue, bool hidden = false) {
  return {{"id", id}, {"group", group}, {"label", id + "标签"}, {"description", ""}, {"kind", kind}, {"hidden", hidden}, {"defaultValue", defaultValue}, {"value", value}};
}

/** 默认的设置清单：外观（主题）、声音（静音、淡入、备注、隐藏的音量）、输入（空的，隐藏项）、打印机（文件夹）。 */
json listing() {
  json theme = setting("appearance.theme", "appearance", "choice", "system", "system");
  theme["options"] = json::array({{{"value", "system"}, {"label", "跟随系统"}}, {{"value", "dark"}, {"label", "深色"}}, {{"value", "light"}, {"label", "浅色"}}});
  json fade = setting("sound.fade", "sound", "integer", 10, 10);
  fade["minimum"] = 0;
  fade["maximum"] = 100;
  json folder = setting("printer.outputDirectory", "printer", "text", "", "");
  folder["picker"] = "directory";
  return {{"groups", json::array({{{"id", "appearance"}, {"label", "外观"}}, {{"id", "sound"}, {"label", "声音"}}, {{"id", "input"}, {"label", "输入"}}, {{"id", "printer"}, {"label", "打印机"}}})},
          {"settings", json::array({theme, setting("sound.startMuted", "sound", "switch", false, false), fade, setting("sound.note", "sound", "text", "abc", ""), setting("sound.volume", "sound", "integer", 100, 100, true),
                                    setting("input.hiddenOnly", "input", "switch", false, false, true), folder})}};
}

json machine(const std::string& id, const std::string& name, bool needsBios, bool supported, const json& bios = nullptr) {
  json item = {{"id", id}, {"name", name}, {"needsBios", needsBios}, {"supported", supported}};
  if (!bios.is_null()) item["bios"] = bios;
  return item;
}

json machinesResult() {
  return {{"machines", json::array({machine("fc", "经典 FC", false, true), machine("sb97", "小霸王 97", true, true), machine("sb2000", "小霸王 2000", true, true, {{"fileName", "SB2K_BIOS.NES"}, {"size", 524304}}),
                                    machine("bbk10", "步步高软驱一号", true, false)})}};
}

const contracts::PreferenceGroupView* group(const contracts::PreferencesView& view, const std::string& id) {
  for (const auto& item : view.groups) {
    if (item.id == id) return &item;
  }
  return nullptr;
}

TEST(PreferencesRules, ShowsOnlyVisibleSettingsAndGroupsWithRowsAndInsertsMachinesAfterInput) {
  const auto parsed = logic::parseSettingListing(listing());
  ASSERT_TRUE(parsed.has_value());
  const auto groups = logic::buildGroupViews(*parsed);
  std::vector<std::string> ids;
  for (const auto& item : groups) ids.push_back(item.id);
  // 输入分组只有隐藏项，不显示；没有输入分组时“机型与 BIOS”排在声音之后
  EXPECT_EQ(ids, (std::vector<std::string>{"appearance", "sound", "machines", "printer"}));
  EXPECT_TRUE(groups[2].machines);
  EXPECT_EQ(groups[2].label, "机型与 BIOS");
  ASSERT_EQ(groups[1].rows.size(), 3U);
  EXPECT_EQ(groups[1].rows[0].control, PreferenceControl::Switch);
  EXPECT_EQ(groups[1].rows[1].control, PreferenceControl::Integer);
  EXPECT_EQ(groups[1].rows[1].text, "10");
  EXPECT_FALSE(groups[1].rows[1].changed);
  EXPECT_EQ(groups[1].rows[2].text, "abc");
  EXPECT_TRUE(groups[1].rows[2].changed);
}

TEST(PreferencesRules, MachinesGroupFollowsInputThenFallsBackToSoundThenTheEnd) {
  auto withInput = listing();
  withInput["settings"].push_back(setting("input.capture", "input", "switch", true, true));
  const auto afterInput = logic::buildGroupViews(*logic::parseSettingListing(withInput));
  std::vector<std::string> ids;
  for (const auto& item : afterInput) ids.push_back(item.id);
  EXPECT_EQ(ids, (std::vector<std::string>{"appearance", "sound", "input", "machines", "printer"}));
  const logic::SettingListing onlyAppearanceListing{{{"appearance", "外观"}}, {}};
  const auto onlyAppearance = logic::buildGroupViews(onlyAppearanceListing);
  ASSERT_EQ(onlyAppearance.size(), 1U);
  EXPECT_EQ(onlyAppearance[0].id, "machines");
}

TEST(PreferencesRules, ParsesChoiceOptionsAndDirectoryPicker) {
  const auto groups = logic::buildGroupViews(*logic::parseSettingListing(listing()));
  const auto& theme = groups[0].rows[0];
  EXPECT_EQ(theme.control, PreferenceControl::Choice);
  ASSERT_EQ(theme.options.size(), 3U);
  EXPECT_EQ(theme.options[1].value, "dark");
  EXPECT_EQ(theme.options[1].label, "深色");
  EXPECT_EQ(theme.text, "system");
  EXPECT_TRUE(groups[3].rows[0].directoryPicker);
  EXPECT_FALSE(groups[1].rows[2].directoryPicker);
}

TEST(PreferencesRules, BadShapesAreRejectedOrSkipped) {
  EXPECT_FALSE(logic::parseSettingListing(json::object()).has_value());
  EXPECT_FALSE(logic::parseSettingListing({{"groups", 1}, {"settings", json::array()}}).has_value());
  auto broken = listing();
  broken["settings"].push_back({{"id", "x"}, {"kind", "unknown"}});
  broken["settings"].push_back(5);
  EXPECT_EQ(logic::parseSettingListing(broken)->settings.size(), 7U);
  EXPECT_FALSE(logic::parseBiosMachines(json::object()).has_value());
}

TEST(PreferencesRules, ParseIntegerChecksFormatAndRange) {
  EXPECT_FALSE(logic::parseInteger("abc", 0, 100).ok);
  EXPECT_EQ(logic::parseInteger("abc", 0, 100).message, "请输入整数。");
  EXPECT_EQ(logic::parseInteger("", 0, 100).message, "请输入整数。");
  EXPECT_EQ(logic::parseInteger("1.5", 0, 100).message, "请输入整数。");
  EXPECT_EQ(logic::parseInteger("-", 0, 100).message, "请输入整数。");
  EXPECT_EQ(logic::parseInteger("500", 0, 100).message, "请输入 0 至 100 之间的整数。");
  EXPECT_EQ(logic::parseInteger("-1", 0, 100).message, "请输入 0 至 100 之间的整数。");
  const auto ok = logic::parseInteger("  30 ", 0, 100);
  EXPECT_TRUE(ok.ok);
  EXPECT_EQ(ok.value, 30);
  EXPECT_TRUE(logic::parseInteger("-5", std::nullopt, std::nullopt).ok);
  EXPECT_EQ(logic::parseInteger("99999999999999999999", 0, 100).message, "请输入 0 至 100 之间的整数。");
  EXPECT_EQ(logic::parseInteger("20", 30, std::nullopt).message, "请输入 30 至 9007199254740991 之间的整数。");
}

TEST(PreferencesRules, BiosStatusAndRowsOnlyForMachinesThatNeedBios) {
  const auto machines = *logic::parseBiosMachines(machinesResult());
  ASSERT_EQ(machines.size(), 4U);
  EXPECT_EQ(logic::biosStatusText(machines[1]), "未添加");
  EXPECT_EQ(logic::biosStatusText(machines[2]), "SB2K_BIOS.NES（512 KB）");
  EXPECT_EQ(logic::biosStatusText(machines[3]), "暂未支持");
  const auto rows = logic::buildBiosRows(machines);
  ASSERT_EQ(rows.size(), 3U);
  EXPECT_EQ(rows[0].machine, "sb97");
  EXPECT_TRUE(rows[1].hasBios);
  EXPECT_FALSE(rows[2].supported);
}

TEST(PreferencesRules, ErrorTextFallsBack) {
  EXPECT_EQ(logic::errorText({"x", "核心未启动"}), "核心未启动");
  EXPECT_EQ(logic::errorText({"x", ""}), "操作失败。");
}

/** 带假核心的首选项控制器。 */
struct PreferencesFixture {
  test::FakeCoreGateway core;
  test::FakeBroker broker{core};
  test::FakeShellApi shell;
  logic::PreferencesController controller{broker, core};
  std::function<void()> detach;
  std::function<void()> unsubscribe;
  int changes{0};

  PreferencesFixture() {
    reply("preferences.list", listing());
    unsubscribe = controller.subscribe([this] { ++changes; });
  }

  void attach() { detach = controller.attach(shell); }
  void reply(const std::string& name, const json& result) {
    core.replies[name] = [result](const json&) { return CoreReply::success(result); };
  }
  void fail(const std::string& name, const std::string& message) {
    core.replies[name] = [message](const json&) { return CoreReply::failure("preferences.invalidValue", message); };
  }
  [[nodiscard]] const test::FakeCoreGateway::Call* last(const std::string& name) const {
    for (auto it = core.calls.rbegin(); it != core.calls.rend(); ++it) {
      if (it->name == name) return &*it;
    }
    return nullptr;
  }
  [[nodiscard]] const contracts::PreferenceRow* row(const std::string& id) const {
    static contracts::PreferencesView view;
    view = controller.view();
    for (const auto& item : view.groups) {
      for (const auto& candidate : item.rows) {
        if (candidate.id == id) return &candidate;
      }
    }
    return nullptr;
  }
};

TEST(PreferencesController, IsLoadingUntilAttachedThenReadsEverything) {
  PreferencesFixture f;
  EXPECT_EQ(f.controller.view().load, PreferencesLoad::Loading);
  EXPECT_TRUE(f.controller.view().groups.empty());
  f.attach();
  const auto view = f.controller.view();
  EXPECT_EQ(view.load, PreferencesLoad::Ready);
  EXPECT_NE(group(view, "appearance"), nullptr);
  EXPECT_NE(group(view, "machines"), nullptr);
  EXPECT_EQ(group(view, "input"), nullptr);
  EXPECT_GE(f.changes, 1);
}

TEST(PreferencesController, ReadFailureShowsTheCoreMessageAndRetriesWhenTheCoreIsReady) {
  PreferencesFixture f;
  f.fail("preferences.list", "核心未启动");
  f.attach();
  EXPECT_EQ(f.controller.view().load, PreferencesLoad::Failed);
  EXPECT_EQ(f.controller.view().message, "核心未启动");
  f.reply("preferences.list", listing());
  f.core.setState(contracts::CoreState::Running);
  EXPECT_EQ(f.controller.view().load, PreferencesLoad::Ready);
}

TEST(PreferencesController, ChoiceAndSwitchSaveThenTakeTheCoreValue) {
  PreferencesFixture f;
  f.attach();
  f.reply("preferences.set", {{"id", "appearance.theme"}, {"value", "dark"}});
  f.controller.setChoice("appearance.theme", "dark");
  EXPECT_EQ(f.last("preferences.set")->params, (json{{"id", "appearance.theme"}, {"value", "dark"}}));
  EXPECT_EQ(f.row("appearance.theme")->text, "dark");
  EXPECT_TRUE(f.row("appearance.theme")->changed);
  f.reply("preferences.set", {{"id", "sound.startMuted"}, {"value", true}});
  f.controller.setSwitch("sound.startMuted", true);
  EXPECT_EQ(f.last("preferences.set")->params, (json{{"id", "sound.startMuted"}, {"value", true}}));
  EXPECT_TRUE(f.row("sound.startMuted")->checked);
  EXPECT_TRUE(f.shell.notices.empty());
}

TEST(PreferencesController, SaveFailureNotifiesTheReasonKeepsTheValueAndStillNotifiesSubscribers) {
  PreferencesFixture f;
  f.attach();
  f.fail("preferences.set", "主题：取值必须是下面之一：system、dark、light");
  const int before = f.changes;
  f.controller.setChoice("appearance.theme", "dark");
  ASSERT_EQ(f.shell.notices.size(), 1U);
  EXPECT_EQ(f.shell.notices[0].first, "主题：取值必须是下面之一：system、dark、light");
  EXPECT_EQ(f.shell.notices[0].second, contracts::NotificationLevel::Error);
  EXPECT_EQ(f.row("appearance.theme")->text, "system");
  EXPECT_GT(f.changes, before);
}

TEST(PreferencesController, SubmitTextChecksIntegersAndSendsTheirNumbers) {
  PreferencesFixture f;
  f.attach();
  const int sets = static_cast<int>(std::count_if(f.core.calls.begin(), f.core.calls.end(), [](const auto& call) { return call.name == "preferences.set"; }));
  EXPECT_EQ(f.controller.submitText("sound.fade", "500"), "请输入 0 至 100 之间的整数。");
  EXPECT_EQ(f.controller.submitText("sound.fade", "abc"), "请输入整数。");
  EXPECT_EQ(static_cast<int>(std::count_if(f.core.calls.begin(), f.core.calls.end(), [](const auto& call) { return call.name == "preferences.set"; })), sets);
  f.reply("preferences.set", {{"id", "sound.fade"}, {"value", 30}});
  EXPECT_EQ(f.controller.submitText("sound.fade", "30"), "");
  EXPECT_EQ(f.last("preferences.set")->params, (json{{"id", "sound.fade"}, {"value", 30}}));
  EXPECT_EQ(f.row("sound.fade")->text, "30");
}

TEST(PreferencesController, SubmitTextSendsTextAsIsAndSkipsUnchangedValues) {
  PreferencesFixture f;
  f.attach();
  f.reply("preferences.set", {{"id", "sound.note"}, {"value", "新 备注"}});
  EXPECT_EQ(f.controller.submitText("sound.note", "新 备注"), "");
  EXPECT_EQ(f.last("preferences.set")->params, (json{{"id", "sound.note"}, {"value", "新 备注"}}));
  f.core.calls.clear();
  const int before = f.changes;
  EXPECT_EQ(f.controller.submitText("sound.note", "新 备注"), "");
  EXPECT_TRUE(f.core.calls.empty());
  // 没有要保存的也通知订阅者，输入框据此回到当前取值
  EXPECT_GT(f.changes, before);
  EXPECT_EQ(f.controller.submitText("sound.fade", " 10 "), "");
  EXPECT_TRUE(f.core.calls.empty());
  EXPECT_EQ(f.controller.submitText("no.such", "x"), "");
}

TEST(PreferencesController, ResetTakesTheDefaultFromTheCore) {
  PreferencesFixture f;
  f.attach();
  f.reply("preferences.reset", {{"id", "sound.note"}, {"value", ""}});
  f.controller.reset("sound.note");
  EXPECT_EQ(f.last("preferences.reset")->params, (json{{"id", "sound.note"}}));
  EXPECT_EQ(f.row("sound.note")->text, "");
  EXPECT_FALSE(f.row("sound.note")->changed);
  f.fail("preferences.reset", "无法恢复");
  f.controller.reset("sound.note");
  EXPECT_EQ(f.shell.notices.back().first, "无法恢复");
}

TEST(PreferencesController, PickDirectorySavesTheChosenPathAndIgnoresCancel) {
  PreferencesFixture f;
  f.attach();
  f.reply("preferences.pickDirectory", {{"canceled", true}});
  f.controller.pickDirectory("printer.outputDirectory");
  EXPECT_EQ(f.last("preferences.set"), nullptr);
  f.reply("preferences.pickDirectory", {{"canceled", false}, {"path", "D:/prints"}});
  f.reply("preferences.set", {{"id", "printer.outputDirectory"}, {"value", "D:/prints"}});
  f.controller.pickDirectory("printer.outputDirectory");
  EXPECT_EQ(f.last("preferences.set")->params, (json{{"id", "printer.outputDirectory"}, {"value", "D:/prints"}}));
  EXPECT_EQ(f.row("printer.outputDirectory")->text, "D:/prints");
  EXPECT_TRUE(f.shell.notices.empty());
}

TEST(PreferencesController, EventsKeepValuesCurrentAndIgnoreOtherEvents) {
  PreferencesFixture f;
  f.attach();
  f.core.emitEvent("preferences.changed", {{"id", "appearance.theme"}, {"value", "light"}});
  EXPECT_EQ(f.row("appearance.theme")->text, "light");
  const int before = f.changes;
  f.core.emitEvent("preferences.changed", nullptr);
  f.core.emitEvent("runtime.stateChanged", {{"id", "appearance.theme"}, {"value", "dark"}});
  f.core.emitEvent("preferences.changed", {{"id", "appearance.nothing"}, {"value", 1}});
  EXPECT_EQ(f.row("appearance.theme")->text, "light");
  EXPECT_EQ(f.changes, before + 1);
}

TEST(PreferencesController, CoreRestartReloadsAndDetachStopsEverything) {
  PreferencesFixture f;
  f.attach();
  f.core.setState(contracts::CoreState::Starting);
  EXPECT_EQ(f.controller.view().load, PreferencesLoad::Loading);
  EXPECT_TRUE(f.controller.view().groups.empty());
  f.core.setState(contracts::CoreState::Running);
  EXPECT_EQ(f.controller.view().load, PreferencesLoad::Ready);
  f.detach();
  EXPECT_EQ(f.core.subscriberCount(), 0U);
  f.core.calls.clear();
  f.controller.setChoice("appearance.theme", "dark");
  EXPECT_TRUE(f.core.calls.empty());
}

/** 带假核心的“机型与 BIOS”控制器。 */
struct BiosFixture {
  test::FakeCoreGateway core;
  test::FakeBroker broker{core};
  test::FakeShellApi shell;
  logic::BiosController controller{broker, core};
  std::function<void()> detach;
  int changes{0};
  std::function<void()> unsubscribe;

  BiosFixture() {
    reply("machines.list", machinesResult());
    unsubscribe = controller.subscribe([this] { ++changes; });
  }

  void attach() { detach = controller.attach(shell); }
  void reply(const std::string& name, const json& result) {
    core.replies[name] = [result](const json&) { return CoreReply::success(result); };
  }
  void fail(const std::string& name, const std::string& message) {
    core.replies[name] = [message](const json&) { return CoreReply::failure("machines.biosInvalid", message); };
  }
  [[nodiscard]] const test::FakeCoreGateway::Call* last(const std::string& name) const {
    for (auto it = core.calls.rbegin(); it != core.calls.rend(); ++it) {
      if (it->name == name) return &*it;
    }
    return nullptr;
  }
};

TEST(BiosController, ListsOnlyMachinesThatNeedBios) {
  BiosFixture f;
  EXPECT_EQ(f.controller.view().load, PreferencesLoad::Loading);
  f.attach();
  const auto view = f.controller.view();
  EXPECT_EQ(view.load, PreferencesLoad::Ready);
  ASSERT_EQ(view.rows.size(), 3U);
  EXPECT_EQ(view.rows[1].status, "SB2K_BIOS.NES（512 KB）");
  EXPECT_FALSE(view.rows[2].supported);
}

TEST(BiosController, ChooseAddsOrReplacesAndNotifiesWithTheFileName) {
  BiosFixture f;
  f.attach();
  f.reply("machines.openBios", {{"canceled", false}, {"machine", machine("sb97", "小霸王 97", true, true, {{"fileName", "SB97.BIN"}, {"size", 524288}})}});
  f.controller.choose("sb97");
  EXPECT_EQ(f.last("machines.openBios")->params, (json{{"machine", "sb97"}}));
  ASSERT_EQ(f.shell.notices.size(), 1U);
  EXPECT_EQ(f.shell.notices[0].first, "已添加 小霸王 97 的 BIOS：SB97.BIN");
  EXPECT_EQ(f.shell.notices[0].second, contracts::NotificationLevel::Success);
  f.reply("machines.openBios", {{"canceled", false}, {"machine", machine("sb2000", "小霸王 2000", true, true, {{"fileName", "NEW.NES"}, {"size", 524304}})}});
  f.controller.choose("sb2000");
  EXPECT_EQ(f.shell.notices.back().first, "已替换 小霸王 2000 的 BIOS：NEW.NES");
}

TEST(BiosController, CancelIsSilentAndErrorsAreNotified) {
  BiosFixture f;
  f.attach();
  f.reply("machines.openBios", {{"canceled", true}});
  f.controller.choose("sb97");
  EXPECT_TRUE(f.shell.notices.empty());
  f.fail("machines.openBios", "小霸王 2000 的 BIOS 的 PRG 必须是 512KB");
  f.controller.choose("sb2000");
  ASSERT_EQ(f.shell.notices.size(), 1U);
  EXPECT_EQ(f.shell.notices[0].first, "小霸王 2000 的 BIOS 的 PRG 必须是 512KB");
  EXPECT_EQ(f.shell.notices[0].second, contracts::NotificationLevel::Error);
}

TEST(BiosController, RemoveNotifiesAndOpenDirectoryReportsFailure) {
  BiosFixture f;
  f.attach();
  f.controller.remove("sb2000");
  EXPECT_EQ(f.last("machines.removeBios")->params, (json{{"machine", "sb2000"}}));
  EXPECT_EQ(f.shell.notices.back().first, "已移除 小霸王 2000 的 BIOS");
  f.controller.openDirectory();
  EXPECT_NE(f.last("machines.openBiosDirectory"), nullptr);
  const std::size_t notices = f.shell.notices.size();
  f.fail("machines.openBiosDirectory", "无法打开 BIOS 目录：没有权限");
  f.controller.openDirectory();
  ASSERT_EQ(f.shell.notices.size(), notices + 1);
  EXPECT_EQ(f.shell.notices.back().first, "无法打开 BIOS 目录：没有权限");
}

TEST(BiosController, MachinesChangedReloadsAndCoreRestartClears) {
  BiosFixture f;
  f.attach();
  f.reply("machines.list", {{"machines", json::array({machine("sb97", "小霸王 97", true, true, {{"fileName", "A.BIN"}, {"size", 524288}})})}});
  f.core.emitEvent("machines.changed", {{"machine", "sb97"}});
  ASSERT_EQ(f.controller.view().rows.size(), 1U);
  EXPECT_TRUE(f.controller.view().rows[0].hasBios);
  f.core.setState(contracts::CoreState::Starting);
  EXPECT_EQ(f.controller.view().load, PreferencesLoad::Loading);
  EXPECT_TRUE(f.controller.view().rows.empty());
  f.detach();
  EXPECT_EQ(f.core.subscriberCount(), 0U);
}

TEST(PreferencesModule, RegistersTheDialogTheCommandAndTheSettingsMenuItem) {
  test::FakeCoreGateway core;
  test::FakeBroker broker{core};
  logic::PreferencesController preferences(broker, core);
  logic::BiosController bios(broker, core);
  test::FakeUserDataLocation location;
  test::FakeDirectory directory;
  test::FakeConfirmation confirmation;
  logic::UserDataController userData(location, directory, confirmation);
  const auto module = logic::createPreferencesModule(preferences, bios, userData);
  EXPECT_EQ(module.name, "preferences");
  ASSERT_EQ(module.dialogs.size(), 1U);
  EXPECT_EQ(module.dialogs[0].id, "preferences.preferences");
  EXPECT_EQ(module.dialogs[0].title, "首选项");
  EXPECT_EQ(module.dialogs[0].width, 720);
  EXPECT_EQ(module.dialogs[0].height, 520);
  EXPECT_EQ(module.dialogs[0].minWidth, 560);
  EXPECT_EQ(module.dialogs[0].minHeight, 360);
  ASSERT_EQ(module.commands.size(), 1U);
  EXPECT_EQ(module.commands[0].id, "preferences.open");
  EXPECT_EQ(module.commands[0].defaultShortcut, "Ctrl+,");
  test::FakeShellApi shell;
  module.commands[0].run(shell);
  EXPECT_EQ(shell.dialogs, (std::vector<std::string>{"preferences.preferences"}));
  ASSERT_EQ(module.menuItems.size(), 1U);
  EXPECT_EQ(module.menuItems[0].menu, "settings");
  EXPECT_EQ(module.menuItems[0].command, "preferences.open");
}

TEST(PreferencesModule, AttachConnectsBothControllersAndDetachDisconnectsThem) {
  test::FakeCoreGateway core;
  test::FakeBroker broker{core};
  test::FakeShellApi shell;
  logic::PreferencesController preferences(broker, core);
  logic::BiosController bios(broker, core);
  test::FakeUserDataLocation location;
  test::FakeDirectory directory;
  test::FakeConfirmation confirmation;
  logic::UserDataController userData(location, directory, confirmation);
  const auto module = logic::createPreferencesModule(preferences, bios, userData);
  const auto detach = module.attach(shell);
  EXPECT_EQ(core.subscriberCount(), 4U);
  detach();
  EXPECT_EQ(core.subscriberCount(), 0U);
}

TEST(UserDataController, ShowsTheDirectoryAndTheSwitchButtonForTheOtherPlace) {
  test::FakeUserDataLocation location;
  test::FakeDirectory directory;
  test::FakeConfirmation confirmation;
  logic::UserDataController controller(location, directory, confirmation);
  auto view = controller.view();
  EXPECT_EQ(view.directory, location.current.directory);
  EXPECT_EQ(view.switchLabel, "改用软件所在目录");
  EXPECT_TRUE(view.unavailableReason.empty());
  location.current.place = shared::UserDataPlace::ExecutableDirectory;
  location.current.directory = "D:\\Ember\\ember-data";
  view = controller.view();
  EXPECT_EQ(view.switchLabel, "改用用户数据目录");
}

TEST(UserDataController, SwitchIsDisabledWhenTheTargetIsUnavailableOrTheDirectoryIsOverridden) {
  test::FakeUserDataLocation location;
  test::FakeDirectory directory;
  test::FakeConfirmation confirmation;
  logic::UserDataController controller(location, directory, confirmation);
  location.executableUnavailable = "软件所在的目录不能写入";
  EXPECT_EQ(controller.view().unavailableReason, "软件所在的目录不能写入");
  location.executableUnavailable.clear();
  location.current.overridden = true;
  EXPECT_FALSE(controller.view().unavailableReason.empty());

  test::FakeShellApi shell;
  const auto detach = controller.attach(shell);
  controller.switchPlace();
  ASSERT_EQ(shell.notices.size(), 1U);
  EXPECT_EQ(shell.notices[0].second, contracts::NotificationLevel::Error);
  EXPECT_TRUE(confirmation.asked.empty());
  EXPECT_TRUE(location.moves.empty());
  EXPECT_EQ(shell.quits, 0);
  detach();
}

TEST(UserDataController, SwitchAsksThenStartsTheMoveAndQuits) {
  test::FakeUserDataLocation location;
  test::FakeDirectory directory;
  test::FakeConfirmation confirmation;
  test::FakeShellApi shell;
  logic::UserDataController controller(location, directory, confirmation);
  const auto detach = controller.attach(shell);

  confirmation.answer = false;
  controller.switchPlace();
  ASSERT_EQ(confirmation.asked.size(), 1U);
  EXPECT_NE(confirmation.asked[0].find("D:\\Ember\\ember-data"), std::string::npos);
  EXPECT_TRUE(location.moves.empty());
  EXPECT_EQ(shell.quits, 0);

  confirmation.answer = true;
  controller.switchPlace();
  EXPECT_EQ(location.moves, (std::vector<shared::UserDataPlace>{shared::UserDataPlace::ExecutableDirectory}));
  EXPECT_EQ(shell.quits, 1);
  EXPECT_TRUE(shell.notices.empty());
  detach();
}

TEST(UserDataController, FailedStartIsReportedAndTheAppKeepsRunning) {
  test::FakeUserDataLocation location;
  test::FakeDirectory directory;
  test::FakeConfirmation confirmation;
  test::FakeShellApi shell;
  logic::UserDataController controller(location, directory, confirmation);
  const auto detach = controller.attach(shell);
  location.moveError = "无法启动新的进程";
  controller.switchPlace();
  EXPECT_EQ(shell.quits, 0);
  ASSERT_EQ(shell.notices.size(), 1U);
  EXPECT_EQ(shell.notices[0].first, "无法启动新的进程");
  detach();
}

TEST(UserDataController, OpenDirectoryReportsTheErrorWithANotice) {
  test::FakeUserDataLocation location;
  test::FakeDirectory directory;
  test::FakeConfirmation confirmation;
  test::FakeShellApi shell;
  logic::UserDataController controller(location, directory, confirmation);
  const auto detach = controller.attach(shell);
  controller.openDirectory();
  EXPECT_EQ(directory.opened, 1);
  EXPECT_TRUE(shell.notices.empty());
  directory.error = "打不开";
  controller.openDirectory();
  ASSERT_EQ(shell.notices.size(), 1U);
  EXPECT_EQ(shell.notices[0].first, "打不开");
  detach();
}

}  // namespace
}  // namespace ember::win32
