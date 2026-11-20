/*
 * ------------------------------------------------------------------------
 * 名称：system-gamepad-reader.hpp
 * 说明：手柄读取端口的实现：读系统里的手柄（平台层的 Gamepads：XInput 手柄与通用 USB 手柄），转成手柄设置对话框用的手柄样本。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-20
 * 备注：与画面区读手柄的是同一个读取实现，但各用各的实例，互不影响；没有手柄时读到的为空。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <memory>
#include <vector>

#include "contracts/input/gamepad-reader.hpp"

namespace ember::win32::platform {
class Gamepads;
}  // namespace ember::win32::platform

namespace ember::win32::infra {

/** 读系统里的手柄。 */
class SystemGamepadReader final : public contracts::GamepadReader {
 public:
  SystemGamepadReader();
  ~SystemGamepadReader() override;
  SystemGamepadReader(const SystemGamepadReader&) = delete;
  SystemGamepadReader& operator=(const SystemGamepadReader&) = delete;

  [[nodiscard]] std::vector<contracts::GamepadSample> read() override;

 private:
  std::unique_ptr<platform::Gamepads> gamepads_;
};

}  // namespace ember::win32::infra
