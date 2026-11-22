/*
 * ------------------------------------------------------------------------
 * 名称：system-logic-test.cpp
 * 说明：系统模块逻辑的测试：升级控制器（读取初始状态、菜单名称与可用状态、手动与自动检查的通知差别、重启安装、不支持升级）、自动检查更新（延时、设置开关、只检查一次、停止）、升级后的更新记录提示、关于控制器（固定文字、版本读取与失败、核心重启后重新读取、打开主页地址与项目地址）与系统模块的登记项（帮助菜单、对话框、标题栏“更新”按钮、请求与失败通知）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <memory>

#include "logic/system/about-controller.hpp"
#include "logic/system/auto-update-check.hpp"
#include "logic/system/release-notes-prompt.hpp"
#include "logic/system/system-module.hpp"
#include "logic/system/update-controller.hpp"
#include "support/fakes.hpp"

namespace ember::win32 {
namespace {

using contracts::CoreReply;
using contracts::NotificationLevel;
using nlohmann::json;

json statusJson(const std::string& state, const std::string& version = "", const std::string& message = "") {
  json status = {{"state", state}};
  if (!version.empty()) status["version"] = version;
  if (!message.empty()) status["message"] = message;
  return {{"status", status}};
}

struct UpdateFixture {
  test::FakeCoreGateway core;
  test::FakeBroker broker{core};
  test::FakeUpdater updater;
  test::FakeShellApi shell;
  logic::UpdateController controller{broker, updater};
  std::function<void()> detach;

  UpdateFixture() {
    reply("system.updateStatus", statusJson("idle"));
    reply("system.checkUpdate", statusJson("checking"));
  }

  void attach() { detach = controller.attach(shell); }
  void reply(const std::string& name, const json& result) {
    core.replies[name] = [result](const json&) { return CoreReply::success(result); };
  }
  void fail(const std::string& name, const std::string& message) {
    core.replies[name] = [message](const json&) { return CoreReply::failure("system.updateNotReady", message); };
  }
  void push(const std::string& state, const std::string& version = "", const std::string& message = "") {
    contracts::UpdateStatus status{state, std::nullopt, std::nullopt};
    if (!version.empty()) status.version = version;
    if (!message.empty()) status.message = message;
    updater.push(status);
  }
};

TEST(UpdateController, ReadsTheInitialStatusWithoutNotifyingAndNamesTheMenuItem) {
  UpdateFixture f;
  f.reply("system.updateStatus", statusJson("ready", "1.2.3"));
  f.attach();
  EXPECT_TRUE(f.controller.isReady());
  EXPECT_EQ(f.controller.title(), "重启并安装更新（1.2.3）");
  EXPECT_TRUE(f.controller.canRun());
  EXPECT_TRUE(f.shell.notices.empty());
  EXPECT_GE(f.shell.refreshes, 1);
}

TEST(UpdateController, TitleAndAvailabilityFollowTheState) {
  UpdateFixture f;
  f.attach();
  EXPECT_EQ(f.controller.title(), "检查更新");
  f.push("checking");
  EXPECT_EQ(f.controller.title(), "正在检查更新…");
  EXPECT_FALSE(f.controller.canRun());
  f.push("downloading");
  EXPECT_EQ(f.controller.title(), "正在下载更新…");
  EXPECT_FALSE(f.controller.canRun());
  f.push("ready");
  EXPECT_EQ(f.controller.title(), "重启并安装更新");
  EXPECT_TRUE(f.controller.canRun());
  f.push("upToDate");
  EXPECT_EQ(f.controller.title(), "检查更新");
  EXPECT_FALSE(f.controller.isReady());
}

TEST(UpdateController, ManualCheckNotifiesTheOutcomeButAutomaticOnlyNotifiesWhenReady) {
  UpdateFixture f;
  f.attach();
  // 自动检查：不通知开始、最新与失败
  f.push("checking");
  f.push("upToDate");
  f.push("error", "", "网络不通");
  EXPECT_TRUE(f.shell.notices.empty());
  f.push("ready", "2.0.0");
  ASSERT_EQ(f.shell.notices.size(), 1U);
  EXPECT_EQ(f.shell.notices[0].first, "新版本 2.0.0 已下载好，在“帮助”菜单里选择“重启并安装更新”即可使用。");

  UpdateFixture g;
  g.attach();
  g.controller.run(g.shell);
  ASSERT_EQ(g.shell.notices.size(), 1U);
  EXPECT_EQ(g.shell.notices[0].first, "正在检查更新…");
  g.push("downloading");
  EXPECT_EQ(g.shell.notices.back().first, "发现新版本，正在下载。");
  g.push("upToDate");
  EXPECT_EQ(g.shell.notices.back().first, "已是最新版本。");
  // 结果通知之后不再是手动
  const std::size_t count = g.shell.notices.size();
  g.push("upToDate");
  EXPECT_EQ(g.shell.notices.size(), count);
}

TEST(UpdateController, ManualCheckFailureIsNotifiedWithTheReason) {
  UpdateFixture f;
  f.attach();
  f.controller.run(f.shell);
  f.push("error", "", "网络不通");
  EXPECT_EQ(f.shell.notices.back().first, "检查更新失败：网络不通");
  EXPECT_EQ(f.shell.notices.back().second, NotificationLevel::Error);
  f.core.replies["system.checkUpdate"] = [](const json&) { return CoreReply::failure("core.internal", ""); };
  f.push("idle");
  f.controller.run(f.shell);
  EXPECT_EQ(f.shell.notices.back().first, "检查更新失败");
}

TEST(UpdateController, UnsupportedIsExplainedAtOnce) {
  UpdateFixture f;
  f.reply("system.checkUpdate", statusJson("unsupported"));
  f.attach();
  f.controller.run(f.shell);
  ASSERT_EQ(f.shell.notices.size(), 1U);
  EXPECT_EQ(f.shell.notices[0].first, "当前运行方式不支持检查更新。");
}

TEST(UpdateController, RunInstallsWhenReadyAndReportsFailure) {
  UpdateFixture f;
  f.attach();
  f.push("ready", "2.0.0");
  f.shell.notices.clear();
  f.controller.run(f.shell);
  EXPECT_NE(std::find_if(f.core.calls.begin(), f.core.calls.end(), [](const auto& call) { return call.name == "system.installUpdate"; }), f.core.calls.end());
  EXPECT_TRUE(f.shell.notices.empty());
  f.fail("system.installUpdate", "没有可安装的新版本，或新版本无法启动");
  f.controller.run(f.shell);
  ASSERT_EQ(f.shell.notices.size(), 1U);
  EXPECT_EQ(f.shell.notices[0].first, "没有可安装的新版本，或新版本无法启动");
}

TEST(UpdateController, DoesNothingWhileCheckingAndAfterDetach) {
  UpdateFixture f;
  f.attach();
  f.push("checking");
  const std::size_t calls = f.core.calls.size();
  f.controller.run(f.shell);
  EXPECT_EQ(f.core.calls.size(), calls);
  f.detach();
  EXPECT_TRUE(f.updater.handlers.empty());
  f.controller.run(f.shell);
  EXPECT_EQ(f.core.calls.size(), calls);
}

json versionResult() { return {{"appVersion", "0.3.0"}, {"coreVersion", "0.3.1"}}; }

struct AboutFixture {
  test::FakeCoreGateway core;
  test::FakeBroker broker{core};
  test::FakeShellApi shell;
  test::FakeExternalLinks links;
  logic::AboutController controller{broker, core, links, {{"Zlib", "1.0", "A、B", "MIT", "https://z", "压缩"}}, "2026-11-21 10:00:00"};
  std::function<void()> detach;
  int changes{0};
  std::function<void()> unsubscribe;

  AboutFixture() {
    core.replies["system.version"] = [](const json&) { return CoreReply::success(versionResult()); };
    unsubscribe = controller.subscribe([this] { ++changes; });
  }
  void attach() { detach = controller.attach(shell); }
};

TEST(AboutController, ShowsTheFixedTextsTheBuildTimeAndTheNotices) {
  AboutFixture f;
  const auto view = f.controller.view();
  EXPECT_EQ(view.appName, "Ember");
  EXPECT_EQ(view.author, "sungbin");
  EXPECT_EQ(view.email, "sungbin@3578.cn");
  EXPECT_EQ(view.qq, "46427991");
  EXPECT_EQ(view.homepageUrl, "https://fcdev.cn");
  EXPECT_EQ(view.projectUrl, "https://github.com/sungbindev/famiclone/tree/master/ember");
  EXPECT_EQ(view.license, "GNU 通用公共许可证第 3 版（GPL-3.0）");
  EXPECT_EQ(view.buildTime, "2026-11-21 10:00:00");
  ASSERT_EQ(view.notices.size(), 1U);
  EXPECT_EQ(view.notices[0].name, "Zlib");
  EXPECT_EQ(view.versionLoad, contracts::AboutVersionLoad::Loading);
}

TEST(AboutController, ReadsTheVersionsOnAttachAndAfterCoreRestart) {
  AboutFixture f;
  f.attach();
  auto view = f.controller.view();
  EXPECT_EQ(view.versionLoad, contracts::AboutVersionLoad::Ready);
  EXPECT_EQ(view.appVersion, "0.3.0");
  EXPECT_EQ(view.coreVersion, "0.3.1");
  f.core.setState(contracts::CoreState::Starting);
  EXPECT_EQ(f.controller.view().versionLoad, contracts::AboutVersionLoad::Loading);
  f.core.setState(contracts::CoreState::Running);
  EXPECT_EQ(f.controller.view().versionLoad, contracts::AboutVersionLoad::Ready);
  f.detach();
  EXPECT_EQ(f.core.subscriberCount(), 0U);
}

TEST(AboutController, UnreadableVersionShowsFailedWithoutANotice) {
  AboutFixture f;
  f.core.replies["system.version"] = [](const json&) { return CoreReply::failure("core.unavailable", "x"); };
  f.attach();
  EXPECT_EQ(f.controller.view().versionLoad, contracts::AboutVersionLoad::Failed);
  EXPECT_TRUE(f.shell.notices.empty());
  f.core.replies["system.version"] = [](const json&) { return CoreReply::success({{"appVersion", 1}}); };
  f.core.setState(contracts::CoreState::Running);
  EXPECT_EQ(f.controller.view().versionLoad, contracts::AboutVersionLoad::Failed);
}

TEST(AboutController, OpenHomepageUsesTheBrowserAndReportsFailure) {
  AboutFixture f;
  f.attach();
  f.controller.openHomepage();
  ASSERT_EQ(f.links.opened.size(), 1U);
  EXPECT_EQ(f.links.opened[0], "https://fcdev.cn");
  EXPECT_TRUE(f.shell.notices.empty());
  f.links.error = "没有浏览器";
  f.controller.openHomepage();
  ASSERT_EQ(f.shell.notices.size(), 1U);
  EXPECT_EQ(f.shell.notices[0].first, "无法打开主页地址：没有浏览器");
}

TEST(AboutController, OpenProjectUsesTheBrowserAndReportsFailure) {
  AboutFixture f;
  f.attach();
  f.controller.openProject();
  ASSERT_EQ(f.links.opened.size(), 1U);
  EXPECT_EQ(f.links.opened[0], "https://github.com/sungbindev/famiclone/tree/master/ember");
  EXPECT_TRUE(f.shell.notices.empty());
  f.links.error = "没有浏览器";
  f.controller.openProject();
  ASSERT_EQ(f.shell.notices.size(), 1U);
  EXPECT_EQ(f.shell.notices[0].first, "无法打开项目地址：没有浏览器");
}

struct ModuleFixture {
  test::FakeCoreGateway core;
  test::FakeBroker broker{core};
  test::FakeUpdater updater;
  test::FakeShellApi shell;
  test::FakeExternalLinks links;
  logic::UpdateController update{broker, updater};
  logic::AboutController about{broker, core, links, {}, "t"};
  contracts::ShellModule module{logic::createSystemModule(broker, update, about)};

  [[nodiscard]] const contracts::ShellCommandDefinition* command(const std::string& id) const {
    for (const auto& item : module.commands) {
      if (item.id == id) return &item;
    }
    return nullptr;
  }
};

TEST(SystemModule, RegistersTheHelpMenuInOrderWithSeparators) {
  ModuleFixture f;
  EXPECT_EQ(f.module.name, "system");
  std::vector<std::string> commands;
  std::vector<bool> separators;
  for (const auto& item : f.module.menuItems) {
    EXPECT_EQ(item.menu, "help");
    commands.push_back(item.command);
    separators.push_back(item.separatorBefore);
  }
  EXPECT_EQ(commands, (std::vector<std::string>{"system.openHelp", "system.openChangelog", "system.openLogDirectory", "system.checkUpdate", "system.openAbout"}));
  EXPECT_EQ(separators, (std::vector<bool>{false, false, false, true, true}));
  EXPECT_EQ(f.command("system.openHelp")->title(), "帮助文档");
  EXPECT_EQ(f.command("system.openChangelog")->title(), "更新记录");
  EXPECT_EQ(f.command("system.openLogDirectory")->title(), "打开日志目录");
  EXPECT_EQ(f.command("system.openAbout")->title(), "关于 Ember");
}

TEST(SystemModule, AboutDialogHasTheAgreedSize) {
  ModuleFixture f;
  ASSERT_EQ(f.module.dialogs.size(), 1U);
  const auto& dialog = f.module.dialogs[0];
  EXPECT_EQ(dialog.id, "system.about");
  EXPECT_EQ(dialog.title, "关于 Ember");
  EXPECT_EQ(dialog.width, 0);
  EXPECT_EQ(dialog.height, 560);
  EXPECT_TRUE(dialog.fitMinWidth);
  EXPECT_EQ(dialog.minHeight, 320);
  f.command("system.openAbout")->run(f.shell);
  EXPECT_EQ(f.shell.dialogs, (std::vector<std::string>{"system.about"}));
}

TEST(SystemModule, HelpLogAndChangelogCommandsSendTheirRequestsAndNotifyFailures) {
  ModuleFixture f;
  f.command("system.openHelp")->run(f.shell);
  f.command("system.openChangelog")->run(f.shell);
  f.command("system.openLogDirectory")->run(f.shell);
  ASSERT_EQ(f.core.calls.size(), 3U);
  EXPECT_EQ(f.core.calls[0].name, "system.openHelp");
  EXPECT_EQ(f.core.calls[0].params, json::object());
  EXPECT_EQ(f.core.calls[1].params, (json{{"page", "changelog"}}));
  EXPECT_EQ(f.core.calls[2].name, "system.openLogDirectory");
  EXPECT_TRUE(f.shell.notices.empty());
  f.core.replies["system.openHelp"] = [](const json&) { return CoreReply::failure("system.helpUnavailable", "无法打开帮助文档：没有浏览器"); };
  f.core.replies["system.openLogDirectory"] = [](const json&) { return CoreReply::failure("system.logDirectoryUnavailable", ""); };
  f.command("system.openHelp")->run(f.shell);
  f.command("system.openLogDirectory")->run(f.shell);
  ASSERT_EQ(f.shell.notices.size(), 2U);
  EXPECT_EQ(f.shell.notices[0].first, "无法打开帮助文档：没有浏览器");
  EXPECT_EQ(f.shell.notices[1].first, "无法打开日志目录");
  EXPECT_EQ(f.shell.notices[1].second, NotificationLevel::Error);
}

TEST(SystemModule, CheckUpdateCommandFollowsTheControllerAndTheTitleBarButtonOnlyShowsWhenReady) {
  ModuleFixture f;
  const auto detach = f.module.attach(f.shell);
  const auto* check = f.command("system.checkUpdate");
  ASSERT_NE(check, nullptr);
  EXPECT_EQ(check->title(), "检查更新");
  EXPECT_TRUE(check->enabled(contracts::ShellViewState{}));
  ASSERT_EQ(f.module.titleBarItems.size(), 1U);
  const auto& item = f.module.titleBarItems[0];
  EXPECT_EQ(item.id, "system.update");
  EXPECT_FALSE(item.view().has_value());
  f.updater.push({"downloading", std::nullopt, std::nullopt});
  EXPECT_FALSE(check->enabled(contracts::ShellViewState{}));
  f.updater.push({"ready", std::string("2.0.0"), std::nullopt});
  EXPECT_EQ(check->title(), "重启并安装更新（2.0.0）");
  ASSERT_TRUE(item.view().has_value());
  EXPECT_EQ(item.view()->text, "更新");
  EXPECT_EQ(item.view()->hint, "重启并安装更新（2.0.0）");
  item.click(f.shell);
  EXPECT_NE(std::find_if(f.core.calls.begin(), f.core.calls.end(), [](const auto& call) { return call.name == "system.installUpdate"; }), f.core.calls.end());
  detach();
  EXPECT_TRUE(f.updater.handlers.empty());
}

// ---- 自动检查更新 ----

/** 假的定时：记下每个任务与它是否被取消，由测试触发。 */
struct AutoCheckFixture {
  struct Timer {
    unsigned delay{0};
    std::function<void()> task;
    bool cancelled{false};
  };

