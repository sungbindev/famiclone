/*
 * ------------------------------------------------------------------------
 * 名称：system-gamepad-reader.cpp
 * 说明：系统手柄读取端口的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-20
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "infra/input/system-gamepad-reader.hpp"

#include "platform/input/gamepads.hpp"

namespace ember::win32::infra {

SystemGamepadReader::SystemGamepadReader() : gamepads_(std::make_unique<platform::Gamepads>()) {}

SystemGamepadReader::~SystemGamepadReader() = default;

std::vector<contracts::GamepadSample> SystemGamepadReader::read() {
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
