/*
 * ------------------------------------------------------------------------
 * 名称：debug-module.hpp
 * 说明：调试服务模块在外壳里的登记项：系统栏右侧、通知按钮左边的“调试模式”项（调试服务开启时才显示）和把状态控制器接到核心上的常驻部分。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：对应 Electron 外壳的 debug-module.tsx；没有面板、菜单与命令（调试服务的开关、端口与令牌在首选项里，见 docs/05-界面.md 第 4.5 节）；点击这一项执行命令 preferences.open（首选项在阶段 9 加入，
 *       之前命令不存在，点击什么都不发生）。
 * ------------------------------------------------------------------------
 */

#pragma once

#include "contracts/shell/shell-registry.hpp"
#include "logic/debug/debug-controller.hpp"

namespace ember::win32::logic {

/** 创建调试服务模块的登记项；控制器必须比登记表活得久。 */
[[nodiscard]] contracts::ShellModule createDebugModule(DebugController& controller);

}  // namespace ember::win32::logic
