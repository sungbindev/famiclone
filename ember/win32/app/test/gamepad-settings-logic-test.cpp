/*
 * ------------------------------------------------------------------------
 * 名称：gamepad-settings-logic-test.cpp
 * 说明：手柄设置模块逻辑的测试：纯规则（物理键与手柄来源的显示文字、绑定的增删与冲突、设备选项、从手柄状态认出被按下的按钮或轴、槽位与 JSON 互转）、手柄设置控制器（读取、学习键盘与实体手柄、清除、指定手柄、恢复默认、事件、通知、核心重启、断开）与输入模块的登记项。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-20
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include "logic/input/gamepad-settings-controller.hpp"
#include "logic/input/input-module.hpp"
#include "logic/input/pad-rules.hpp"
#include "support/fakes.hpp"

namespace ember::win32 {
namespace {

using contracts::CoreReply;
using contracts::GamepadSample;
using contracts::PadInputSource;
using contracts::PadSettingsLoad;
using logic::PadDevice;
using logic::PadSlot;
using logic::PadSource;
using nlohmann::json;

/** 默认的绑定：玩家 1 是出厂默认的键盘键，玩家 2 没有。 */
json defaultSlots() {
  return json::array({{{"keys", json::array({{{"button", "up"}, {"code", "ArrowUp"}},
                                             {{"button", "down"}, {"code", "ArrowDown"}},
                                             {{"button", "left"}, {"code", "ArrowLeft"}},
                                             {{"button", "right"}, {"code", "ArrowRight"}},
                                             {{"button", "b"}, {"code", "KeyZ"}},
                                             {{"button", "a"}, {"code", "KeyX"}},
                                             {{"button", "select"}, {"code", "ShiftRight"}},
                                             {{"button", "start"}, {"code", "Enter"}}})}},
                      {{"keys", json::array()}}});
}

std::vector<PadSlot> slotsOf(const json& value) { return *logic::parseSlots(value); }

GamepadSample pad(int index, const std::string& id, std::vector<bool> buttons = {}, std::vector<double> axes = {}, bool connected = true) {
  GamepadSample sample;
  sample.index = index;
  sample.id = id;
  sample.connected = connected;
  sample.buttons = std::move(buttons);
  sample.axes = std::move(axes);
  return sample;
}

TEST(PadRules, KeyLabelsAreReadable) {
  EXPECT_EQ(logic::keyLabel("KeyZ"), "Z");
  EXPECT_EQ(logic::keyLabel("Digit5"), "5");
  EXPECT_EQ(logic::keyLabel("ArrowUp"), "↑");
  EXPECT_EQ(logic::keyLabel("ArrowRight"), "→");
  EXPECT_EQ(logic::keyLabel("Enter"), "回车");
  EXPECT_EQ(logic::keyLabel("ShiftRight"), "右 Shift");
  EXPECT_EQ(logic::keyLabel("ControlLeft"), "左 Control");
  EXPECT_EQ(logic::keyLabel("Numpad7"), "小键盘 7");
  EXPECT_EQ(logic::keyLabel("Backslash"), "\\");
  EXPECT_EQ(logic::keyLabel("F5"), "F5");
}

TEST(PadRules, OnlyLettersAndDigitsAreCapturable) {
  EXPECT_TRUE(logic::isCapturableCode("KeyZ"));
  EXPECT_TRUE(logic::isCapturableCode("Numpad1"));
  EXPECT_FALSE(logic::isCapturableCode(""));
  EXPECT_FALSE(logic::isCapturableCode("Key Z"));
  EXPECT_FALSE(logic::isCapturableCode(std::string(33, 'A')));
}

