/*
 * ------------------------------------------------------------------------
 * 名称：runtime-logic-test.cpp
 * 说明：运行模块逻辑的测试：纯规则（状态、可用性、文字、机型选项）、运行模块控制器（初始读取、选择机型、开机与断电、控制、打开卡带与最近打开、音量与静音、Family BASIC 键盘、画面设置、核心重启、断开）、运行模块的登记项，以及输入控制器的消息。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include "logic/input/input-controller.hpp"
#include "logic/runtime/runtime-controller.hpp"
#include "logic/runtime/runtime-module.hpp"
#include "logic/runtime/runtime-rules.hpp"
#include "logic/shell/shell-controller.hpp"
#include "logic/shell/shell-module.hpp"
#include "support/fakes.hpp"

namespace ember::win32 {
namespace {

using contracts::CoreReply;
using contracts::MachineState;
using nlohmann::json;

const json kNoStatus = json::object();

json statusJson(const std::string& power, const std::string& mode, const std::string& machine = "", bool mouse = false, const std::string& path = "") {
  json status = {{"power", power}, {"mode", mode}, {"frame", 0}, {"cycles", 0}, {"speed", 1}, {"mouse", mouse}};
  if (!machine.empty()) status["machine"] = machine;
  if (!path.empty()) status["cartridge"] = {{"path", path}, {"pluginName", "NROM"}};
  return status;
}

json machinesJson() {
  return {{"machines",
           json::array({{{"id", "fc"}, {"name", "经典 FC"}, {"needsBios", false}, {"supported", true}},
                        {{"id", "sb97"}, {"name", "小霸王 97"}, {"needsBios", true}, {"supported", true}},
                        {{"id", "sb2000"}, {"name", "小霸王 2000"}, {"needsBios", true}, {"supported", true}, {"bios", {{"name", "SB2K_BIOS.bin"}}}},
                        {{"id", "bbk10"}, {"name", "步步高软驱一号"}, {"needsBios", true}, {"supported", false}}})}};
}

TEST(RuntimeRules, StateTextsFollowPowerAndMode) {
  using logic::RuntimeStatus;
  EXPECT_EQ(logic::machineState(std::nullopt), MachineState::Unknown);
  EXPECT_FALSE(logic::stateText(std::nullopt).has_value());
  EXPECT_EQ(logic::machineState(logic::parseRuntimeStatus(statusJson("off", "paused"))), MachineState::None);
  EXPECT_EQ(logic::stateText(logic::parseRuntimeStatus(statusJson("off", "paused"))), "未开机");
  EXPECT_EQ(logic::stateText(logic::parseRuntimeStatus(statusJson("off", "paused", "fc"))), "已断电");
  EXPECT_EQ(logic::stateText(logic::parseRuntimeStatus(statusJson("on", "paused", "fc"))), "已暂停");
  EXPECT_EQ(logic::stateText(logic::parseRuntimeStatus(statusJson("on", "running", "fc"))), "运行中");
  EXPECT_EQ(logic::powerToggleText(logic::parseRuntimeStatus(statusJson("on", "running", "fc"))), "断电");
  EXPECT_EQ(logic::powerToggleText(logic::parseRuntimeStatus(statusJson("off", "paused", "fc"))), "开机");
  EXPECT_EQ(logic::pauseToggleText(logic::parseRuntimeStatus(statusJson("on", "running", "fc"))), "暂停");
  EXPECT_EQ(logic::pauseToggleText(logic::parseRuntimeStatus(statusJson("on", "paused", "fc"))), "继续");
}

TEST(RuntimeRules, StatusItemsOnlyHaveTextWhenThereIsInformation) {
  const auto none = logic::parseRuntimeStatus(statusJson("off", "paused"));
  EXPECT_EQ(logic::hostText(none, "sb2000"), "小霸王 2000");
  EXPECT_FALSE(logic::cartridgeText(none).has_value());
  EXPECT_FALSE(logic::pluginText(none).has_value());
  EXPECT_FALSE(logic::hostText(std::nullopt, "fc").has_value());
  const auto loaded = logic::parseRuntimeStatus(statusJson("on", "running", "fc", false, "D:\\roms\\Super Game.nes"));
  EXPECT_EQ(logic::hostText(loaded, "sb2000"), "经典 FC");
  EXPECT_EQ(logic::cartridgeText(loaded), "Super Game.nes");
  EXPECT_EQ(logic::pluginText(loaded), "NROM");
  EXPECT_EQ(logic::hostText(logic::parseRuntimeStatus(statusJson("on", "running", "unknown1")), "fc"), "unknown1");
}

TEST(RuntimeRules, AvailabilityFollowsStateAndSelectedMachine) {
  const auto machines = logic::parseMachines(machinesJson());
  auto controls = logic::controlAvailability(std::nullopt, "fc", machines);
  EXPECT_FALSE(controls.powerToggle || controls.pauseToggle || controls.softReset || controls.hardReset || controls.eject);
  // 没有开机：选了有 BIOS 的整机机型才能开机，其余置灰
  controls = logic::controlAvailability(logic::parseRuntimeStatus(statusJson("off", "paused")), "sb2000", machines);
  EXPECT_TRUE(controls.powerToggle);
  EXPECT_FALSE(controls.pauseToggle || controls.softReset || controls.hardReset);
  EXPECT_FALSE(logic::controlAvailability(logic::parseRuntimeStatus(statusJson("off", "paused")), "sb97", machines).powerToggle);
  EXPECT_FALSE(logic::controlAvailability(logic::parseRuntimeStatus(statusJson("off", "paused")), "bbk10", machines).powerToggle);
  EXPECT_FALSE(logic::controlAvailability(logic::parseRuntimeStatus(statusJson("off", "paused")), "fc", machines).powerToggle);
  // 断电：卡带还在时能开机，不能暂停与复位，能关闭卡带
  controls = logic::controlAvailability(logic::parseRuntimeStatus(statusJson("off", "paused", "fc", false, "a.nes")), "fc", machines);
  EXPECT_TRUE(controls.powerToggle);
  EXPECT_TRUE(controls.eject);
  EXPECT_FALSE(controls.pauseToggle || controls.softReset || controls.hardReset);
  controls = logic::controlAvailability(logic::parseRuntimeStatus(statusJson("on", "running", "fc", false, "a.nes")), "fc", machines);
  EXPECT_TRUE(controls.powerToggle && controls.pauseToggle && controls.softReset && controls.hardReset);
  // 已选经典 FC、装入的是学习机：要先打开卡带
  EXPECT_FALSE(logic::controlAvailability(logic::parseRuntimeStatus(statusJson("off", "paused", "sb2000")), "fc", machines).powerToggle);
}

TEST(RuntimeRules, MachineChoicesExplainWhyAMachineCannotBePicked) {
  const auto choices = logic::machineChoices(logic::parseMachines(machinesJson()));
  ASSERT_EQ(choices.size(), 4U);
  EXPECT_EQ(choices[0].id, "sb97");
  EXPECT_EQ(choices[0].hint, "未添加 BIOS，请到设置→机型与 BIOS 添加");
  EXPECT_EQ(choices[1].id, "sb2000");
  EXPECT_TRUE(choices[1].hint.empty());
  EXPECT_EQ(choices[2].id, "bbk10");
  EXPECT_EQ(choices[2].hint, "暂未支持");
  EXPECT_EQ(choices[3].id, "bbk20");
  EXPECT_EQ(choices[3].hint, "暂未支持");
  for (const auto& choice : logic::machineChoices({})) EXPECT_EQ(choice.hint, "暂未支持");
}

TEST(RuntimeRules, MouseIsCapturedOnlyWhenEnabledAndTheMachineHasOne) {
  EXPECT_TRUE(logic::capturesMouse(true, logic::parseRuntimeStatus(statusJson("on", "running", "sb2000", true))));
  EXPECT_FALSE(logic::capturesMouse(false, logic::parseRuntimeStatus(statusJson("on", "running", "sb2000", true))));
  EXPECT_FALSE(logic::capturesMouse(true, logic::parseRuntimeStatus(statusJson("on", "running", "fc", false))));
  EXPECT_FALSE(logic::capturesMouse(true, std::nullopt));
}

TEST(RuntimeRules, KeyboardIsExclusiveForHandheldMachinesAndWithTheFamilyKeyboard) {
  const auto status = [](const char* machine) { return logic::parseRuntimeStatus(statusJson("on", "running", machine, false)); };
  // 学习机整机机型独占，经典 FC 的普通游戏不独占
  EXPECT_TRUE(logic::exclusiveKeyboard(false, status("sb2000")));
  EXPECT_TRUE(logic::exclusiveKeyboard(false, status("sb97")));
  EXPECT_TRUE(logic::exclusiveKeyboard(false, status("bbk10")));
  EXPECT_FALSE(logic::exclusiveKeyboard(false, status("fc")));
  // 接入 Family BASIC 键盘后独占
  EXPECT_TRUE(logic::exclusiveKeyboard(true, status("fc")));
  // 还没开机或还没读到状态时只看 Family BASIC 键盘
  EXPECT_FALSE(logic::exclusiveKeyboard(false, std::nullopt));
  EXPECT_TRUE(logic::exclusiveKeyboard(true, std::nullopt));
}

/** 带假核心的运行模块控制器。 */
struct RuntimeFixture {
  test::FakeCoreGateway core;
  test::FakeBroker broker{core};
  test::FakeShellState state;
  test::FakeShellApi shell;
  logic::RuntimeController controller{broker, core, state};
  std::function<void()> detach;
  std::function<void()> unsubscribe;
  int focusCalls{0};
  int changes{0};

