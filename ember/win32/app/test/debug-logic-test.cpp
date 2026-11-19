/*
 * ------------------------------------------------------------------------
 * 名称：debug-logic-test.cpp
 * 说明：调试服务模块逻辑的测试：状态的读取、“调试模式”项的显示规则（没有开启不显示、绿色、没有监听时警告色与原因、连接数）、控制器（初始读取、事件、核心重启、断开）与登记项（右侧、点击打开首选项）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：只检查点击发出的命令标识。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include "logic/debug/debug-controller.hpp"
#include "logic/debug/debug-module.hpp"
#include "logic/debug/debug-rules.hpp"
#include "support/fakes.hpp"

namespace ember::win32 {
namespace {

using contracts::CoreReply;
using nlohmann::json;

json statusJson(bool enabled, bool listening, int port, int connections = 0, const std::string& error = "") {
  json status = {{"enabled", enabled}, {"listening", listening}, {"port", port}, {"connections", json::array()}};
  for (int i = 0; i < connections; ++i) status["connections"].push_back({{"id", i + 1}, {"client", "c"}});
  if (!error.empty()) status["error"] = error;
  return status;
}

TEST(DebugRules, ParsesStatusAndRejectsBadShapes) {
  const auto state = logic::parseDebugStatus(statusJson(true, true, 5688, 2));
  ASSERT_TRUE(state.has_value());
  EXPECT_TRUE(state->enabled && state->listening);
  EXPECT_EQ(state->port, 5688);
  EXPECT_EQ(state->connections, 2);
  EXPECT_FALSE(logic::parseDebugStatus(json::object()).has_value());
  EXPECT_FALSE(logic::parseDebugStatus(json::array()).has_value());
}

TEST(DebugRules, ItemIsHiddenUntilEnabled) {
  EXPECT_FALSE(logic::debugItemView(std::nullopt).has_value());
  EXPECT_FALSE(logic::debugItemView(logic::parseDebugStatus(statusJson(false, false, 0))).has_value());
}

TEST(DebugRules, ListeningShowsGreenTextWithAddressAndConnections) {
  auto view = logic::debugItemView(logic::parseDebugStatus(statusJson(true, true, 5688)));
  ASSERT_TRUE(view.has_value());
  EXPECT_EQ(view->text, "调试模式");
  EXPECT_EQ(view->tone, contracts::ShellTone::Success);
  EXPECT_EQ(view->title, "调试服务：ws://127.0.0.1:5688");
  view = logic::debugItemView(logic::parseDebugStatus(statusJson(true, true, 5688, 2)));
  EXPECT_EQ(view->title, "调试服务：ws://127.0.0.1:5688，2 个连接");
}

TEST(DebugRules, NotListeningShowsAWarningWithTheReason) {
  auto view = logic::debugItemView(logic::parseDebugStatus(statusJson(true, false, 0, 0, "端口被占用")));
  ASSERT_TRUE(view.has_value());
  EXPECT_EQ(view->text, "调试模式");
  EXPECT_EQ(view->tone, contracts::ShellTone::Warning);
  EXPECT_EQ(view->title, "调试服务没有监听：端口被占用");
  EXPECT_EQ(logic::debugItemView(logic::parseDebugStatus(statusJson(true, false, 0)))->title, "调试服务没有监听");
}

struct DebugFixture {
  test::FakeCoreGateway core;
  test::FakeBroker broker{core};
  test::FakeShellApi shell;
  logic::DebugController controller{broker, core};
  std::function<void()> detach;

  DebugFixture() { core.replies["debug.status"] = [](const json&) { return CoreReply::success(statusJson(true, true, 5688)); }; }
  void attach() { detach = controller.attach(shell); }
  [[nodiscard]] int reads() const {
    int total = 0;
    for (const auto& call : core.calls) total += call.name == "debug.status" ? 1 : 0;
    return total;
  }
};

TEST(DebugController, ReadsWhenAttachedAndOnStatusEvents) {
  DebugFixture f;
  EXPECT_FALSE(f.controller.state().has_value());
  f.attach();
  ASSERT_TRUE(f.controller.state().has_value());
  EXPECT_EQ(f.controller.state()->port, 5688);
  EXPECT_GE(f.shell.refreshes, 1);
  f.core.replies["debug.status"] = [](const json&) { return CoreReply::success(statusJson(true, true, 5688, 3)); };
  f.core.emitEvent("debug.statusChanged", {{"enabled", true}, {"listening", true}, {"port", 5688}, {"connections", 3}});
  EXPECT_EQ(f.controller.state()->connections, 3);
  EXPECT_EQ(f.reads(), 2);
  // 其他事件不读
  f.core.emitEvent("runtime.stateChanged", json::object());
  EXPECT_EQ(f.reads(), 2);
}

TEST(DebugController, UnchangedStatusDoesNotRedrawTheStatusBar) {
  DebugFixture f;
  f.attach();
  const int refreshes = f.shell.refreshes;
  f.core.emitEvent("debug.statusChanged", json::object());
  EXPECT_EQ(f.shell.refreshes, refreshes);
}

TEST(DebugController, FailedReadGoesBackToUnknown) {
  DebugFixture f;
  f.attach();
  f.core.replies["debug.status"] = [](const json&) { return CoreReply::failure("x.failed", "boom"); };
  f.core.emitEvent("debug.statusChanged", json::object());
  EXPECT_FALSE(f.controller.state().has_value());
}

TEST(DebugController, CoreRestartClearsAndRereads) {
  DebugFixture f;
  f.attach();
  f.core.setState(contracts::CoreState::Starting);
  EXPECT_FALSE(f.controller.state().has_value());
  f.core.setState(contracts::CoreState::Running);
  EXPECT_TRUE(f.controller.state().has_value());
}

TEST(DebugController, DetachingStopsEverything) {
  DebugFixture f;
  f.attach();
  f.detach();
  const int reads = f.reads();
  f.core.emitEvent("debug.statusChanged", json::object());
  EXPECT_EQ(f.reads(), reads);
  EXPECT_EQ(f.core.subscriberCount(), 0U);
}

TEST(DebugModule, ItemIsOnTheRightAndClickingOpensPreferences) {
  DebugFixture f;
  const auto module = logic::createDebugModule(f.controller);
  EXPECT_EQ(module.name, "debug");
  ASSERT_EQ(module.statusItems.size(), 1U);
  const auto& item = module.statusItems[0];
  EXPECT_EQ(item.id, "debug.service");
  EXPECT_TRUE(item.alignEnd);
  EXPECT_FALSE(item.view().has_value());
  f.attach();
  ASSERT_TRUE(item.view().has_value());
  EXPECT_EQ(item.view()->tone, contracts::ShellTone::Success);
  ASSERT_TRUE(item.click != nullptr);
  item.click(f.shell);
  EXPECT_EQ(f.shell.commands, (std::vector<std::string>{"preferences.open"}));
}

}  // namespace
}  // namespace ember::win32