TEST(PadRules, BindKeyFreesTheKeyEverywhereAndReplacesTheButtonsOldKey) {
  auto slots = slotsOf(defaultSlots());
  slots = logic::bindKey(slots, 1, "a", "KeyZ");
  // 玩家 1 的 B 原来绑着 KeyZ，被解除
  EXPECT_FALSE(logic::keyOf(slots[0], "b").has_value());
  EXPECT_EQ(logic::keyOf(slots[1], "a"), "KeyZ");
  // 同一个键原来的物理键被替换
  slots = logic::bindKey(slots, 1, "a", "KeyQ");
  EXPECT_EQ(logic::keyOf(slots[1], "a"), "KeyQ");
  EXPECT_EQ(slots[1].keys.size(), 1U);
  // 不改传入的数据
  const auto original = slotsOf(defaultSlots());
  (void)logic::bindKey(original, 0, "up", "KeyW");
  EXPECT_EQ(logic::keyOf(original[0], "up"), "ArrowUp");
  EXPECT_EQ(logic::keyOf(logic::clearKey(slots, 0, "up")[0], "up"), std::nullopt);
}

TEST(PadRules, DeviceKeysRoundTripAndLabelsMarkTheOccurrenceAndConnection) {
  const PadDevice second{"Xbox 360 Controller (XInput STANDARD GAMEPAD)", 1};
  EXPECT_EQ(logic::deviceKey(second), "1:Xbox 360 Controller (XInput STANDARD GAMEPAD)");
  EXPECT_EQ(logic::parseDeviceKey(logic::deviceKey(second)), second);
  EXPECT_EQ(logic::parseDeviceKey("0:a:b"), (PadDevice{"a:b", 0}));
  EXPECT_FALSE(logic::parseDeviceKey("x:abc").has_value());
  EXPECT_FALSE(logic::parseDeviceKey("1:").has_value());
  EXPECT_FALSE(logic::parseDeviceKey("").has_value());
  EXPECT_EQ(logic::deviceLabel({"pad", 0}, true), "pad");
  EXPECT_EQ(logic::deviceLabel({"pad", 1}, true), "pad（第 2 个）");
  EXPECT_EQ(logic::deviceLabel({"pad", 0}, false), "pad（未连接）");
}

TEST(PadRules, ListDevicesOrdersByIndexSkipsDisconnectedAndCountsOccurrences) {
  const auto devices = logic::listDevices({pad(2, "A"), pad(0, "B"), pad(1, "A"), pad(3, "C", {}, {}, false)});
  ASSERT_EQ(devices.size(), 3U);
  EXPECT_EQ(devices[0], (PadDevice{"B", 0}));
  EXPECT_EQ(devices[1], (PadDevice{"A", 0}));
  EXPECT_EQ(devices[2], (PadDevice{"A", 1}));
  const std::vector<GamepadSample> pads{pad(2, "A"), pad(1, "A")};
  ASSERT_NE(logic::findPad(pads, {"A", 1}), nullptr);
  EXPECT_EQ(logic::findPad(pads, {"A", 1})->index, 2);
  EXPECT_EQ(logic::findPad(pads, {"A", 2}), nullptr);
  EXPECT_EQ(logic::findPad(pads, {"B", 0}), nullptr);
}

TEST(PadRules, AssignDeviceReleasesItFromTheOtherPlayerAndKeepsBindingsOnlyForTheSameDevice) {
  auto slots = slotsOf(defaultSlots());
  slots = logic::assignDevice(slots, 0, PadDevice{"Pad", 0});
  slots = logic::bindGamepad(slots, 0, "a", PadSource{"button", 0, {}});
  ASSERT_TRUE(slots[0].gamepad.has_value());
  EXPECT_EQ(logic::bindingOf(slots[0].gamepad, "a")->index, 0);
  // 指定给另一个玩家：从原来的玩家名下取消
  slots = logic::assignDevice(slots, 1, PadDevice{"Pad", 0});
  EXPECT_FALSE(slots[0].gamepad.has_value());
  EXPECT_TRUE(slots[1].gamepad.has_value());
  // 同一个手柄再指定一次：绑定保留；换成别的手柄：绑定清空；不使用：取消指定
  slots = logic::bindGamepad(slots, 1, "b", PadSource{"axis", 1, "negative"});
  slots = logic::assignDevice(slots, 1, PadDevice{"Pad", 0});
  EXPECT_EQ(slots[1].gamepad->bindings.size(), 1U);
  slots = logic::assignDevice(slots, 1, PadDevice{"Other", 0});
  EXPECT_TRUE(slots[1].gamepad->bindings.empty());
  slots = logic::assignDevice(slots, 1, std::nullopt);
  EXPECT_FALSE(slots[1].gamepad.has_value());
  // 没有指定实体手柄时绑定不生效
  EXPECT_FALSE(logic::bindGamepad(slots, 1, "a", PadSource{"button", 3, {}})[1].gamepad.has_value());
}

