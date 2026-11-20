/*
 * ------------------------------------------------------------------------
 * 名称：mappers-module.cpp
 * 说明：mapper 插件模块登记项的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "logic/mappers/mappers-module.hpp"

namespace ember::win32::logic {

namespace {

/** 对话框内容区的初始高度与最小高度（逻辑像素）；宽度按内容算，打开时就用最小宽度。 */
constexpr int kDialogHeight = 560;
constexpr int kDialogMinHeight = 360;

constexpr const char* kTitle = "mapper 插件";

}  // namespace

contracts::ShellModule createMappersModule(MappersController& controller) {
  contracts::ShellModule module;
  module.name = "mappers";

  contracts::ShellDialogDefinition dialog;
  dialog.id = kMappersDialogId;
  dialog.title = kTitle;
  dialog.height = kDialogHeight;
  dialog.fitMinWidth = true;
  dialog.minHeight = kDialogMinHeight;
  module.dialogs.push_back(std::move(dialog));

  contracts::ShellCommandDefinition open;
  open.id = kMappersOpenCommand;
  open.title = [] { return std::string(kTitle); };
  open.run = [](contracts::ShellApi& shell) { shell.openDialog(kMappersDialogId); };
  module.commands.push_back(std::move(open));

  contracts::ShellMenuItemDefinition item;
  item.menu = "settings";
  item.command = kMappersOpenCommand;
  module.menuItems.push_back(std::move(item));

  module.attach = [&controller](contracts::ShellApi& shell) { return controller.attach(shell); };
  return module;
}

}  // namespace ember::win32::logic