  RuntimeFixture() {
    reply("runtime.status", statusJson("off", "paused"));
    reply("machines.list", machinesJson());
    reply("cartridges.recent", {{"items", json::array({{{"id", 1}, {"name", "a.nes"}}, {{"id", 2}, {"name", "b.nes"}}})}});
    reply("output.setVolume", {{"volume", 80}, {"muted", false}});
    reply("preferences.list", {{"settings", json::array()}});
    controller.setScreenFocus([this] { ++focusCalls; });
    unsubscribe = controller.subscribe([this] { ++changes; });
  }

  void attach() { detach = controller.attach(shell); }

  void reply(const std::string& name, const json& result) {
    core.replies[name] = [result](const json&) { return CoreReply::success(result); };
  }
  void fail(const std::string& name, const std::string& message) {
    core.replies[name] = [message](const json&) { return CoreReply::failure("x.failed", message); };
  }
  [[nodiscard]] int count(const std::string& name) const {
    int total = 0;
    for (const auto& call : core.calls) total += call.name == name ? 1 : 0;
    return total;
  }
  [[nodiscard]] const test::FakeCoreGateway::Call* last(const std::string& name) const {
    for (auto it = core.calls.rbegin(); it != core.calls.rend(); ++it) {
      if (it->name == name) return &*it;
    }
    return nullptr;
  }
};

