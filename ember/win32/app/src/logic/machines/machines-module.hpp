/*
 * ------------------------------------------------------------------------
 * 名称：machines-module.hpp
 * 说明：机型模块在外壳里的登记项：“机型与 BIOS”对话框、打开它的命令与“设置”菜单里的菜单项，加上常驻部分（读取机型清单并保持最新）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：对话框是独立的系统窗口，同一时间只开一个；控制器由组装根创建并传入，对话框内容控件与登记项共用；规则见 docs/05-界面.md 第 4.7 节。
 * ------------------------------------------------------------------------
 */

#pragma once

#include "contracts/shell/shell-registry.hpp"
#include "logic/machines/bios-controller.hpp"

namespace ember::win32::logic {

/** “机型与 BIOS”对话框的标识。 */
inline constexpr const char* kMachinesDialogId = "machines.bios";
/** 打开“机型与 BIOS”对话框的命令。 */
inline constexpr const char* kMachinesOpenCommand = "machines.open";

/** 创建机型模块的登记项；控制器必须比登记表活得久。 */
[[nodiscard]] contracts::ShellModule createMachinesModule(BiosController& controller);

}  // namespace ember::win32::logic
