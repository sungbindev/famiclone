/*
 * ------------------------------------------------------------------------
 * 名称：runtime-module.hpp
 * 说明：运行模块在外壳里的登记项：显示区（画面）、底部面板的“运行”（系统栏左侧的运行按钮，图标实时显示运行状态）、命令与默认快捷键、“文件”与“机器”菜单、系统栏项（主机型号、卡带、mapper 插件）、拖入卡带的处理与常驻部分。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：登记项都绑定同一个控制器，由组装根创建并传入；规则见 docs/05-界面.md 第 3、4.1、5、6 节。
 *       “机器”菜单的第一项“选择机型”列出六个整机机型，已选的一项前面打对勾，点一项就记住它并直接开机；没有经典 FC（经典 FC 按卡带开机，打开卡带即选中）；整机机型程序里有实现且已添加 ROM 才可选，
 *       没有 ROM 的置灰并注明“未添加 ROM”，暂未支持的置灰并注明“暂未支持”；文件菜单的“最近打开”随最近打开的清单变化，没有记录时置灰。
 *       快捷键：打开卡带 Ctrl+O，开机或断电 F9，暂停或继续 F8，软复位 F7，硬复位 Ctrl+F7。
 * ------------------------------------------------------------------------
 */

#pragma once

#include "contracts/shell/shell-registry.hpp"
#include "logic/runtime/runtime-controller.hpp"

namespace ember::win32::logic {

/** 底部面板“运行”页签的标识。 */
inline constexpr const char* kRuntimePanelId = "runtime.panel";

/** 创建运行模块的登记项；控制器必须比登记表活得久。 */
[[nodiscard]] contracts::ShellModule createRuntimeModule(RuntimeController& controller);

}  // namespace ember::win32::logic
