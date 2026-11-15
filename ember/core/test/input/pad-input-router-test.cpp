/*
 * ------------------------------------------------------------------------
 * 名称：pad-input-router-test.cpp
 * 说明：手柄输入路由器的测试：键盘当手柄、键盘属于机器时不截、实体手柄的按钮与轴、同名设备的第几个、连发按帧翻转、换主机后重新设置。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：测试用主机把其余功能转给一台真的 FcMachine，只记录手柄口的设置并可以改变“键盘是否属于机器”。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "bootstrap/machines/fc.hpp"
#include "contracts/hardware/button-input.hpp"
#include "infra/input/input-binding-registry.hpp"
#include "logic/input/pad-input-router.hpp"

namespace {

namespace hw = ember::contracts::hardware;
namespace api = ember::contracts::input;
using ember::infra::input::InputBindingRegistry;
using ember::infra::input::InputBindingRegistryOptions;
using ember::logic::input::PadInputRouter;

/** 记录手柄口设置的主机：其余转给内部的 FcMachine。 */
class TestMachine final : public hw::Machine {
 public:
  void powerOn() override { inner.powerOn(); }
  void powerOff() override { inner.powerOff(); }
  void softReset() override { inner.softReset(); }
  [[nodiscard]] bool isPoweredOn() const override { return inner.isPoweredOn(); }
  void run(hw::MasterTime ticks) override { inner.run(ticks); }
  void runFrame() override { inner.runFrame(); }
  [[nodiscard]] hw::FrameBuffer frame() const override { return inner.frame(); }
  [[nodiscard]] unsigned audioSampleRate() const override { return inner.audioSampleRate(); }
  void takeAudio(std::vector<float>& samples) override { inner.takeAudio(samples); }
  void input(const hw::InputEvent& event) override { inner.input(event); }
  [[nodiscard]] bool keyboardIsMachineInput() const override { return keyboardOwned; }
  [[nodiscard]] hw::CpuRegisters cpuRegisters() const override { return inner.cpuRegisters(); }
  [[nodiscard]] const hw::Clock& clock() const override { return inner.clock(); }
  hw::CartridgeSlot& cartridgeSlot() override { return inner.cartridgeSlot(); }
  hw::IrqOutput& cartridgeIrq() override { return inner.cartridgeIrq(); }
  [[nodiscard]] unsigned padPortCount() const override { return 2; }
  void setButtons(unsigned port, std::uint8_t buttons) override {
    masks[port] = buttons;
    ++sets;
  }

  ember::bootstrap::FcMachine inner;
  bool keyboardOwned{false};
  std::uint8_t masks[2] = {0, 0};
  int sets{0};
};

/** 路由器、绑定服务与测试主机。 */
struct Rig {
  Rig() : registry(std::make_shared<InputBindingRegistry>(InputBindingRegistryOptions{})), router(registry) {}

  bool key(const char* code, bool down) { return router.handle(hw::KeyInput{code, down}, machine); }

  bool pad(std::uint8_t index, const std::string& id, std::vector<bool> buttons, std::vector<double> axes = {}, bool connected = true) {
    hw::GamepadInput event;
    event.index = index;
    event.id = id;
    event.connected = connected;
    event.buttons = std::move(buttons);
    event.axes = std::move(axes);
    return router.handle(event, machine);
  }

  void bindPad(std::size_t slot, const std::string& id, std::int64_t occurrence, std::vector<api::PadGamepadBinding> bindings) {
    auto slots = *registry->bindings();
    slots[slot].gamepad = api::PadGamepadAssignment{id, occurrence, std::move(bindings)};
    registry->set(slots);
  }

  std::shared_ptr<InputBindingRegistry> registry;
  PadInputRouter router;
  TestMachine machine;
};

TEST(PadInputRouter, ABoundKeyBecomesAPadButtonWhenTheKeyboardIsNotTheMachines) {
  Rig rig;
  EXPECT_TRUE(rig.key("KeyX", true)) << "默认 X 是玩家 1 的 A，按下被截下";
  EXPECT_EQ(rig.machine.masks[0], hw::pad_button::A);
  EXPECT_TRUE(rig.key("ArrowUp", true));
  EXPECT_EQ(rig.machine.masks[0], hw::pad_button::A | hw::pad_button::UP);
  EXPECT_FALSE(rig.key("KeyX", false)) << "松开总是同时交给机器，不让机器那边留下按着的键";
  EXPECT_EQ(rig.machine.masks[0], hw::pad_button::UP);
  rig.key("ArrowUp", false);
  EXPECT_EQ(rig.machine.masks[0], 0);
  EXPECT_EQ(rig.machine.masks[1], 0);
}