TEST(RuntimeController, ReadsStatusRecentMachinesAndVolumeWhenAttached) {
  RuntimeFixture f;
  EXPECT_FALSE(f.controller.status().has_value());
  EXPECT_EQ(f.controller.selectedMachine(), "fc");
  EXPECT_EQ(f.controller.view().state, MachineState::Unknown);
  EXPECT_FALSE(f.controller.availability().powerToggle);
  f.attach();
  ASSERT_TRUE(f.controller.status().has_value());
  EXPECT_EQ(f.controller.view().state, MachineState::None);
  EXPECT_EQ(f.controller.recent().size(), 2U);
  EXPECT_EQ(f.controller.machines().size(), 4U);
  EXPECT_EQ(f.controller.view().volume, 80);
  EXPECT_GT(f.shell.refreshes, 0);
  EXPECT_GT(f.changes, 0);
}

TEST(RuntimeController, StatusStaysUnknownWhenTheCoreCannotAnswerYet) {
  RuntimeFixture f;
  f.fail("runtime.status", "核心还没有就绪");
  f.attach();
  EXPECT_FALSE(f.controller.status().has_value());
  EXPECT_TRUE(f.shell.notices.empty());
}

TEST(RuntimeController, MachinesAreReadAgainAfterTheBiosChanges) {
  RuntimeFixture f;
  f.attach();
  EXPECT_FALSE(logic::machineChoices(f.controller.machines())[0].hint.empty());
  auto machines = machinesJson();
  machines["machines"][1]["bios"] = {{"name", "SB97.bin"}};
  f.reply("machines.list", machines);
  f.core.emitEvent("machines.changed", json::object());
  EXPECT_TRUE(logic::machineChoices(f.controller.machines())[0].hint.empty());
}

TEST(RuntimeController, SelectingAMachineOnlySavesItAndDoesNotBoot) {
  RuntimeFixture f;
  f.attach();
  f.controller.selectMachine("sb2000");
  EXPECT_EQ(f.controller.selectedMachine(), "sb2000");
  EXPECT_EQ(f.state.state.layout.selectedMachine, std::optional<std::string>("sb2000"));
  EXPECT_EQ(f.count("machines.boot"), 0);
  f.controller.selectMachine("sb2000");
  EXPECT_EQ(f.state.layoutUpdates, 1);
}

TEST(RuntimeController, SelectAndBootRemembersTheMachineAndBootsIt) {
  RuntimeFixture f;
  f.reply("runtime.status", statusJson("on", "running", "sb97"));
  f.attach();
  f.controller.selectAndBootMachine("sb2000");
  ASSERT_EQ(f.count("machines.boot"), 1);
  EXPECT_EQ(f.last("machines.boot")->params, json({{"machine", "sb2000"}}));
  EXPECT_EQ(f.controller.selectedMachine(), "sb2000");
  EXPECT_EQ(f.state.layoutUpdates, 1);
  EXPECT_GE(f.focusCalls, 1);
}

