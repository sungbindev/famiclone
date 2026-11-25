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

/** 对话框内容区的初始高度与最小高度（逻辑像素）；宽度按内容算：只显示资源管理器时就是它的最小宽度，展开镜像内容后窗口会变宽。 */
constexpr int kDialogHeight = 560;
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
  dialog.height = kDialogHeight;
  dialog.minHeight = kDialogMinHeight;
  dialog.fitMinWidth = true;
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
