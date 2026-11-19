/*
 * ------------------------------------------------------------------------
 * 名称：debug-module.cpp
 * 说明：调试服务模块登记项的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "logic/debug/debug-module.hpp"

namespace ember::win32::logic {

namespace {

/** 点击后执行的命令：打开首选项。 */
constexpr const char* kOpenPreferencesCommand = "preferences.open";

}  // namespace

contracts::ShellModule createDebugModule(DebugController& controller) {
  contracts::ShellModule module;
  module.name = "debug";

  contracts::ShellStatusItemDefinition item;
  item.id = "debug.service";
  item.alignEnd = true;
  item.view = [&controller] { return debugItemView(controller.state()); };
  item.click = [](contracts::ShellApi& shell) { shell.runCommand(kOpenPreferencesCommand); };
  module.statusItems.push_back(std::move(item));

  module.attach = [&controller](contracts::ShellApi& shell) { return controller.attach(shell); };
  return module;
}

}  // namespace ember::win32::logic
