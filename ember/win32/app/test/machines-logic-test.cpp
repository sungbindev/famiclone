/*
 * ------------------------------------------------------------------------
 * 名称：machines-logic-test.cpp
 * 说明：机型模块逻辑的测试：纯规则（机型清单的读取、BIOS 状态文字、对话框的行、错误说明）、“机型与 BIOS”控制器（读取、添加、替换、移除、打开目录、事件、核心重启、断开）与机型模块的登记项。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include "logic/machines/bios-controller.hpp"
#include "logic/machines/machines-module.hpp"
#include "logic/machines/machines-rules.hpp"
#include "support/fakes.hpp"

namespace ember::win32 {
namespace {

using contracts::CoreReply;
using contracts::MachinesLoad;
using nlohmann::json;

json machine(const std::string& id, const std::string& name, bool needsBios, bool supported, const json& bios = nullptr) {
  json item = {{"id", id}, {"name", name}, {"needsBios", needsBios}, {"supported", supported}};
  if (!bios.is_null()) item["bios"] = bios;
  return item;
}

json machinesResult() {
  return {{"machines", json::array({machine("fc", "经典 FC", false, true), machine("sb97", "小霸王 97", true, true), machine("sb2000", "小霸王 2000", true, true, {{"fileName", "SB2K_BIOS.NES"}, {"size", 524304}}),
                                    machine("bbk10", "步步高软驱一号", true, false)})}};
}

TEST(MachinesRules, BiosStatusAndRowsOnlyForMachinesThatNeedBios) {
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

TEST(MachinesRules, BadShapesAreRejectedOrSkipped) {
  EXPECT_FALSE(logic::parseBiosMachines(json::object()).has_value());
  EXPECT_FALSE(logic::parseBiosMachines({{"machines", 1}}).has_value());
  EXPECT_FALSE(logic::parseBiosMachine(json::array()).has_value());
  EXPECT_FALSE(logic::parseBiosMachine({{"name", "没有标识"}}).has_value());
  const json broken = {{"machines", json::array({machine("sb97", "小霸王 97", true, true), 5, {{"name", "没有标识"}}})}};
  EXPECT_EQ(logic::parseBiosMachines(broken)->size(), 1U);
}

TEST(MachinesRules, ErrorTextFallsBack) {
  EXPECT_EQ(logic::machineErrorText({"x", "核心未启动"}), "核心未启动");
  EXPECT_EQ(logic::machineErrorText({"x", ""}), "操作失败。");
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
  EXPECT_EQ(f.controller.view().load, MachinesLoad::Loading);
  f.attach();
  const auto view = f.controller.view();
  EXPECT_EQ(view.load, MachinesLoad::Ready);
  ASSERT_EQ(view.rows.size(), 3U);
  EXPECT_EQ(view.rows[1].status, "SB2K_BIOS.NES（512 KB）");
  EXPECT_FALSE(view.rows[2].supported);
  EXPECT_GE(f.changes, 1);
}

TEST(BiosController, ReadFailureShowsTheCoreMessage) {
  BiosFixture f;
  f.fail("machines.list", "核心未启动");
  f.attach();
  EXPECT_EQ(f.controller.view().load, MachinesLoad::Failed);
  EXPECT_EQ(f.controller.view().message, "核心未启动");
  EXPECT_TRUE(f.controller.view().rows.empty());
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
  EXPECT_EQ(f.controller.view().load, MachinesLoad::Loading);
  EXPECT_TRUE(f.controller.view().rows.empty());
  f.detach();
  EXPECT_EQ(f.core.subscriberCount(), 0U);
}

TEST(MachinesModule, RegistersTheDialogTheCommandAndTheSettingsMenuItem) {
  test::FakeCoreGateway core;
  test::FakeBroker broker{core};
  logic::BiosController bios(broker, core);
  const auto module = logic::createMachinesModule(bios);
  EXPECT_EQ(module.name, "machines");
  ASSERT_EQ(module.dialogs.size(), 1U);
  EXPECT_EQ(module.dialogs[0].id, "machines.bios");
  EXPECT_EQ(module.dialogs[0].title, "机型与 BIOS");
  ASSERT_EQ(module.commands.size(), 1U);
  EXPECT_EQ(module.commands[0].id, "machines.open");
  EXPECT_TRUE(module.commands[0].defaultShortcut.empty());
  test::FakeShellApi shell;
  module.commands[0].run(shell);
  EXPECT_EQ(shell.dialogs, (std::vector<std::string>{"machines.bios"}));
  ASSERT_EQ(module.menuItems.size(), 1U);
  EXPECT_EQ(module.menuItems[0].menu, "settings");
  EXPECT_EQ(module.menuItems[0].command, "machines.open");
}

TEST(MachinesModule, AttachConnectsTheControllerAndDetachDisconnectsIt) {
  test::FakeCoreGateway core;
  test::FakeBroker broker{core};
  test::FakeShellApi shell;
  logic::BiosController bios(broker, core);
  const auto module = logic::createMachinesModule(bios);
  const auto detach = module.attach(shell);
  EXPECT_EQ(core.subscriberCount(), 2U);
  detach();
  EXPECT_EQ(core.subscriberCount(), 0U);
}

}  // namespace
}  // namespace ember::win32
