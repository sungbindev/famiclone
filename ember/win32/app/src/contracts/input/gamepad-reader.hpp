/*
 * ------------------------------------------------------------------------
 * 名称：gamepad-reader.hpp
 * 说明：手柄读取端口：读一轮系统里现在连着的游戏手柄，手柄设置对话框经它发现手柄、学习手柄上的按钮或轴。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-20
 * 备注：对应 Electron 外壳里对话框自己的 navigator.getGamepads()；格式同画面区的手柄输入（GamepadSample：编号、设备标识、按钮、轴，按浏览器的标准映射排列）；实现在 infra/input。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <vector>

#include "contracts/input/screen-input.hpp"

namespace ember::win32::contracts {

/** 手柄读取端口。 */
class GamepadReader {
 public:
  virtual ~GamepadReader() = default;

  /** 读一轮：返回现在连着的手柄；没有手柄或读取不可用时为空。 */
  [[nodiscard]] virtual std::vector<GamepadSample> read() = 0;
};

}  // namespace ember::win32::contracts