TEST(PadRules, BindingsReplaceAndClear) {
  auto slots = logic::assignDevice(slotsOf(defaultSlots()), 0, PadDevice{"Pad", 0});
  slots = logic::bindGamepad(slots, 0, "a", PadSource{"button", 0, {}});
  slots = logic::bindGamepad(slots, 0, "a", PadSource{"axis", 2, "positive"});
  ASSERT_EQ(slots[0].gamepad->bindings.size(), 1U);
  EXPECT_EQ(slots[0].gamepad->bindings[0].kind, "axis");
  EXPECT_EQ(logic::sourceLabel({"axis", 2, "positive"}), "轴 2 正向");
  EXPECT_EQ(logic::sourceLabel({"axis", 0, "negative"}), "轴 0 负向");
  EXPECT_EQ(logic::sourceLabel({"button", 7, {}}), "按钮 7");
  slots = logic::clearGamepad(slots, 0, "a");
  EXPECT_TRUE(slots[0].gamepad->bindings.empty());
}

TEST(PadRules, DetectSourceUsesTheBaselineForButtonsAndAxes) {
  const GamepadSample resting = pad(0, "Pad", {false, false, true}, {0.0, 0.0});
  const auto baseline = logic::baselineOf(resting);
  EXPECT_FALSE(logic::detectSource(baseline, resting).has_value());
  // 开始前就按着的按钮不算，新按下的算
  EXPECT_FALSE(logic::detectSource(baseline, pad(0, "Pad", {false, false, true}, {0.0, 0.0})).has_value());
  EXPECT_EQ(logic::detectSource(baseline, pad(0, "Pad", {false, true, true}, {0.0, 0.0})), (PadSource{"button", 1, {}}));
  // 轴：离开基准超过 0.5 且绝对值超过 0.5，方向取符号
  EXPECT_EQ(logic::detectSource(baseline, pad(0, "Pad", {false, false, true}, {0.0, -0.9})), (PadSource{"axis", 1, "negative"}));
  EXPECT_EQ(logic::detectSource(baseline, pad(0, "Pad", {false, false, true}, {0.8, 0.0})), (PadSource{"axis", 0, "positive"}));
  EXPECT_FALSE(logic::detectSource(baseline, pad(0, "Pad", {false, false, true}, {0.4, 0.0})).has_value());
  // 基准里本来就推到底的轴不算
  EXPECT_FALSE(logic::detectSource(logic::baselineOf(pad(0, "Pad", {}, {0.9})), pad(0, "Pad", {}, {0.9})).has_value());
  // 编号超出核心接受范围的不认
  std::vector<bool> many(40, false);
  many[35] = true;
  EXPECT_FALSE(logic::detectSource(logic::baselineOf(pad(0, "Pad", std::vector<bool>(40, false))), pad(0, "Pad", many)).has_value());
}

TEST(PadRules, SlotsRoundTripThroughJsonAndBadShapesAreRejected) {
  auto slots = logic::assignDevice(slotsOf(defaultSlots()), 0, PadDevice{"Pad", 2});
  slots = logic::bindGamepad(slots, 0, "a", PadSource{"axis", 3, "negative"});
  slots = logic::bindGamepad(slots, 0, "b", PadSource{"button", 4, {}});
  const auto again = logic::parseSlots(logic::slotsToJson(slots));
  ASSERT_TRUE(again.has_value());
  EXPECT_EQ(*again, slots);
  const json bound = logic::slotsToJson(slots)[0]["gamepad"]["bindings"];
  EXPECT_EQ(bound[0]["direction"], "negative");
  EXPECT_FALSE(bound[1].contains("direction"));
  EXPECT_FALSE(logic::parseSlots(json::object()).has_value());
  EXPECT_FALSE(logic::parseSlots(json::array({json::object()})).has_value());
  EXPECT_FALSE(logic::parseSlots(json::array({{{"keys", json::array({{{"button", "nothing"}, {"code", "KeyA"}}})}}})).has_value());
}