  test::FakeCoreGateway core;
  test::FakeBroker broker{core};
  test::FakeUpdater updater;
  std::vector<std::shared_ptr<Timer>> timers;
  logic::AutoUpdateCheck check{broker, core, updater,
                               [this](unsigned delay, std::function<void()> task) {
                                 auto timer = std::make_shared<Timer>(Timer{delay, std::move(task), false});
                                 timers.push_back(timer);
                                 return std::function<void()>([timer] { timer->cancelled = true; });
                               },
                               5000};

  AutoCheckFixture() { setting(true); }

  void setting(bool value) {
    core.replies["preferences.list"] = [value](const json&) { return CoreReply::success(json{{"settings", json::array({json{{"id", "other"}, {"value", 1}}, json{{"id", "update.autoCheck"}, {"value", value}}})}}); };
  }
  void running() { core.current.state = contracts::CoreState::Running; }
  [[nodiscard]] int pendingTimers() const {
    return static_cast<int>(std::count_if(timers.begin(), timers.end(), [](const auto& timer) { return !timer->cancelled && timer->task; }));
  }
  void fire(std::size_t index) {
    auto task = timers.at(index)->task;
    timers.at(index)->task = nullptr;
    task();
  }
};

TEST(AutoUpdateCheck, ChecksOnceAfterTheDelayWhenTheSettingIsOnAndTheCoreIsRunning) {
  AutoCheckFixture f;
  f.running();
  f.check.start();
  ASSERT_EQ(f.timers.size(), 1U);
  EXPECT_EQ(f.timers[0]->delay, 5000U);
  EXPECT_EQ(f.updater.checks, 0);
  f.fire(0);
  EXPECT_EQ(f.updater.checks, 1);
  // 只检查一次：之后设置再变化、核心再启动都不再安排
  f.core.emitEvent("preferences.changed", json{{"id", "update.autoCheck"}, {"value", true}});
  f.core.setState(contracts::CoreState::Running);
  EXPECT_EQ(f.timers.size(), 1U);
  EXPECT_EQ(f.updater.checks, 1);
}

