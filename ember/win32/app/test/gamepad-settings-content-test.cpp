/*
 * ------------------------------------------------------------------------
 * 名称：gamepad-settings-content-test.cpp
 * 说明：手柄设置对话框内容的测试：读取前没有页签、读取失败显示说明、两个玩家页签与手柄图（10 个键）取自模型、点键学习、输入来源与使用的手柄、说明行与“清除”、恢复默认、切换页签、定时读手柄（间隔随模型）、销毁后取消订阅；手柄图的版面（键不出界、不重叠）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-20
 * 备注：用控件库的离屏根容器测试，模型是假的；点击与按键用根容器的事件，缩放 1.0。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <map>
#include <memory>

#include "contracts/input/gamepad-settings-model.hpp"
#include "rujian/ui/root.hpp"
#include "ui/input/gamepad-settings-content.hpp"
#include "ui/input/pad-layout.hpp"

namespace ember::win32 {
namespace {

constexpr int kVirtualKeyRight = 0x27;

/** 假的手柄设置模型：记下操作，内容由测试直接改。 */
class FakePadSettings final : public contracts::GamepadSettingsModel {
 public:
  contracts::PadSettingsView current;
  std::vector<std::string> calls;
  std::map<int, std::function<void()>> handlers;
  unsigned interval{500};
  int polls{0};

  [[nodiscard]] contracts::PadSettingsView view() const override { return current; }
  void selectPlayer(int player) override { calls.push_back("player " + std::to_string(player)); }
  void setSource(int player, contracts::PadInputSource source) override { calls.push_back("source " + std::to_string(player) + (source == contracts::PadInputSource::Gamepad ? " pad" : " key")); }
  void pickKey(int player, const std::string& key) override { calls.push_back("pick " + std::to_string(player) + " " + key); }
  void clearSelected(int player) override { calls.push_back("clear " + std::to_string(player)); }
  void assignDevice(int player, const std::string& deviceValue) override { calls.push_back("assign " + std::to_string(player) + " [" + deviceValue + "]"); }
  void reset() override { calls.push_back("reset"); }
  void cancelLearning() override { calls.push_back("cancel"); }
  [[nodiscard]] bool keyPressed(const std::string&) override { return false; }
  [[nodiscard]] bool learningKey() const override { return false; }
  void poll() override { ++polls; }
  [[nodiscard]] unsigned pollIntervalMs() const override { return interval; }
  [[nodiscard]] std::function<void()> subscribe(std::function<void()> handler) override {
    const int id = static_cast<int>(handlers.size()) + 1;
    handlers[id] = std::move(handler);
    return [this, id] { handlers.erase(id); };
  }
  void notify() {
    const auto copy = handlers;
    for (const auto& [id, handler] : copy) handler();
  }
};

contracts::PadPlayerView playerView() {
  contracts::PadPlayerView player;
  for (const char* id : {"up", "down", "left", "right", "a", "b", "turboA", "turboB", "select", "start"}) {
    contracts::PadKeyView key;
    key.id = id;
    key.role = id;
    key.current = "未绑定";
    key.unbound = true;
    key.tip = std::string("绑定键盘按键：") + id;
    player.keys.push_back(key);
  }
  player.keys[0].current = "↑";
  player.keys[0].unbound = false;
  player.message = "点手柄上的键，再按下键盘上的键来设置。";
  player.clearLabel = "清除绑定";
  return player;
}

struct Scene {
  // 模型要比根容器活得久：控件销毁时还要取消订阅
  FakePadSettings model;
  rujian::ui::Root root{rujian::ui::ThemeName::Dark, 1.0};
  ui::GamepadSettingsContent* content{nullptr};

  Scene() {
    model.current.load = contracts::PadSettingsLoad::Ready;
    model.current.players = {playerView(), playerView()};
  }

  void open() {
    auto created = std::make_unique<ui::GamepadSettingsContent>(model);
    content = created.get();
    root.setContent(std::move(created));
    root.layout({600, 700});
  }

  void click(const rujian::ui::Widget& widget) {
    const auto& box = widget.bounds();
    const rujian::ui::Point at{box.x + box.width / 2, box.y + box.height / 2};
    root.mouseMove(at, 0);
    root.mouseDown(at, rujian::ui::MouseButton::Left, 0, 0);
    root.mouseUp(at, rujian::ui::MouseButton::Left, 0);
  }
};