/** 假的手柄读取：返回事先设好的手柄。 */
class FakeGamepadReader : public contracts::GamepadReader {
 public:
  std::vector<GamepadSample> samples;
  [[nodiscard]] std::vector<GamepadSample> read() override { return samples; }
};

/** 带假核心的手柄设置控制器。 */
struct PadFixture {
  test::FakeCoreGateway core;
  test::FakeBroker broker{core};
  test::FakeShellApi shell;
  FakeGamepadReader reader;
  logic::GamepadSettingsController controller{broker, core, reader};
  std::function<void()> detach;
  std::function<void()> unsubscribe;
  int changes{0};

  PadFixture() {
    reply("input.getBindings", {{"slots", defaultSlots()}});
    // 保存与恢复默认：核心把收到的槽位原样存下
    core.replies["input.setBindings"] = [](const json& params) { return CoreReply::success({{"slots", params["slots"]}}); };
    reply("input.resetBindings", {{"slots", defaultSlots()}});
    unsubscribe = controller.subscribe([this] { ++changes; });
  }

  void attach() { detach = controller.attach(shell); }
  void reply(const std::string& name, const json& result) {
    core.replies[name] = [result](const json&) { return CoreReply::success(result); };
  }
  [[nodiscard]] const test::FakeCoreGateway::Call* last(const std::string& name) const {
    for (auto it = core.calls.rbegin(); it != core.calls.rend(); ++it) {
      if (it->name == name) return &*it;
    }
    return nullptr;
  }
  [[nodiscard]] contracts::PadPlayerView player(int index) const { return controller.view().players[static_cast<std::size_t>(index)]; }
  [[nodiscard]] static const contracts::PadKeyView& key(const contracts::PadPlayerView& view, const std::string& id) {
    for (const auto& item : view.keys) {
      if (item.id == id) return item;
    }
    static const contracts::PadKeyView none;
    return none;
  }
  /** 设置当前能读到的手柄并读一次。 */
  void connect(std::vector<GamepadSample> pads) {
    reader.samples = std::move(pads);
    controller.poll();
  }
};

TEST(GamepadSettingsController, LoadsBothPlayersWithTenKeysAndTheirKeyboardBindings) {
  PadFixture f;
  EXPECT_EQ(f.controller.view().load, PadSettingsLoad::Loading);
  EXPECT_TRUE(f.controller.view().players.empty());
  f.attach();
  const auto view = f.controller.view();
  EXPECT_EQ(view.load, PadSettingsLoad::Ready);
  ASSERT_EQ(view.players.size(), 2U);
  const auto& first = view.players[0];
  EXPECT_EQ(first.source, PadInputSource::Keyboard);
  ASSERT_EQ(first.keys.size(), 10U);
  EXPECT_EQ(first.keys[0].id, "up");
  EXPECT_EQ(first.keys[0].role, "上");
  EXPECT_EQ(first.keys[0].current, "↑");
  EXPECT_EQ(PadFixture::key(first, "select").current, "右 Shift");
  EXPECT_EQ(PadFixture::key(first, "turboA").current, "未绑定");
  EXPECT_TRUE(PadFixture::key(first, "turboA").unbound);
  EXPECT_EQ(PadFixture::key(first, "a").tip, "绑定键盘按键：A（当前：X）");
  EXPECT_TRUE(first.keysEnabled);
  EXPECT_FALSE(first.showDevice);
  EXPECT_EQ(first.message, "点手柄上的键，再按下键盘上的键来设置。");
  EXPECT_FALSE(first.canClear);
  EXPECT_EQ(first.clearLabel, "清除绑定");
  EXPECT_EQ(view.activePlayer, 0);
  EXPECT_TRUE(view.players[1].keys[0].unbound);
}

