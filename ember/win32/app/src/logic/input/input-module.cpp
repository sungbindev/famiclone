/*
 * ------------------------------------------------------------------------
 * 名称：input-module.cpp
 * 说明：输入模块登记项的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-20
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "logic/input/input-module.hpp"

namespace ember::win32::logic {

namespace {

/** 对话框内容区的初始尺寸（逻辑像素）；没有最小尺寸。 */
constexpr int kDialogWidth = 640;
constexpr int kDialogHeight = 560;

constexpr const char* kTitle = "手柄设置";

}  // namespace

contracts::ShellModule createInputModule(GamepadSettingsController& controller) {
  contracts::ShellModule module;
  module.name = "input";

  contracts::ShellDialogDefinition dialog;
  dialog.id = kGamepadSettingsDialogId;
  dialog.title = kTitle;
  dialog.width = kDialogWidth;
  dialog.height = kDialogHeight;
  module.dialogs.push_back(std::move(dialog));

  contracts::ShellCommandDefinition open;
  open.id = kGamepadSettingsOpenCommand;
  open.title = [] { return std::string(kTitle); };
  open.run = [](contracts::ShellApi& shell) { shell.openDialog(kGamepadSettingsDialogId); };
  module.commands.push_back(std::move(open));

  contracts::ShellMenuItemDefinition item;
  item.menu = "settings";
  item.command = kGamepadSettingsOpenCommand;
  item.separatorBefore = true;
  module.menuItems.push_back(std::move(item));

  module.attach = [&controller](contracts::ShellApi& shell) { return controller.attach(shell); };
  return module;
}

}  // namespace ember::win32::logic
