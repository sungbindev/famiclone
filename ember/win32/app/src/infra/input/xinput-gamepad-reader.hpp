/*
 * ------------------------------------------------------------------------
 * 名称：xinput-gamepad-reader.hpp
 * 说明：手柄读取端口的实现：用 XInput 手柄读取（平台层的 XInputGamepads），转成手柄设置对话框用的手柄样本。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-20
 * 备注：与画面区读手柄的是同一个读取实现，但各用各的实例，互不影响；没有 XInput 库时读到的手柄为空。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <memory>
#include <vector>

#include "contracts/input/gamepad-reader.hpp"

namespace ember::win32::platform {
class XInputGamepads;
}  // namespace ember::win32::platform

namespace ember::win32::infra {

/** 用 XInput 读手柄。 */
class XInputGamepadReader final : public contracts::GamepadReader {
 public:
  XInputGamepadReader();
  ~XInputGamepadReader() override;
  XInputGamepadReader(const XInputGamepadReader&) = delete;
  XInputGamepadReader& operator=(const XInputGamepadReader&) = delete;

  [[nodiscard]] std::vector<contracts::GamepadSample> read() override;

 private:
  std::unique_ptr<platform::XInputGamepads> gamepads_;
};

}  // namespace ember::win32::infra