TEST(GamepadSettingsController, ReadFailureShowsTheCoreMessageAndRetriesWhenTheCoreIsReady) {
  PadFixture f;
  f.core.replies["input.getBindings"] = [](const json&) { return CoreReply::failure("core.unavailable", "核心未启动"); };
  f.attach();
  EXPECT_EQ(f.controller.view().load, PadSettingsLoad::Failed);
  EXPECT_EQ(f.controller.view().message, "核心未启动");
  f.reply("input.getBindings", {{"slots", defaultSlots()}});
  f.core.setState(contracts::CoreState::Running);
  EXPECT_EQ(f.controller.view().load, PadSettingsLoad::Ready);
}

TEST(GamepadSettingsController, ClickingAKeyStartsLearningAndClickingItAgainCancels) {
  PadFixture f;
  f.attach();
  f.controller.pickKey(0, "up");
  auto view = f.player(0);
  EXPECT_TRUE(PadFixture::key(view, "up").learning);
  EXPECT_TRUE(PadFixture::key(view, "up").selected);
  EXPECT_EQ(PadFixture::key(view, "up").current, "请按键");
  EXPECT_FALSE(PadFixture::key(view, "up").unbound);
  EXPECT_EQ(PadFixture::key(view, "up").tip, "请按下键盘上的键（Esc 取消）");
  EXPECT_EQ(view.message, "设置“上”：请按下键盘上的键（Esc 取消）");
  EXPECT_TRUE(f.controller.learningKey());
  f.controller.pickKey(0, "up");
  view = f.player(0);
  EXPECT_FALSE(f.controller.learningKey());
  EXPECT_FALSE(PadFixture::key(view, "up").learning);
  // 取消后键保持选中，说明行显示当前绑定，“清除”可用
  EXPECT_TRUE(PadFixture::key(view, "up").selected);
  EXPECT_EQ(view.message, "上：↑");
  EXPECT_TRUE(view.canClear);
  EXPECT_EQ(view.clearLabel, "清除键盘按键：上");
  // 点另一个键改为学习那个键
  f.controller.pickKey(0, "a");
  f.controller.pickKey(0, "b");
  EXPECT_TRUE(PadFixture::key(f.player(0), "b").learning);
  EXPECT_FALSE(PadFixture::key(f.player(0), "a").learning);
}

TEST(GamepadSettingsController, LearnedKeyIsSavedAndFreesItFromTheOtherButton) {
  PadFixture f;
  f.attach();
  f.controller.pickKey(1, "a");
  f.controller.selectPlayer(1);
  // 切换玩家页签取消了学习；重新开始
  EXPECT_FALSE(f.controller.learningKey());
  f.controller.pickKey(1, "a");
  EXPECT_TRUE(f.controller.keyPressed("KeyZ"));
  ASSERT_NE(f.last("input.setBindings"), nullptr);
  const auto saved = *logic::parseSlots(f.last("input.setBindings")->params["slots"]);
  EXPECT_EQ(logic::keyOf(saved[1], "a"), "KeyZ");
  EXPECT_FALSE(logic::keyOf(saved[0], "b").has_value());
  EXPECT_FALSE(f.controller.learningKey());
  EXPECT_EQ(PadFixture::key(f.player(1), "a").current, "Z");
  EXPECT_TRUE(PadFixture::key(f.player(0), "b").unbound);
  EXPECT_TRUE(f.shell.notices.empty());
}

TEST(GamepadSettingsController, EscapeCancelsAndKeysThatCannotBeBoundAreSwallowedWithoutSaving) {
  PadFixture f;
  f.attach();
  // 没有在学习时不吞按键
  EXPECT_FALSE(f.controller.keyPressed("KeyZ"));
  f.controller.pickKey(0, "up");
  EXPECT_TRUE(f.controller.keyPressed("F-key"));
  EXPECT_TRUE(f.controller.learningKey());
  EXPECT_EQ(f.last("input.setBindings"), nullptr);
  EXPECT_TRUE(f.controller.keyPressed("Escape"));
  EXPECT_FALSE(f.controller.learningKey());
  EXPECT_EQ(f.last("input.setBindings"), nullptr);
}

