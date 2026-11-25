/*
 * ------------------------------------------------------------------------
 * 名称：filemanager-module.hpp
 * 说明：媒体库模块在外壳里的登记项：“媒体库”对话框（非模态，停靠在主窗口左边）、开关它的命令（菜单项开着时打对号）与“文件”菜单里的菜单项，加上常驻部分（读取目录树并保持最新）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-25
 * 备注：对话框是独立的系统窗口，同一时间只开一个，打开期间主窗口仍可使用；控制器由组装根创建并传入，对话框内容控件与登记项共用；规则见 docs/05-界面.md 第 4.8 节。
 * ------------------------------------------------------------------------
 */

#pragma once

#include "contracts/shell/shell-registry.hpp"
#include "logic/filemanager/filemanager-controller.hpp"

namespace ember::win32::logic {

/** “媒体库”对话框的标识。 */
inline constexpr const char* kFileManagerDialogId = "filemanager.dialog";
/** 打开“媒体库”对话框的命令。 */
inline constexpr const char* kFileManagerOpenCommand = "filemanager.open";

/** 创建媒体库模块的登记项；控制器必须比登记表活得久。 */
[[nodiscard]] contracts::ShellModule createFileManagerModule(FileManagerController& controller);

}  // namespace ember::win32::logic