TEST(RuntimeController, SelectingTheRunningCheckedMachineAgainDoesNothing) {
  for (const char* mode : {"running", "paused"}) {
    RuntimeFixture f;
    f.state.state.layout.selectedMachine = "sb2000";
    f.reply("runtime.status", statusJson("on", mode, "sb2000"));
    f.attach();
    f.controller.selectAndBootMachine("sb2000");
    EXPECT_EQ(f.count("machines.boot"), 0) << mode;
    EXPECT_EQ(f.state.layoutUpdates, 0) << mode;
    // 点别的机型照常立即开机，并记住新点的
    f.controller.selectAndBootMachine("sb97");
    EXPECT_EQ(f.count("machines.boot"), 1) << mode;
    EXPECT_EQ(f.controller.selectedMachine(), "sb97") << mode;
  }
}

TEST(RuntimeController, SelectingTheCheckedMachineAfterPowerOffBootsIt) {
  RuntimeFixture f;
  f.state.state.layout.selectedMachine = "sb2000";
  f.reply("runtime.status", statusJson("off", "paused", "sb2000"));
  f.attach();
  f.controller.selectAndBootMachine("sb2000");
  EXPECT_EQ(f.count("machines.boot"), 1);
}

TEST(RuntimeController, BootFailureIsNotifiedAndTheChoiceIsStillRemembered) {
  RuntimeFixture f;
  f.fail("machines.boot", "BIOS 不合格");
  f.attach();
  f.controller.selectAndBootMachine("sb2000");
  ASSERT_EQ(f.shell.notices.size(), 1U);
  EXPECT_EQ(f.shell.notices[0].first, "开机失败：BIOS 不合格");
  EXPECT_EQ(f.shell.notices[0].second, contracts::NotificationLevel::Error);
  EXPECT_EQ(f.controller.selectedMachine(), "sb2000");
}

TEST(RuntimeController, ReusesTheSavedMachineOfTheLastRun) {
  RuntimeFixture f;
  f.state.state.layout.selectedMachine = "sb2000";
  f.attach();
  EXPECT_EQ(f.controller.selectedMachine(), "sb2000");
  EXPECT_EQ(f.state.layoutUpdates, 0);
}

TEST(RuntimeController, PowerWithClassicSelectedNeedsALoadedCartridge) {
  RuntimeFixture f;
  f.attach();
  f.controller.togglePower();
  EXPECT_EQ(f.count("runtime.powerOn"), 0);
  EXPECT_EQ(f.count("machines.boot"), 0);
  f.reply("runtime.status", statusJson("off", "paused", "fc", false, "a.nes"));
  f.core.emitEvent("runtime.stateChanged", json::object());
  f.controller.togglePower();
  ASSERT_EQ(f.count("runtime.powerOn"), 1);
  EXPECT_EQ(f.last("runtime.powerOn")->params, json({{"resume", true}}));
}

TEST(RuntimeController, PowerWhenOnTurnsItOffAndAMachineBootsWithItsBios) {
  RuntimeFixture f;
  f.reply("runtime.status", statusJson("on", "running", "fc", false, "a.nes"));
  f.attach();
  f.controller.togglePower();
  EXPECT_EQ(f.count("runtime.powerOff"), 1);
  RuntimeFixture g;
  g.state.state.layout.selectedMachine = "sb2000";
  g.attach();
  g.controller.togglePower();
  EXPECT_EQ(g.count("machines.boot"), 1);
  // 没有 BIOS 的机型不发请求
  RuntimeFixture h;
  h.state.state.layout.selectedMachine = "sb97";
  h.attach();
  h.controller.togglePower();
  EXPECT_EQ(h.count("machines.boot"), 0);
}

TEST(RuntimeController, PowerFailureIsNotifiedAndTheStateIsReadAgain) {
  RuntimeFixture f;
  f.reply("runtime.status", statusJson("on", "running", "fc", false, "a.nes"));
  f.fail("runtime.powerOff", "机器忙");
  f.attach();
  const int before = f.count("runtime.status");
  f.controller.togglePower();
  ASSERT_EQ(f.shell.notices.size(), 1U);
  EXPECT_EQ(f.shell.notices[0].first, "断电失败：机器忙");
  EXPECT_EQ(f.count("runtime.status"), before + 1);
}

