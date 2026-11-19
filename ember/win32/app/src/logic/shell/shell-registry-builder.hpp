/*
 * ------------------------------------------------------------------------
 * 名称：shell-registry-builder.hpp
 * 说明：汇总外壳自己与各模块的登记项并检查：模块名合格且不重复、标识以“模块名.”开头且不重复、菜单项是命令或子菜单二选一、引用的命令存在、
 *       显示区至多一个、文件处理项的扩展名合格且不重复、默认快捷键写法合格且不与保留组合及其他命令冲突。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：对应 Electron 外壳的 src/ui/shell/build-registry.ts；登记有误属于程序错误，直接抛出异常并列出全部问题。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "contracts/shell/shell-registry.hpp"

namespace ember::win32::logic {

/** 外壳自己的登记项使用的名称。 */
inline constexpr const char* kShellNamespace = "shell";

/** 汇总后的登记表。 */
struct ShellRegistry {
  /** 提供显示区的模块名；没有模块提供时为空。 */
  std::string screenOwner;
  std::map<std::string, contracts::ShellDialogDefinition> dialogs;
  std::map<std::string, contracts::ShellCommandDefinition> commands;
  /** 全部菜单项：外壳自己的排在各模块之后。 */
  std::vector<contracts::ShellMenuItemDefinition> menuItems;
  std::vector<contracts::ShellPanelDefinition> bottomPanels;
  std::vector<contracts::ShellStatusItemDefinition> statusItems;
  std::vector<contracts::ShellTitleBarItemDefinition> titleBarItems;
  std::vector<contracts::ShellFileHandlerDefinition> fileHandlers;
  /** 默认快捷键到命令标识。 */
  std::map<std::string, std::string> shortcuts;
  /** 常驻部分，按登记顺序。 */
  std::vector<std::pair<std::string, std::function<std::function<void()>(contracts::ShellApi&)>>> attachments;
};

/**
 * 汇总并检查登记表。
 * @param modules 全部登记项，外壳自己的排在最前面（名称为 shell），其余是各模块。
 * @returns 登记表。
 * @throws std::runtime_error 登记有误时，说明里列出全部问题。
 */
[[nodiscard]] ShellRegistry buildShellRegistry(std::vector<contracts::ShellModule> modules);

}  // namespace ember::win32::logic