TEST(GamepadSettingsController, ClearRemovesTheSelectedKeysBinding) {
  PadFixture f;
  f.attach();
  f.controller.pickKey(0, "up");
  f.controller.cancelLearning();
  f.controller.clearSelected(0);
  const auto saved = *logic::parseSlots(f.last("input.setBindings")->params["slots"]);
  EXPECT_FALSE(logic::keyOf(saved[0], "up").has_value());
  EXPECT_EQ(logic::keyOf(saved[0], "down"), "ArrowDown");
  EXPECT_TRUE(PadFixture::key(f.player(0), "up").unbound);
  EXPECT_FALSE(f.player(0).canClear);
}

TEST(GamepadSettingsController, SaveFailureNotifiesTheReasonAndKeepsTheOldBindings) {
  PadFixture f;
  f.attach();
  f.core.replies["input.setBindings"] = [](const json&) { return CoreReply::failure("input.invalidBindings", "同一个键盘按键只能绑给一个手柄键"); };
  f.controller.pickKey(0, "up");
  EXPECT_TRUE(f.controller.keyPressed("KeyW"));
  ASSERT_EQ(f.shell.notices.size(), 1U);
  EXPECT_EQ(f.shell.notices[0].first, "同一个键盘按键只能绑给一个手柄键");
  EXPECT_EQ(f.shell.notices[0].second, contracts::NotificationLevel::Error);
  EXPECT_EQ(PadFixture::key(f.player(0), "up").current, "↑");
}

TEST(GamepadSettingsController, GamepadSourceNeedsADeviceAndExplainsWhy) {
  PadFixture f;
  f.attach();
  f.controller.setSource(0, PadInputSource::Gamepad);
  auto view = f.player(0);
  EXPECT_TRUE(view.showDevice);
  EXPECT_FALSE(view.keysEnabled);
  EXPECT_EQ(view.message, "先在上面选择要使用的手柄。");
  EXPECT_EQ(PadFixture::key(view, "up").current, "未绑定");
  // 有手柄但还没指定
  f.connect({pad(0, "Xbox")});
  view = f.player(0);
  ASSERT_EQ(view.devices.size(), 1U);
  EXPECT_EQ(view.devices[0].value, "0:Xbox");
  EXPECT_EQ(view.devices[0].label, "Xbox");
  EXPECT_EQ(view.assignedDevice, "");
  // 指定后键可用，但手柄还没接上时给出提示
  f.controller.assignDevice(0, "0:Xbox");
  EXPECT_TRUE(f.player(0).keysEnabled);
  EXPECT_EQ(f.player(0).assignedDevice, "0:Xbox");
  f.connect({});
  view = f.player(0);
  EXPECT_FALSE(view.keysEnabled);
  EXPECT_EQ(view.message, "这个手柄现在没有连接，接上后要按一下它的任意按钮。");
  ASSERT_EQ(view.devices.size(), 1U);
  EXPECT_EQ(view.devices[0].label, "Xbox（未连接）");
  // 点不了键：不开始学习
  f.controller.pickKey(0, "a");
  EXPECT_FALSE(PadFixture::key(f.player(0), "a").learning);
}

TEST(GamepadSettingsController, AssigningADeviceSavesItAndReleasesItFromTheOtherPlayer) {
  PadFixture f;
  f.attach();
  f.connect({pad(0, "Xbox")});
  f.controller.assignDevice(0, "0:Xbox");
  EXPECT_TRUE(logic::parseSlots(f.last("input.setBindings")->params["slots"])->at(0).gamepad.has_value());
  f.controller.assignDevice(1, "0:Xbox");
  const auto saved = *logic::parseSlots(f.last("input.setBindings")->params["slots"]);
  EXPECT_FALSE(saved[0].gamepad.has_value());
  EXPECT_TRUE(saved[1].gamepad.has_value());
  f.controller.assignDevice(1, "");
  EXPECT_FALSE(logic::parseSlots(f.last("input.setBindings")->params["slots"])->at(1).gamepad.has_value());
}

