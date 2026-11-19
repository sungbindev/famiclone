/*
 * ------------------------------------------------------------------------
 * 名称：xinput-gamepads.cpp
 * 说明：XInput 手柄读取的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：不包含 xinput.h：只用到的结构与常量在这里按 XInput 的定义写出，函数用 GetProcAddress 取；没有手柄的位置，XInputGetState 在没有连接时会比较慢，所以没有任何手柄时隔 60 次才再查。
 * ------------------------------------------------------------------------
 */

#include "platform/input/xinput-gamepads.hpp"

#include <windows.h>

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace ember::win32::platform {

namespace {

/** XInput 的按键位（XINPUT_GAMEPAD_*）。 */
constexpr WORD kDpadUp = 0x0001;
constexpr WORD kDpadDown = 0x0002;
constexpr WORD kDpadLeft = 0x0004;
constexpr WORD kDpadRight = 0x0008;
constexpr WORD kStart = 0x0010;
constexpr WORD kBack = 0x0020;
constexpr WORD kLeftThumb = 0x0040;
constexpr WORD kRightThumb = 0x0080;
constexpr WORD kLeftShoulder = 0x0100;
constexpr WORD kRightShoulder = 0x0200;
constexpr WORD kButtonA = 0x1000;
constexpr WORD kButtonB = 0x2000;
constexpr WORD kButtonX = 0x4000;
constexpr WORD kButtonY = 0x8000;

/** XInput 推荐的死区与扳机的按下门限。 */
constexpr int kLeftDeadzone = 7849;
constexpr int kRightDeadzone = 8689;
constexpr int kTriggerThreshold = 30;
constexpr int kMaxPads = 4;
constexpr unsigned kIdleRecheck = 60;

struct XInputGamepad {
  WORD wButtons;
  BYTE bLeftTrigger;
  BYTE bRightTrigger;
  SHORT sThumbLX;
  SHORT sThumbLY;
  SHORT sThumbRX;
  SHORT sThumbRY;
};

struct XInputState {
  DWORD dwPacketNumber;
  XInputGamepad Gamepad;
};

using XInputGetStateFn = DWORD(WINAPI*)(DWORD, XInputState*);

/** 摇杆的值转成 -1 到 1：死区内为 0；向上（Y 轴正方向）在标准映射里是负值。 */
double axisValue(int value, int deadzone, bool invert) {
  if (std::abs(value) <= deadzone) return 0.0;
  const double scaled = std::clamp(value / 32767.0, -1.0, 1.0);
  return invert ? -scaled : scaled;
}

}  // namespace

XInputGamepads::XInputGamepads() {
  // 取 Windows 7 自带的 9.1.0 版，不依赖需要另装的 xinput1_3.dll
  module_ = LoadLibraryW(L"xinput9_1_0.dll");
  if (module_ != nullptr) getState_ = reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(module_), "XInputGetState"));
}

XInputGamepads::~XInputGamepads() {
  if (module_ != nullptr) FreeLibrary(static_cast<HMODULE>(module_));
}

std::vector<GamepadReading> XInputGamepads::read() {
  std::vector<GamepadReading> pads;
  if (getState_ == nullptr) return pads;
  if (idlePolls_ > 0 && idlePolls_ < kIdleRecheck) {
    ++idlePolls_;
    return pads;
  }
  const auto getState = reinterpret_cast<XInputGetStateFn>(getState_);
  for (int index = 0; index < kMaxPads; ++index) {
    XInputState state{};
    if (getState(static_cast<DWORD>(index), &state) != ERROR_SUCCESS) continue;
    const XInputGamepad& pad = state.Gamepad;
    GamepadReading reading;
    reading.index = index;
    reading.id = "Xbox 360 Controller (XInput STANDARD GAMEPAD)";
    const auto held = [&pad](WORD bit) { return (pad.wButtons & bit) != 0; };
    reading.buttons = {held(kButtonA),       held(kButtonB),      held(kButtonX),         held(kButtonY),       held(kLeftShoulder), held(kRightShoulder),
                       pad.bLeftTrigger > kTriggerThreshold, pad.bRightTrigger > kTriggerThreshold, held(kBack), held(kStart), held(kLeftThumb), held(kRightThumb),
                       held(kDpadUp),        held(kDpadDown),     held(kDpadLeft),        held(kDpadRight),     false};
    reading.axes = {axisValue(pad.sThumbLX, kLeftDeadzone, false), axisValue(pad.sThumbLY, kLeftDeadzone, true), axisValue(pad.sThumbRX, kRightDeadzone, false), axisValue(pad.sThumbRY, kRightDeadzone, true)};
    pads.push_back(std::move(reading));
  }
  idlePolls_ = pads.empty() ? 1 : 0;
  return pads;
}

}  // namespace ember::win32::platform