TEST(AutoUpdateCheck, WaitsForTheCoreAndDoesNothingWhenTheSettingIsOff) {
  AutoCheckFixture f;
  f.check.start();
  EXPECT_TRUE(f.core.calls.empty());
  f.setting(false);
  f.core.setState(contracts::CoreState::Running);
  EXPECT_EQ(f.core.calls.size(), 1U);
  EXPECT_TRUE(f.timers.empty());
  EXPECT_EQ(f.updater.checks, 0);
}

TEST(AutoUpdateCheck, TurningTheSettingOnSchedulesAndTurningItOffCancels) {
  AutoCheckFixture f;
  f.setting(false);
  f.running();
  f.check.start();
  f.core.emitEvent("preferences.changed", json{{"id", "update.autoCheck"}, {"value", true}});
  ASSERT_EQ(f.timers.size(), 1U);
  // 已经安排的不重复安排
  f.core.emitEvent("preferences.changed", json{{"id", "update.autoCheck"}, {"value", true}});
  EXPECT_EQ(f.timers.size(), 1U);
  f.core.emitEvent("preferences.changed", json{{"id", "update.autoCheck"}, {"value", false}});
  EXPECT_TRUE(f.timers[0]->cancelled);
  f.core.emitEvent("preferences.changed", json{{"id", "update.autoCheck"}, {"value", true}});
  EXPECT_EQ(f.timers.size(), 2U);
  // 其他设置与不合格的事件都忽略
  f.core.emitEvent("preferences.changed", json{{"id", "other"}, {"value", false}});
  f.core.emitEvent("preferences.changed", json("text"));
  f.core.emitEvent("other.event", json{{"id", "update.autoCheck"}, {"value", false}});
  EXPECT_FALSE(f.timers[1]->cancelled);
}

