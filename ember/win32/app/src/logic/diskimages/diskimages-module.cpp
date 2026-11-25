/*
 * ------------------------------------------------------------------------
 * 名称：diskimages-module.cpp
 * 说明：磁盘镜像管理模块登记项的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-25
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "logic/diskimages/diskimages-module.hpp"

namespace ember::win32::logic {

namespace {

/** 对话框内容区的初始大小与最小大小（逻辑像素）。 */
constexpr int kDialogWidth = 960;
constexpr int kDialogHeight = 560;
constexpr int kDialogMinWidth = 640;
constexpr int kDialogMinHeight = 360;

constexpr const char* kTitle = "磁盘镜像管理";
/** “文件”菜单里这一项的助记字母。 */
constexpr const char* kMnemonic = "D";

}  // namespace

contracts::ShellModule createDiskImagesModule(DiskImagesController& controller) {
  contracts::ShellModule module;
  module.name = "diskimages";

  contracts::ShellDialogDefinition dialog;
  dialog.id = kDiskImagesDialogId;
  dialog.title = kTitle;
  dialog.width = kDialogWidth;
  dialog.height = kDialogHeight;
  dialog.minWidth = kDialogMinWidth;
  dialog.minHeight = kDialogMinHeight;
  dialog.modeless = true;
  module.dialogs.push_back(std::move(dialog));

  contracts::ShellCommandDefinition open;
  open.id = kDiskImagesOpenCommand;
  open.title = [] { return std::string(kTitle); };
  open.run = [](contracts::ShellApi& shell) { shell.openDialog(kDiskImagesDialogId); };
  module.commands.push_back(std::move(open));

  contracts::ShellMenuItemDefinition item;
  item.menu = "file";
  item.command = kDiskImagesOpenCommand;
  item.mnemonic = kMnemonic;
  item.separatorBefore = true;
  module.menuItems.push_back(std::move(item));

  module.attach = [&controller](contracts::ShellApi& shell) { return controller.attach(shell); };
  return module;
}

}  // namespace ember::win32::logic