TEST(PadInputRouter, KeysThatAreNotBoundAreLeftToTheMachine) {
  Rig rig;
  EXPECT_FALSE(rig.key("KeyQ", true));
  EXPECT_EQ(rig.machine.masks[0], 0);
}

TEST(PadInputRouter, TheKeyboardBelongingToTheMachineIsNeverTurnedIntoPadButtons) {
  Rig rig;
  rig.machine.keyboardOwned = true;
  EXPECT_FALSE(rig.key("KeyX", true));
  EXPECT_EQ(rig.machine.masks[0], 0);
  // 按着键时机器切换成键盘不属于机器（学习机进入 FC 兼容模式）：下一帧起按这个键算手柄键。
  rig.machine.keyboardOwned = false;
  rig.router.frameCompleted(rig.machine);
  EXPECT_EQ(rig.machine.masks[0], hw::pad_button::A);
}

TEST(PadInputRouter, GamepadButtonsAndAxesFollowTheAssignedDevice) {
  Rig rig;
  rig.bindPad(1, "Demo Pad", 0, {{"a", "button", 1, std::nullopt}, {"left", "axis", 0, std::string("negative")}, {"down", "axis", 1, std::string("positive")}});
  EXPECT_TRUE(rig.pad(3, "Demo Pad", {false, true}, {-0.8, 0.2})) << "手柄事件全部由路由器截下";
  EXPECT_EQ(rig.machine.masks[1], hw::pad_button::A | hw::pad_button::LEFT);
  rig.pad(3, "Demo Pad", {false, false}, {-0.3, 0.9});
  EXPECT_EQ(rig.machine.masks[1], hw::pad_button::DOWN) << "轴没过 0.5 不算按下";
  rig.pad(4, "Other Pad", {true, true}, {-1, 1});
  EXPECT_EQ(rig.machine.masks[1], hw::pad_button::DOWN) << "别的设备不影响这个槽位";
  rig.pad(3, "Demo Pad", {}, {}, false);
  EXPECT_EQ(rig.machine.masks[1], 0) << "拔掉之后按键松开";
}

TEST(PadInputRouter, TheOccurrenceSelectsWhichOfSeveralIdenticalDevices) {
  Rig rig;
  rig.bindPad(0, "Twin", 1, {{"start", "button", 0, std::nullopt}});
  rig.pad(2, "Twin", {true});
  EXPECT_EQ(rig.machine.masks[0], 0) << "现在只有 2 号一个，它是第 0 个，第 1 个还不存在";
  rig.pad(5, "Twin", {false});
  EXPECT_EQ(rig.machine.masks[0], 0) << "编号 5 是第 1 个，还没有按键";
  rig.pad(5, "Twin", {true});
  EXPECT_EQ(rig.machine.masks[0], hw::pad_button::START);
  rig.pad(2, "Twin", {}, {}, false);
  EXPECT_EQ(rig.machine.masks[0], 0) << "2 号拔掉后 5 号成了第 0 个，第 1 个不存在";
}

TEST(PadInputRouter, TurboTogglesTheAAndBBitsEveryTwoFrames) {
  Rig rig;
  rig.key("KeyV", true);
  std::vector<std::uint8_t> seen;
  for (int frame = 0; frame < 8; ++frame) {
    seen.push_back(rig.machine.masks[0]);
    rig.router.frameCompleted(rig.machine);
  }
  const std::uint8_t on = hw::pad_button::A;
  EXPECT_EQ(seen, (std::vector<std::uint8_t>{on, on, 0, 0, on, on, 0, 0})) << "默认 V 是玩家 1 的连发 A";
  rig.key("KeyV", false);
  EXPECT_EQ(rig.machine.masks[0], 0);
  rig.key("KeyC", true);
  EXPECT_EQ(rig.machine.masks[0] & hw::pad_button::B, hw::pad_button::B) << "连发 B";
}

TEST(PadInputRouter, OnlyChangesAreSentToTheMachineAndAMachineChangeSendsThemAgain) {
  Rig rig;
  rig.key("KeyX", true);
  const int afterPress = rig.machine.sets;
  rig.router.frameCompleted(rig.machine);
  rig.router.frameCompleted(rig.machine);
  EXPECT_EQ(rig.machine.sets, afterPress) << "掩码没有变化就不再设置";
  rig.router.machineChanged();
  rig.router.frameCompleted(rig.machine);
  EXPECT_GT(rig.machine.sets, afterPress) << "换了主机之后重新设置";
  EXPECT_EQ(rig.machine.masks[0], hw::pad_button::A);
}

TEST(PadInputRouter, MouseEventsAreLeftToTheMachine) {
  Rig rig;
  EXPECT_FALSE(rig.router.handle(hw::MouseInput{}, rig.machine));
}

}  // namespace
