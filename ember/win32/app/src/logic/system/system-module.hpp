/*
 * ------------------------------------------------------------------------
 * 名称：system-module.hpp
 * 说明：系统模块在外壳里的登记项：“帮助”菜单的“帮助文档”“更新记录”“打开日志目录”“检查更新”“关于 Ember”、“关于 Ember”对话框、标题栏右侧的“更新”按钮，加上常驻部分（升级状态与版本的读取）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：对话框是独立的系统窗口，同一时间只开一个；控制器由组装根创建并传入；打开帮助文档与日志目录经 system.openHelp、system.openLogDirectory 两个外壳请求，失败用通知；规则见 docs/05-界面.md 第 5、8 节。
 * ------------------------------------------------------------------------
 */

#pragma once

#include "contracts/shell/request-broker.hpp"
#include "contracts/shell/shell-registry.hpp"
#include "logic/system/about-controller.hpp"
#include "logic/system/update-controller.hpp"

namespace ember::win32::logic {

/** “关于 Ember”对话框的标识。 */
inline constexpr const char* kAboutDialogId = "system.about";

/** 命令的标识。 */
inline constexpr const char* kSystemOpenHelpCommand = "system.openHelp";
inline constexpr const char* kSystemOpenChangelogCommand = "system.openChangelog";
inline constexpr const char* kSystemOpenLogDirectoryCommand = "system.openLogDirectory";
inline constexpr const char* kSystemCheckUpdateCommand = "system.checkUpdate";
inline constexpr const char* kSystemOpenAboutCommand = "system.openAbout";
/** 标题栏“更新”按钮的标识。 */
inline constexpr const char* kSystemUpdateTitleBarItem = "system.update";

/** 创建系统模块的登记项；各对象必须比登记表活得久。 */
[[nodiscard]] contracts::ShellModule createSystemModule(contracts::RequestBroker& broker, UpdateController& update, AboutController& about);

}  // namespace ember::win32::logic
