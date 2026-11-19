/*
 * ------------------------------------------------------------------------
 * 名称：input-module.hpp
 * 说明：输入模块在外壳里的登记项：“手柄设置”对话框、打开它的命令与“设置”菜单里的菜单项（前面有分隔线），加上常驻部分（读取手柄绑定并保持最新）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-20
 * 备注：对应 Electron 外壳的 input-module.tsx；命令没有快捷键；对话框是独立的系统窗口，同一时间只开一个；控制器由组装根创建并传入，对话框内容控件与登记项共用；规则见 docs/05-界面.md 第 4.6 节。
 * ------------------------------------------------------------------------
 */

#pragma once

#include "contracts/shell/shell-registry.hpp"
#include "logic/input/gamepad-settings-controller.hpp"

namespace ember::win32::logic {

/** “手柄设置”对话框的标识。 */
inline constexpr const char* kGamepadSettingsDialogId = "input.gamepadSettings";
/** 打开手柄设置的命令。 */
inline constexpr const char* kGamepadSettingsOpenCommand = "input.openGamepadSettings";

/** 创建输入模块的登记项；控制器必须比登记表活得久。 */
[[nodiscard]] contracts::ShellModule createInputModule(GamepadSettingsController& controller);

}  // namespace ember::win32::logic
