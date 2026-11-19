/*
 * ------------------------------------------------------------------------
 * 名称：mappers-module.hpp
 * 说明：mapper 插件模块在外壳里的登记项：“mapper 插件”对话框、打开它的命令与“设置”菜单里的菜单项，加上常驻部分（读取插件列表并保持最新）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：对话框是独立的系统窗口，同一时间只开一个；控制器由组装根创建并传入，对话框内容控件与登记项共用；规则见 docs/05-界面.md 第 4.4 节。
 * ------------------------------------------------------------------------
 */

#pragma once

#include "contracts/shell/shell-registry.hpp"
#include "logic/mappers/mappers-controller.hpp"

namespace ember::win32::logic {

/** “mapper 插件”对话框的标识。 */
inline constexpr const char* kMappersDialogId = "mappers.plugins";
/** 打开 mapper 插件对话框的命令。 */
inline constexpr const char* kMappersOpenCommand = "mappers.open";

/** 创建 mapper 插件模块的登记项；控制器必须比登记表活得久。 */
[[nodiscard]] contracts::ShellModule createMappersModule(MappersController& controller);

}  // namespace ember::win32::logic
