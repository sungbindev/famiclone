/*
 * ------------------------------------------------------------------------
 * 名称：preferences-module.cpp
 * 说明：首选项模块登记项的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-20
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "logic/preferences/preferences-module.hpp"

namespace ember::win32::logic {

namespace {

constexpr const char* kTitle = "首选项";

}  // namespace

contracts::ShellModule createPreferencesModule(PreferencesController& preferences, UserDataController& userData) {
  contracts::ShellModule module;
  module.name = "preferences";

  contracts::ShellDialogDefinition dialog;
  dialog.id = kPreferencesDialogId;
  dialog.title = kTitle;
  dialog.fitMinWidth = true;
  dialog.fitMinHeight = true;
  module.dialogs.push_back(std::move(dialog));

  contracts::ShellCommandDefinition open;
  open.id = kPreferencesOpenCommand;
  open.title = [] { return std::string(kTitle); };
  open.defaultShortcut = "Ctrl+,";
  open.run = [](contracts::ShellApi& shell) { shell.openDialog(kPreferencesDialogId); };
  module.commands.push_back(std::move(open));

  contracts::ShellMenuItemDefinition item;
  item.menu = "settings";
  item.command = kPreferencesOpenCommand;
  module.menuItems.push_back(std::move(item));

  module.attach = [&preferences, &userData](contracts::ShellApi& shell) -> std::function<void()> {
    auto stopPreferences = preferences.attach(shell);
    auto stopUserData = userData.attach(shell);
    return [stopPreferences = std::move(stopPreferences), stopUserData = std::move(stopUserData)] {
      stopPreferences();
      stopUserData();
    };
  };
  return module;
}

}  // namespace ember::win32::logic
