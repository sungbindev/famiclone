/*
 * ------------------------------------------------------------------------
 * 名称：input-controller.hpp
 * 说明：输入控制器：把键盘、鼠标与游戏手柄的原始事件转成核心的输入消息，跟踪按着的键、鼠标的位置与每个手柄上次上报的状态。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：对应 Electron 外壳的 key-capture.ts、pointer-capture.ts 与 gamepad-capture.ts；消息的格式（type、code、down、kind、x、y、dx、dy、buttons、wheel、index、id、connected、buttons、axes）与上限同核心宿主的校验。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <map>
#include <optional>
#include <set>
#include <string>

#include "contracts/core-gateway/core-gateway.hpp"
#include "contracts/input/screen-input.hpp"

namespace ember::win32::logic {

/** 输入控制器。 */
class InputController final : public contracts::ScreenInput {
 public:
  /** @param core 核心网关，输入消息经它的 sendInput 交给核心。 */
  explicit InputController(contracts::CoreGateway& core) : core_(core) {}

  void nativeKey(const std::string& code, bool down) override;
  void releaseAllKeys() override;
  void mouseAt(contracts::MouseKind kind, const shared::FramePoint& point, int buttons, double wheel) override;
  void mouseMoveBy(contracts::MouseKind kind, double dx, double dy, int buttons, double wheel) override;
  void mouseLeave(int buttons) override;
  void pollGamepads(const std::vector<contracts::GamepadSample>& pads) override;
  void disconnectGamepads() override;

 private:
  void sendMouse(contracts::MouseKind kind, double x, double y, double dx, double dy, int buttons, double wheel);
  void reportGamepad(const contracts::GamepadSample& pad);
  void disconnectGamepad(int index);

  contracts::CoreGateway& core_;
  std::set<std::string> held_;
  /** 鼠标上一次的位置；离开后清空。 */
  std::optional<shared::FramePoint> last_;
  /** 每个手柄上一次上报的内容（序列化成文字，便于比较）与设备标识。 */
  std::map<int, std::string> reported_;
  std::map<int, std::string> ids_;
};

}  // namespace ember::win32::logic
