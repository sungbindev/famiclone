/*
 * ------------------------------------------------------------------------
 * 名称：xinput-gamepad-reader.cpp
 * 说明：XInput 手柄读取端口的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-20
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "infra/input/xinput-gamepad-reader.hpp"

#include "platform/input/xinput-gamepads.hpp"

namespace ember::win32::infra {

XInputGamepadReader::XInputGamepadReader() : gamepads_(std::make_unique<platform::XInputGamepads>()) {}

XInputGamepadReader::~XInputGamepadReader() = default;

std::vector<contracts::GamepadSample> XInputGamepadReader::read() {
  std::vector<contracts::GamepadSample> samples;
  for (auto& reading : gamepads_->read()) {
    contracts::GamepadSample sample;
    sample.index = reading.index;
    sample.id = std::move(reading.id);
    sample.connected = true;
    sample.buttons = std::move(reading.buttons);
    sample.axes = std::move(reading.axes);
    samples.push_back(std::move(sample));
  }
  return samples;
}

}  // namespace ember::win32::infra