TEST(GamepadSettingsController, LearningAGamepadButtonUsesTheBaselineAndPollsEveryFrame) {
  PadFixture f;
  f.attach();
  f.connect({pad(0, "Xbox", {false, true, false}, {0.0, 0.0})});
  f.controller.setSource(0, PadInputSource::Gamepad);
  f.controller.assignDevice(0, "0:Xbox");
  EXPECT_EQ(f.controller.pollIntervalMs(), 500U);
  f.controller.pickKey(0, "a");
  EXPECT_EQ(f.controller.pollIntervalMs(), 16U);
  EXPECT_EQ(f.player(0).message, "设置“A”：请按下手柄上的按钮或推动摇杆（再点一次取消）");
  // 第一次读取只取基准，开始前就按着的按钮不算
  f.controller.poll();
  f.controller.poll();
  EXPECT_TRUE(PadFixture::key(f.player(0), "a").learning);
  f.connect({pad(0, "Xbox", {false, true, true}, {0.0, 0.0})});
  EXPECT_FALSE(PadFixture::key(f.player(0), "a").learning);
  const auto saved = *logic::parseSlots(f.last("input.setBindings")->params["slots"]);
  const auto binding = logic::bindingOf(saved[0].gamepad, "a");
  ASSERT_TRUE(binding.has_value());
  EXPECT_EQ(binding->kind, "button");
  EXPECT_EQ(binding->index, 2);
  EXPECT_EQ(PadFixture::key(f.player(0), "a").current, "按钮 2");
  EXPECT_EQ(f.controller.pollIntervalMs(), 500U);
  // 选中的键保持选中，可以清除
  EXPECT_TRUE(f.player(0).canClear);
  EXPECT_EQ(f.player(0).clearLabel, "清除手柄按钮：A");
  f.controller.clearSelected(0);
  EXPECT_TRUE(PadFixture::key(f.player(0), "a").unbound);
}

TEST(GamepadSettingsController, LearningAnAxisBindsItsDirection) {
  PadFixture f;
  f.attach();
  f.connect({pad(0, "Xbox", {false}, {0.0, 0.0})});
  f.controller.setSource(1, PadInputSource::Gamepad);
  f.controller.assignDevice(1, "0:Xbox");
  f.controller.pickKey(1, "left");
  f.controller.poll();
  f.connect({pad(0, "Xbox", {false}, {-1.0, 0.0})});
  const auto saved = *logic::parseSlots(f.last("input.setBindings")->params["slots"]);
  const auto binding = logic::bindingOf(saved[1].gamepad, "left");
  ASSERT_TRUE(binding.has_value());
  EXPECT_EQ(binding->kind, "axis");
  EXPECT_EQ(binding->direction, "negative");
  EXPECT_EQ(PadFixture::key(f.player(1), "left").current, "轴 0 负向");
}

TEST(GamepadSettingsController, ChangingSourceOrPlayerOrResettingCancelsLearningWithoutTouchingData) {
  PadFixture f;
  f.attach();
  f.controller.pickKey(0, "up");
  f.controller.setSource(0, PadInputSource::Gamepad);
  EXPECT_FALSE(f.controller.learningKey());
  EXPECT_FALSE(PadFixture::key(f.player(0), "up").selected);
  f.controller.setSource(0, PadInputSource::Keyboard);
  // 切换来源不改数据：键盘绑定仍在
  EXPECT_EQ(PadFixture::key(f.player(0), "up").current, "↑");
  f.controller.pickKey(0, "up");
  f.controller.selectPlayer(1);
  EXPECT_FALSE(f.controller.learningKey());
  EXPECT_EQ(f.controller.view().activePlayer, 1);
  f.controller.pickKey(1, "a");
  f.controller.reset();
  EXPECT_FALSE(f.controller.learningKey());
  EXPECT_NE(f.last("input.resetBindings"), nullptr);
  EXPECT_EQ(f.core.calls.back().name, "input.resetBindings");
}

