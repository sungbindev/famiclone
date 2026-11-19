/*
 * ------------------------------------------------------------------------
 * 名称：preferences-module.hpp
 * 说明：首选项模块在外壳里的登记项：“首选项”对话框、打开它的命令（Ctrl+,）与“设置”菜单里的菜单项，加上常驻部分（读取首选项与机型清单并保持最新）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-20
 * 备注：对话框是独立的系统窗口，同一时间只开一个；控制器由组装根创建并传入，对话框内容控件与登记项共用；规则见 docs/05-界面.md 第 4.5 节。
 * ------------------------------------------------------------------------
 */

#pragma once

#include "contracts/shell/shell-registry.hpp"
#include "logic/preferences/bios-controller.hpp"
#include "logic/preferences/preferences-controller.hpp"
#include "logic/preferences/user-data-controller.hpp"

namespace ember::win32::logic {

/** “首选项”对话框的标识。 */
inline constexpr const char* kPreferencesDialogId = "preferences.preferences";
/** 打开首选项的命令。 */
inline constexpr const char* kPreferencesOpenCommand = "preferences.open";

/** 创建首选项模块的登记项；控制器必须比登记表活得久。 */
[[nodiscard]] contracts::ShellModule createPreferencesModule(PreferencesController& preferences, BiosController& bios, UserDataController& userData);

}  // namespace ember::win32::logic
