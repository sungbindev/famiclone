/*
 * ------------------------------------------------------------------------
 * 名称：shell-module.hpp
 * 说明：外壳自己的登记项：命令与菜单项（文件→退出、设置→窗口始终置顶）、全屏命令（F11，没有菜单项）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：外壳自己也走登记表，所以快捷键冲突、标识前缀都按同样的规则检查；底部面板没有快捷键与菜单，只经系统栏左侧的按钮展开。
 * ------------------------------------------------------------------------
 */

#pragma once

#include "contracts/shell/shell-registry.hpp"

namespace ember::win32::logic {

/** 外壳命令的标识。 */
inline constexpr const char* kShellQuitCommand = "shell.quit";
inline constexpr const char* kShellFullScreenCommand = "shell.toggleFullScreen";
inline constexpr const char* kShellAlwaysOnTopCommand = "shell.toggleAlwaysOnTop";

/** 外壳自己的登记项，排在各模块之前。 */
[[nodiscard]] contracts::ShellModule createShellModule();

}  // namespace ember::win32::logic