TEST(RuntimeController, PauseResumeAndResetsOnlyWorkWhenPoweredOn) {
  RuntimeFixture f;
  f.reply("runtime.status", statusJson("on", "running", "fc", false, "a.nes"));
  f.attach();
  f.controller.togglePause();
  EXPECT_EQ(f.count("runtime.pause"), 1);
  f.reply("runtime.status", statusJson("on", "paused", "fc", false, "a.nes"));
  f.core.emitEvent("runtime.stateChanged", json::object());
  f.controller.togglePause();
  EXPECT_EQ(f.count("runtime.resume"), 1);
  f.controller.softReset();
  f.controller.hardReset();
  EXPECT_EQ(f.last("runtime.reset")->params, json({{"kind", "hard"}}));
  EXPECT_EQ(f.count("runtime.reset"), 2);

  RuntimeFixture off;
  off.reply("runtime.status", statusJson("off", "paused", "fc", false, "a.nes"));
  off.attach();
  off.controller.togglePause();
  off.controller.softReset();
  off.controller.hardReset();
  EXPECT_EQ(off.count("runtime.pause") + off.count("runtime.resume") + off.count("runtime.reset"), 0);
  off.controller.ejectCartridge();
  EXPECT_EQ(off.count("cartridges.eject"), 1);

  RuntimeFixture none;
  none.attach();
  none.controller.ejectCartridge();
  EXPECT_EQ(none.count("cartridges.eject"), 0);
}

TEST(RuntimeController, OpeningACartridgeRefreshesTheRecentListAndFollowsTheLoadedEvent) {
  RuntimeFixture f;
  f.state.state.layout.selectedMachine = "sb2000";
  f.attach();
  const int recentBefore = f.count("cartridges.recent");
  f.controller.openCartridge();
  EXPECT_EQ(f.last("cartridges.open")->params, json::object());
  EXPECT_EQ(f.count("cartridges.recent"), recentBefore + 1);
  // 装入成功的事件：已选机型回到经典 FC 并保存
  f.core.emitEvent("cartridges.loaded", json::object());
  EXPECT_EQ(f.controller.selectedMachine(), "fc");
  EXPECT_EQ(f.state.state.layout.selectedMachine, std::optional<std::string>("fc"));
}

TEST(RuntimeController, DroppedAndRecentCartridgesCarryTheirIds) {
  RuntimeFixture f;
  f.attach();
  f.controller.openDroppedCartridge(7);
  EXPECT_EQ(f.last("cartridges.open")->params, json({{"droppedId", 7}}));
  f.controller.openRecent(2);
  EXPECT_EQ(f.last("cartridges.openRecent")->params, json({{"id", 2}}));
  f.fail("cartridges.openRecent", "文件已不存在");
  f.controller.openRecent(2);
  ASSERT_EQ(f.shell.notices.size(), 1U);
  EXPECT_EQ(f.shell.notices[0].first, "打开卡带失败：文件已不存在");
}

TEST(RuntimeController, VolumeIsClampedAndTheCoreValueWins) {
  RuntimeFixture f;
  f.attach();
  f.reply("output.setVolume", {{"volume", 100}, {"muted", false}});
  f.controller.setVolume(150);
  EXPECT_EQ(f.last("output.setVolume")->params, json({{"volume", 100}}));
  f.reply("output.setVolume", {{"volume", 40}, {"muted", false}});
  f.controller.setVolume(-5);
  EXPECT_EQ(f.last("output.setVolume")->params, json({{"volume", 0}}));
  EXPECT_EQ(f.controller.view().volume, 40);
}

TEST(RuntimeController, MuteKeepsTheVolumeAndFailuresAreNotified) {
  RuntimeFixture f;
  f.attach();
  f.reply("output.setVolume", {{"volume", 80}, {"muted", true}});
  f.controller.toggleMute();
  EXPECT_EQ(f.last("output.setVolume")->params, json({{"muted", true}}));
  EXPECT_TRUE(f.controller.view().muted);
  EXPECT_EQ(f.controller.view().volume, 80);
  f.fail("output.setVolume", "声音设备不可用");
  f.controller.toggleMute();
  EXPECT_EQ(f.shell.notices.back().first, "调整音量失败：声音设备不可用");
}

TEST(RuntimeController, FamilyKeyboardAndDisplaySettingsFollowThePreferences) {
  RuntimeFixture f;
  f.reply("preferences.list", {{"settings", json::array({{{"id", "input.familyKeyboard"}, {"value", true}},
                                                        {{"id", "display.aspect"}, {"value", "4:3"}},
                                                        {{"id", "display.scaling"}, {"value", "integer"}},
                                                        {{"id", "display.showAllLines"}, {"value", false}},
                                                        {{"id", "input.captureMouse"}, {"value", false}}})}});
  f.attach();
  EXPECT_TRUE(f.controller.familyKeyboard());
  const auto view = f.controller.view();
  EXPECT_EQ(view.screen.aspect, shared::ScreenAspect::Ratio4x3);
  EXPECT_EQ(view.screen.scaling, shared::ScreenScaling::Integer);
  EXPECT_FALSE(view.screen.showAllLines);
  EXPECT_FALSE(view.screen.captureMouse);
  // 其他地方改了取值同样跟随，别的设置项不影响
  f.core.emitEvent("preferences.changed", {{"id", "display.aspect"}, {"value", "8:7"}});
  EXPECT_EQ(f.controller.view().screen.aspect, shared::ScreenAspect::Ratio8x7);
  EXPECT_TRUE(f.controller.familyKeyboard());
  f.core.emitEvent("preferences.changed", {{"id", "input.familyKeyboard"}, {"value", false}});
  EXPECT_FALSE(f.controller.familyKeyboard());
  f.reply("preferences.set", {{"id", "input.familyKeyboard"}, {"value", true}});
  f.controller.toggleFamilyKeyboard();
  EXPECT_EQ(f.last("preferences.set")->params, json({{"id", "input.familyKeyboard"}, {"value", true}}));
  EXPECT_TRUE(f.controller.familyKeyboard());
  f.fail("preferences.set", "写不进去");
  f.controller.toggleFamilyKeyboard();
  EXPECT_EQ(f.shell.notices.back().first, "设置 Family BASIC 键盘失败：写不进去");
  EXPECT_TRUE(f.controller.familyKeyboard());
}

