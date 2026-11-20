/*
 * ------------------------------------------------------------------------
 * 名称：gamepads.cpp
 * 说明：游戏手柄合并读取的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-20
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "platform/input/gamepads.hpp"

#include <iterator>

namespace ember::win32::platform {

std::vector<GamepadReading> Gamepads::read() {
  std::vector<GamepadReading> pads = xinput_.read();
  std::vector<GamepadReading> others = joystick_.read();
  pads.insert(pads.end(), std::make_move_iterator(others.begin()), std::make_move_iterator(others.end()));
  return pads;
}

}  // namespace ember::win32::platform
