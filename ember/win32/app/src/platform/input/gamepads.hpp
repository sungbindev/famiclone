/*
 * ------------------------------------------------------------------------
 * 名称：gamepads.hpp
 * 说明：游戏手柄：把 XInput 手柄与通用游戏控制器（DirectInput 一类的 USB 手柄）合在一起读。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-20
 * 备注：XInput 手柄的位置是 0 到 3，通用手柄是 4 以后；同一个手柄不会被两边都读到（见 joystick-gamepads.hpp）。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <vector>

#include "platform/input/joystick-gamepads.hpp"
#include "platform/input/xinput-gamepads.hpp"

namespace ember::win32::platform {

/** 系统里所有能读到的游戏手柄。 */
class Gamepads {
 public:
  /** 读一轮：返回现在连着的手柄，没有时为空。 */
  [[nodiscard]] std::vector<GamepadReading> read();

 private:
  XInputGamepads xinput_;
  JoystickGamepads joystick_;
};

}  // namespace ember::win32::platform