TEST(RuntimeController, KeyboardIsExclusiveOnlyForHandheldMachinesOrTheFamilyKeyboard) {
  RuntimeFixture f;
  f.reply("runtime.status", statusJson("on", "running", "fc", false, "a.nes"));
  f.attach();
  EXPECT_FALSE(f.controller.view().exclusiveKeyboard);
  f.core.emitEvent("preferences.changed", {{"id", "input.familyKeyboard"}, {"value", true}});
  EXPECT_TRUE(f.controller.view().exclusiveKeyboard);
  f.core.emitEvent("preferences.changed", {{"id", "input.familyKeyboard"}, {"value", false}});
  EXPECT_FALSE(f.controller.view().exclusiveKeyboard);
  f.reply("runtime.status", statusJson("on", "running", "sb2000", true));
  f.core.emitEvent("runtime.stateChanged", json::object());
  EXPECT_TRUE(f.controller.view().exclusiveKeyboard);
}

TEST(RuntimeController, ResetCountGrowsOnlyWhenTheCoreReportsAReset) {
  RuntimeFixture f;
  f.reply("runtime.status", statusJson("on", "running", "sb2000", true));
  f.attach();
  EXPECT_EQ(f.controller.view().resetCount, 0U);
  f.core.emitEvent("runtime.stateChanged", {{"reason", "reset"}});
  EXPECT_EQ(f.controller.view().resetCount, 1U);
  f.core.emitEvent("runtime.stateChanged", {{"reason", "pause"}});
  EXPECT_EQ(f.controller.view().resetCount, 1U);
  f.core.emitEvent("runtime.stateChanged", {{"reason", "reset"}});
  EXPECT_EQ(f.controller.view().resetCount, 2U);
}

TEST(RuntimeController, MouseCaptureNeedsTheSettingAndAMachineWithAMouse) {
  RuntimeFixture f;
  f.reply("runtime.status", statusJson("on", "running", "sb2000", true));
  f.attach();
  EXPECT_TRUE(f.controller.view().capturesMouse);
  f.core.emitEvent("preferences.changed", {{"id", "input.captureMouse"}, {"value", false}});
  EXPECT_FALSE(f.controller.view().capturesMouse);
}

TEST(RuntimeController, CoreRestartClearsTheStatusAndReadsEverythingWhenReadyAgain) {
  RuntimeFixture f;
  f.reply("runtime.status", statusJson("on", "running", "fc", false, "a.nes"));
  f.attach();
  ASSERT_TRUE(f.controller.status().has_value());
  f.core.setState(contracts::CoreState::Starting);
  EXPECT_FALSE(f.controller.status().has_value());
  EXPECT_FALSE(f.controller.availability().powerToggle);
  const int before = f.count("runtime.status");
  f.core.setState(contracts::CoreState::Running);
  EXPECT_EQ(f.count("runtime.status"), before + 1);
  EXPECT_TRUE(f.controller.status().has_value());
  f.core.setState(contracts::CoreState::Failed);
  EXPECT_FALSE(f.controller.status().has_value());
}

TEST(RuntimeController, DetachingStopsEventsAndForgetsPendingReplies) {
  RuntimeFixture f;
  f.attach();
  EXPECT_GT(f.core.subscriberCount(), 0U);
  f.detach();
  EXPECT_EQ(f.core.subscriberCount(), 0U);
  const int before = f.count("runtime.status");
  f.core.emitEvent("runtime.stateChanged", json::object());
  EXPECT_EQ(f.count("runtime.status"), before);
  // 没有接上时操作不发请求
  f.controller.softReset();
  f.controller.openCartridge();
  EXPECT_EQ(f.count("cartridges.open"), 0);
  // 重新接上时先断开旧的订阅
  f.attach();
  f.attach();
  EXPECT_EQ(f.core.subscriberCount(), 2U);
}