TEST(AutoUpdateCheck, UnsupportedUpdaterAndUnreadableSettingsDoNothing) {
  AutoCheckFixture unsupported;
  unsupported.updater.current.state = "unsupported";
  unsupported.running();
  unsupported.check.start();
  EXPECT_EQ(unsupported.core.subscriberCount(), 0U);
  EXPECT_TRUE(unsupported.timers.empty());

  AutoCheckFixture broken;
  broken.core.replies["preferences.list"] = [](const json&) { return CoreReply::failure("preferences.readFailed", "读取失败"); };
  broken.running();
  broken.check.start();
  EXPECT_TRUE(broken.timers.empty());
  broken.core.replies["preferences.list"] = [](const json&) { return CoreReply::success(json{{"settings", "bad"}}); };
  broken.core.setState(contracts::CoreState::Running);
  EXPECT_TRUE(broken.timers.empty());
}

TEST(AutoUpdateCheck, StopCancelsTheTimerAndUnsubscribes) {
  AutoCheckFixture f;
  f.running();
  f.check.start();
  EXPECT_EQ(f.core.subscriberCount(), 2U);
  f.check.stop();
  EXPECT_TRUE(f.timers[0]->cancelled);
  EXPECT_EQ(f.core.subscriberCount(), 0U);
  f.core.emitEvent("preferences.changed", json{{"id", "update.autoCheck"}, {"value", true}});
  EXPECT_EQ(f.timers.size(), 1U);
  EXPECT_EQ(f.updater.checks, 0);
}

