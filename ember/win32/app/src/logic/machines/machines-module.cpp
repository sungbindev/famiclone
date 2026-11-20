/*
 * ------------------------------------------------------------------------
 * 名称：machines-module.cpp
 * 说明：机型模块登记项的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "logic/machines/machines-module.hpp"

namespace ember::win32::logic {

namespace {

/** 对话框内容区的初始高度与最小高度（逻辑像素）；宽度按内容算，打开时就用最小宽度。 */
constexpr int kDialogHeight = 520;
constexpr int kDialogMinHeight = 300;

constexpr const char* kTitle = "机型与 BIOS";

}  // namespace

contracts::ShellModule createMachinesModule(BiosController& controller) {
  contracts::ShellModule module;
  module.name = "machines";

  contracts::ShellDialogDefinition dialog;
  dialog.id = kMachinesDialogId;
  dialog.title = kTitle;
  dialog.height = kDialogHeight;
  dialog.fitMinWidth = true;
  dialog.minHeight = kDialogMinHeight;
  module.dialogs.push_back(std::move(dialog));

  contracts::ShellCommandDefinition open;
  open.id = kMachinesOpenCommand;
  open.title = [] { return std::string(kTitle); };
  open.run = [](contracts::ShellApi& shell) { shell.openDialog(kMachinesDialogId); };
  module.commands.push_back(std::move(open));

  contracts::ShellMenuItemDefinition item;
  item.menu = "settings";
  item.command = kMachinesOpenCommand;
  module.menuItems.push_back(std::move(item));

  module.attach = [&controller](contracts::ShellApi& shell) { return controller.attach(shell); };
  return module;
}

}  // namespace ember::win32::logic