TEST(RuntimeModule, RegistersMenusShortcutsAndStatusItemsOfTheSpecification) {
  RuntimeFixture f;
  f.reply("runtime.status", statusJson("on", "running", "fc", false, "D:\\roms\\game.nes"));
  test::FakeShellWindow window;
  test::FakeShellDialogs dialogs;
  test::FakeNotifier notifier;
  test::FakeDroppedFiles dropped;
  logic::ShellController shell(logic::buildShellRegistry({logic::createShellModule(), logic::createRuntimeModule(f.controller)}), window, dialogs, notifier, f.state, dropped);
  shell.attachModules();
  const auto& registry = shell.registry();
  EXPECT_EQ(registry.screenOwner, "runtime");
  EXPECT_EQ(registry.shortcuts.at("Ctrl+O"), "runtime.openCartridge");
  EXPECT_EQ(registry.shortcuts.at("F9"), "runtime.powerToggle");
  EXPECT_EQ(registry.shortcuts.at("F8"), "runtime.pauseToggle");
  EXPECT_EQ(registry.shortcuts.at("F7"), "runtime.softReset");
  EXPECT_EQ(registry.shortcuts.at("Ctrl+F7"), "runtime.hardReset");
  EXPECT_EQ(registry.shortcuts.at("F11"), "shell.toggleFullScreen");
  const auto menus = shell.menus();
  ASSERT_EQ(menus.size(), 3U);
  ASSERT_EQ(menus[0].items.size(), 4U);
  EXPECT_EQ(menus[0].items[0].label, "打开卡带");
  EXPECT_EQ(menus[0].items[1].label, "最近打开");
  EXPECT_EQ(menus[0].items[2].label, "关闭卡带");
  EXPECT_EQ(menus[0].items[3].label, "退出");
  EXPECT_EQ(menus[0].items[1].items.size(), 2U);
  // “选择机型”是“机器”菜单的第一项，后面是开机或断电（前面有分隔线）
  ASSERT_EQ(menus[1].items.size(), 7U);
  EXPECT_EQ(menus[1].items[0].label, "选择机型");
  EXPECT_FALSE(menus[1].items[0].separatorBefore);
  ASSERT_EQ(menus[1].items[0].items.size(), 4U);
  EXPECT_FALSE(menus[1].items[0].items[1].disabled);
  EXPECT_TRUE(menus[1].items[0].items[0].disabled);
  EXPECT_EQ(menus[1].items[0].items[0].hint, "未添加 BIOS，请到设置→机型与 BIOS 添加");
  EXPECT_EQ(menus[1].items[1].label, "断电");
  EXPECT_TRUE(menus[1].items[1].separatorBefore);
  EXPECT_EQ(menus[1].items[2].label, "暂停");
  EXPECT_TRUE(menus[1].items[3].separatorBefore);
  EXPECT_FALSE(menus[1].items[5].checked.value());
  const auto host = shell.statusItems(false);
  ASSERT_EQ(host.size(), 3U);
  EXPECT_EQ(host[0].text, "经典 FC");
  EXPECT_EQ(host[1].text, "game.nes");
  EXPECT_EQ(host[2].text, "NROM");
  const auto buttons = shell.panelButtons();
  ASSERT_EQ(buttons.size(), 1U);
  EXPECT_EQ(buttons[0].icon, contracts::ShellIcon::PlayerPlay);
  EXPECT_EQ(buttons[0].tone, contracts::ShellTone::Success);
  EXPECT_EQ(buttons[0].hint, "运行：运行中");
  // 拖入 .nes 交给运行模块，要经请求带上编号
  shell.dropFiles({"C:\\roms\\drop.nes"});
  EXPECT_EQ(f.last("cartridges.open")->params, json({{"droppedId", 1}}));
  shell.detachModules();
}

TEST(InputController, KeysAreSentOnceAndReleasedTogether) {
  test::FakeCoreGateway core;
  logic::InputController input(core);
  input.nativeKey("KeyA", true);
  input.nativeKey("KeyA", true);
  input.nativeKey("KeyB", true);
  input.nativeKey("", true);
  input.nativeKey("KeyC", false);
  ASSERT_EQ(core.inputs.size(), 2U);
  EXPECT_EQ(core.inputs[0], json({{"type", "key"}, {"code", "KeyA"}, {"down", true}}));
  input.nativeKey("KeyA", false);
  EXPECT_EQ(core.inputs.back(), json({{"type", "key"}, {"code", "KeyA"}, {"down", false}}));
  input.releaseAllKeys();
  ASSERT_EQ(core.inputs.size(), 4U);
  EXPECT_EQ(core.inputs.back(), json({{"type", "key"}, {"code", "KeyB"}, {"down", false}}));
  input.releaseAllKeys();
  EXPECT_EQ(core.inputs.size(), 4U);
}