TEST(PadLayout, HasTenKeysInsideTheCanvasAndNoTwoOverlap) {
  const auto layout = ui::padLayout();
  ASSERT_EQ(layout.keys.size(), 10U);
  for (const auto& key : layout.keys) {
    const double height = key.round ? key.width : key.height;
    EXPECT_GE(key.cx - key.width / 2, 0) << key.id;
    EXPECT_LE(key.cx + key.width / 2, layout.canvasWidth) << key.id;
    EXPECT_GE(key.cy - height / 2, 0) << key.id;
    EXPECT_LE(key.cy + height / 2, layout.canvasHeight) << key.id;
  }
  for (std::size_t i = 0; i < layout.keys.size(); ++i) {
    for (std::size_t j = i + 1; j < layout.keys.size(); ++j) {
      const auto& a = layout.keys[i];
      const auto& b = layout.keys[j];
      const double ah = a.round ? a.width : a.height;
      const double bh = b.round ? b.width : b.height;
      const bool separated = a.cx + a.width / 2 <= b.cx - b.width / 2 || b.cx + b.width / 2 <= a.cx - a.width / 2 || a.cy + ah / 2 <= b.cy - bh / 2 || b.cy + bh / 2 <= a.cy - ah / 2;
      EXPECT_TRUE(separated) << a.id << " 与 " << b.id << " 重叠";
    }
  }
}

TEST(GamepadSettingsContent, NothingButTheIntroBeforeTheDataIsRead) {
  Scene s;
  s.model.current = contracts::PadSettingsView{};
  s.open();
  EXPECT_FALSE(s.content->tabs().visible());
  EXPECT_FALSE(s.content->resetButton().parent()->visible());
  EXPECT_FALSE(s.content->failedState().visible());
}

TEST(GamepadSettingsContent, ReadFailureShowsTheMessage) {
  Scene s;
  s.model.current = contracts::PadSettingsView{contracts::PadSettingsLoad::Failed, "核心未启动", {}, 0};
  s.open();
  EXPECT_TRUE(s.content->failedState().visible());
  EXPECT_FALSE(s.content->tabs().visible());
}

TEST(GamepadSettingsContent, ShowsTwoPlayerTabsWithAPadDiagramEach) {
  Scene s;
  s.open();
  EXPECT_TRUE(s.content->tabs().visible());
  ASSERT_EQ(s.content->players().size(), 2U);
  EXPECT_EQ(s.content->tabs().selectedId(), "slot0");
  const auto& first = s.content->players()[0];
  ASSERT_NE(first.diagram->key("up"), nullptr);
  EXPECT_EQ(first.diagram->key("up")->hotspot().current, "↑");
  EXPECT_TRUE(first.diagram->key("a")->hotspot().unbound);
  EXPECT_EQ(first.status->text(), "点手柄上的键，再按下键盘上的键来设置。");
  EXPECT_EQ(first.source->value(), "key");
  // 键盘来源时没有“使用的手柄”
  EXPECT_FALSE(first.deviceField->visible());
  EXPECT_TRUE(first.diagram->key("up")->enabled());
  EXPECT_FALSE(first.clear->enabled());
}

TEST(GamepadSettingsContent, ClickingAKeyTellsTheModel) {
  Scene s;
  s.open();
  s.click(*s.content->players()[0].diagram->key("a"));
  EXPECT_EQ(s.model.calls, (std::vector<std::string>{"pick 0 a"}));
}

TEST(GamepadSettingsContent, KeysShowTheLearningAndSelectedStateFromTheModel) {
  Scene s;
  s.open();
  auto& player = s.model.current.players[0];
  player.keys[4].learning = true;
  player.keys[4].selected = true;
  player.keys[4].current = "请按键";
  player.keys[4].unbound = false;
  player.message = "设置“A”：请按下键盘上的键（Esc 取消）";
  player.canClear = true;
  s.model.notify();
  const auto& widgets = s.content->players()[0];
  EXPECT_TRUE(widgets.diagram->key("a")->learning());
  EXPECT_TRUE(widgets.diagram->key("a")->selected());
  EXPECT_EQ(widgets.diagram->key("a")->hotspot().current, "请按键");
  EXPECT_EQ(widgets.status->text(), "设置“A”：请按下键盘上的键（Esc 取消）");
  EXPECT_TRUE(widgets.clear->enabled());
  s.click(*widgets.clear);
  EXPECT_EQ(s.model.calls, (std::vector<std::string>{"clear 0"}));
}