// ---- 升级后的更新记录 ----

struct ReleaseNotesFixture {
  test::FakeUpdater updater;
  test::FakeShellState state;
  test::FakeHelp help;
  test::FakeErrorLogger logger;

  void run(const std::string& version) { logic::showReleaseNotesOnce(updater, version, state, help, logger); }
};

TEST(ReleaseNotes, FirstRunOnlyRecordsTheVersionWithoutOpeningAnything) {
  ReleaseNotesFixture f;
  f.run("0.3.0");
  EXPECT_EQ(f.state.state.releaseNotesVersion, std::optional<std::string>("0.3.0"));
  EXPECT_TRUE(f.help.pages.empty());
}

TEST(ReleaseNotes, OpensTheChangelogOnceAfterAnUpgrade) {
  ReleaseNotesFixture f;
  f.state.state.releaseNotesVersion = "0.2.2";
  f.run("0.3.0");
  ASSERT_EQ(f.help.pages.size(), 1U);
  EXPECT_EQ(f.help.pages[0], std::optional<std::string>("changelog"));
  EXPECT_EQ(f.state.state.releaseNotesVersion, std::optional<std::string>("0.3.0"));
  f.run("0.3.0");
  EXPECT_EQ(f.help.pages.size(), 1U);
}

TEST(ReleaseNotes, FailureToOpenIsLoggedAndTheVersionIsStillRecorded) {
  ReleaseNotesFixture f;
  f.state.state.releaseNotesVersion = "0.2.2";
  f.help.error = "没有浏览器";
  f.run("0.3.0");
  ASSERT_EQ(f.logger.entries.size(), 1U);
  EXPECT_NE(f.logger.entries[0].message.find("没有浏览器"), std::string::npos);
  EXPECT_EQ(f.state.state.releaseNotesVersion, std::optional<std::string>("0.3.0"));
}

TEST(ReleaseNotes, UnsupportedUpdaterNeverRecordsOrOpens) {
  ReleaseNotesFixture f;
  f.updater.current.state = "unsupported";
  f.state.state.releaseNotesVersion = "0.2.2";
  f.run("0.3.0");
  EXPECT_TRUE(f.help.pages.empty());
  EXPECT_EQ(f.state.state.releaseNotesVersion, std::optional<std::string>("0.2.2"));
}

}  // namespace
}  // namespace ember::win32