TEST(InputController, MouseMessagesCarryPositionDeltaButtonsAndWheel) {
  test::FakeCoreGateway core;
  logic::InputController input(core);
  input.mouseAt(contracts::MouseKind::Move, {10, 20}, 0, 0);
  input.mouseAt(contracts::MouseKind::Down, {12.5, 18}, 1, 0);
  input.mouseAt(contracts::MouseKind::Wheel, {12.5, 18}, 9, 1.5);
  ASSERT_EQ(core.inputs.size(), 3U);
  EXPECT_EQ(core.inputs[0], json({{"type", "mouse"}, {"kind", "move"}, {"x", 10.0}, {"y", 20.0}, {"dx", 0.0}, {"dy", 0.0}, {"buttons", 0}, {"wheel", 0.0}}));
  EXPECT_EQ(core.inputs[1]["dx"], 2.5);
  EXPECT_EQ(core.inputs[1]["dy"], -2.0);
  EXPECT_EQ(core.inputs[1]["buttons"], 1);
  // 按键位只取低三位
  EXPECT_EQ(core.inputs[2]["buttons"], 1);
  EXPECT_EQ(core.inputs[2]["wheel"], 1.5);
  input.mouseLeave(0);
  EXPECT_EQ(core.inputs.back()["kind"], "leave");
  EXPECT_EQ(core.inputs.back()["x"], 12.5);
  // 离开后的第一次移动不带位移
  input.mouseAt(contracts::MouseKind::Move, {100, 100}, 0, 0);
  EXPECT_EQ(core.inputs.back()["dx"], 0.0);
}

TEST(InputController, CapturedMouseStartsAtTheCenterAndStaysInsideTheFrame) {
  test::FakeCoreGateway core;
  logic::InputController input(core);
  input.mouseMoveBy(contracts::MouseKind::Move, 10, -20, 0, 0);
  EXPECT_EQ(core.inputs[0]["x"], 138.0);
  EXPECT_EQ(core.inputs[0]["y"], 100.0);
  input.mouseMoveBy(contracts::MouseKind::Move, 1000, 1000, 0, 0);
  EXPECT_EQ(core.inputs[1]["x"], 256.0);
  EXPECT_EQ(core.inputs[1]["y"], 240.0);
  EXPECT_EQ(core.inputs[1]["dx"], 1000.0);
  input.mouseMoveBy(contracts::MouseKind::Move, -1000, -1000, 0, 0);
  EXPECT_EQ(core.inputs[2]["x"], 0.0);
}

TEST(InputController, GamepadsAreReportedOnChangeRoundedAndDisconnectedWhenGone) {
  test::FakeCoreGateway core;
  logic::InputController input(core);
  contracts::GamepadSample pad;
  pad.index = 0;
  pad.id = "Xbox 360 Controller (XInput STANDARD GAMEPAD)";
  pad.buttons = {true, false};
  pad.axes = {0.123456, -2.0};
  input.pollGamepads({pad});
  input.pollGamepads({pad});
  ASSERT_EQ(core.inputs.size(), 1U);
  EXPECT_EQ(core.inputs[0]["axes"], json::array({0.12, -1.0}));
  EXPECT_EQ(core.inputs[0]["buttons"], json::array({true, false}));
  pad.buttons = {true, true};
  input.pollGamepads({pad});
  EXPECT_EQ(core.inputs.size(), 2U);
  input.pollGamepads({});
  ASSERT_EQ(core.inputs.size(), 3U);
  EXPECT_EQ(core.inputs.back()["connected"], false);
  EXPECT_EQ(core.inputs.back()["id"], pad.id);
  input.pollGamepads({});
  EXPECT_EQ(core.inputs.size(), 3U);
  input.pollGamepads({pad});
  input.disconnectGamepads();
  EXPECT_EQ(core.inputs.back()["connected"], false);
}

TEST(InputController, TooManyButtonsAndAxesAreCutAndLongIdsTruncated) {
  test::FakeCoreGateway core;
  logic::InputController input(core);
  contracts::GamepadSample pad;
  pad.id = std::string(300, 'x');
  pad.buttons = std::vector<bool>(40, true);
  pad.axes = std::vector<double>(20, 0.5);
  input.pollGamepads({pad});
  EXPECT_EQ(core.inputs[0]["buttons"].size(), 32U);
  EXPECT_EQ(core.inputs[0]["axes"].size(), 16U);
  EXPECT_EQ(core.inputs[0]["id"].get<std::string>().size(), 256U);
}

}  // namespace
}  // namespace ember::win32