TEST(GamepadSettingsController, ResetFailureNotifies) {
  PadFixture f;
  f.attach();
  f.core.replies["input.resetBindings"] = [](const json&) { return CoreReply::failure("input.failed", "无法恢复"); };
  f.controller.reset();
  ASSERT_EQ(f.shell.notices.size(), 1U);
  EXPECT_EQ(f.shell.notices[0].first, "无法恢复");
}

TEST(GamepadSettingsController, BindingsChangedEventUpdatesAndCoreRestartReloads) {
  PadFixture f;
  f.attach();
  auto changed = slotsOf(defaultSlots());
  changed = logic::bindKey(changed, 1, "a", "KeyQ");
  f.core.emitEvent("input.bindingsChanged", {{"slots", logic::slotsToJson(changed)}});
  EXPECT_EQ(PadFixture::key(f.player(1), "a").current, "Q");
  const int before = f.changes;
  f.core.emitEvent("input.bindingsChanged", nullptr);
  f.core.emitEvent("runtime.stateChanged", {{"slots", json::array()}});
  EXPECT_EQ(f.changes, before);
  f.core.setState(contracts::CoreState::Starting);
  EXPECT_EQ(f.controller.view().load, PadSettingsLoad::Loading);
  EXPECT_TRUE(f.controller.view().players.empty());
  f.core.setState(contracts::CoreState::Running);
  EXPECT_EQ(f.controller.view().load, PadSettingsLoad::Ready);
  EXPECT_EQ(PadFixture::key(f.player(1), "a").current, "未绑定");
}

TEST(GamepadSettingsController, DetachStopsEverything) {
  PadFixture f;
  f.attach();
  f.detach();
  EXPECT_EQ(f.core.subscriberCount(), 0U);
  f.core.calls.clear();
  f.controller.pickKey(0, "up");
  EXPECT_TRUE(f.controller.keyPressed("KeyW"));
  EXPECT_EQ(f.last("input.setBindings"), nullptr);
}

TEST(InputModule, RegistersTheDialogTheCommandWithoutShortcutAndTheMenuItemWithASeparator) {
  test::FakeCoreGateway core;
  test::FakeBroker broker{core};
  FakeGamepadReader reader;
  logic::GamepadSettingsController controller(broker, core, reader);
  const auto module = logic::createInputModule(controller);
  EXPECT_EQ(module.name, "input");
  ASSERT_EQ(module.dialogs.size(), 1U);
  EXPECT_EQ(module.dialogs[0].id, "input.gamepadSettings");
  EXPECT_EQ(module.dialogs[0].title, "手柄设置");
  EXPECT_EQ(module.dialogs[0].width, 0);
  EXPECT_EQ(module.dialogs[0].height, 0);
  EXPECT_TRUE(module.dialogs[0].fitMinWidth);
  EXPECT_TRUE(module.dialogs[0].fitMinHeight);
  ASSERT_EQ(module.commands.size(), 1U);
  EXPECT_EQ(module.commands[0].id, "input.openGamepadSettings");
  EXPECT_TRUE(module.commands[0].defaultShortcut.empty());
  test::FakeShellApi shell;
  module.commands[0].run(shell);
  EXPECT_EQ(shell.dialogs, (std::vector<std::string>{"input.gamepadSettings"}));
  ASSERT_EQ(module.menuItems.size(), 1U);
  EXPECT_EQ(module.menuItems[0].menu, "settings");
  EXPECT_TRUE(module.menuItems[0].separatorBefore);
  const auto stop = module.attach(shell);
  EXPECT_EQ(core.subscriberCount(), 2U);
  stop();
  EXPECT_EQ(core.subscriberCount(), 0U);
}

}  // namespace
}  // namespace ember::win32