TEST(GamepadSettingsContent, SwitchingToTheGamepadSourceShowsTheDeviceChoiceAndDisablesKeysWithoutADevice) {
  Scene s;
  s.open();
  s.model.current.players[0].source = contracts::PadInputSource::Gamepad;
  s.model.current.players[0].showDevice = true;
  s.model.current.players[0].keysEnabled = false;
  s.model.current.players[0].devices = {{"0:Xbox", "Xbox"}};
  s.model.current.players[0].message = "先在上面选择要使用的手柄。";
  s.model.notify();
  const auto& widgets = s.content->players()[0];
  EXPECT_EQ(widgets.source->value(), "pad");
  EXPECT_TRUE(widgets.deviceField->visible());
  EXPECT_FALSE(widgets.diagram->key("up")->enabled());
  EXPECT_EQ(widgets.device->value(), "");
  s.model.current.players[0].assignedDevice = "0:Xbox";
  s.model.current.players[0].keysEnabled = true;
  s.model.notify();
  EXPECT_EQ(widgets.device->value(), "0:Xbox");
  EXPECT_TRUE(widgets.diagram->key("up")->enabled());
}

TEST(GamepadSettingsContent, PickingASourceOrADeviceTellsTheModel) {
  Scene s;
  s.open();
  const auto& widgets = s.content->players()[0];
  // 点“实体手柄”分段
  const auto& box = widgets.source->bounds();
  const rujian::ui::Point at{box.x + box.width * 0.75, box.y + box.height / 2};
  s.root.mouseMove(at, 0);
  s.root.mouseDown(at, rujian::ui::MouseButton::Left, 0, 0);
  s.root.mouseUp(at, rujian::ui::MouseButton::Left, 0);
  EXPECT_EQ(s.model.calls, (std::vector<std::string>{"source 0 pad"}));
  // 展开下拉框选一个手柄
  s.model.calls.clear();
  s.model.current.players[0].source = contracts::PadInputSource::Gamepad;
  s.model.current.players[0].showDevice = true;
  s.model.current.players[0].devices = {{"0:Xbox", "Xbox"}};
  s.model.notify();
  s.click(*widgets.device);
  ASSERT_TRUE(widgets.device->open());
  const auto row = widgets.device->optionRect(1);
  ASSERT_TRUE(row.has_value());
  const rujian::ui::Point option{row->x + row->width / 2, row->y + row->height / 2};
  s.root.mouseDown(option, rujian::ui::MouseButton::Left, 0, 0);
  s.root.mouseUp(option, rujian::ui::MouseButton::Left, 0);
  EXPECT_EQ(s.model.calls, (std::vector<std::string>{"assign 0 [0:Xbox]"}));
}

TEST(GamepadSettingsContent, ResetButtonAndTabSwitchingTellTheModel) {
  Scene s;
  s.open();
  s.click(s.content->resetButton());
  EXPECT_EQ(s.model.calls, (std::vector<std::string>{"reset"}));
  s.model.calls.clear();
  // 用键盘切换到玩家 2 的页签
  s.root.setFocus(s.content->tabs().tabBar(), true);
  s.root.keyDown(kVirtualKeyRight, 0, false);
  EXPECT_EQ(s.model.calls, (std::vector<std::string>{"player 1"}));
  // 模型里的当前玩家变了，页签跟着变
  s.model.current.activePlayer = 0;
  s.model.notify();
  EXPECT_EQ(s.content->tabs().selectedId(), "slot0");
}

TEST(GamepadSettingsContent, ReadsTheGamepadsOnATimerWhoseIntervalComesFromTheModel) {
  Scene s;
  s.open();
  const auto timers = s.root.pendingTimers();
  ASSERT_EQ(timers.size(), 1U);
  s.root.fireTimer(timers[0]);
  EXPECT_EQ(s.model.polls, 1);
  // 读完之后重新排下一次
  EXPECT_EQ(s.root.pendingTimers().size(), 1U);
  s.root.fireTimer(s.root.pendingTimers()[0]);
  EXPECT_EQ(s.model.polls, 2);
}

TEST(GamepadSettingsContent, DestroyingUnsubscribesFromTheModel) {
  Scene s;
  s.open();
  EXPECT_EQ(s.model.handlers.size(), 1U);
  s.root.setContent(nullptr);
  EXPECT_TRUE(s.model.handlers.empty());
}

}  // namespace
}  // namespace ember::win32
