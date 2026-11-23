/*
 * ------------------------------------------------------------------------
 * 名称：shell-module.cpp
 * 说明：外壳自己的登记项的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "logic/shell/shell-module.hpp"

#include "logic/shell/shell-registry-builder.hpp"

namespace ember::win32::logic {

contracts::ShellModule createShellModule() {
  using contracts::ShellApi;
  using contracts::ShellCommandDefinition;
  using contracts::ShellMenuItemDefinition;
  using contracts::ShellViewState;

  contracts::ShellModule module;
  module.name = kShellNamespace;

  ShellCommandDefinition quit;
  quit.id = kShellQuitCommand;
  quit.title = [] { return std::string("退出"); };
  quit.run = [](ShellApi& shell) { shell.quit(); };
  module.commands.push_back(std::move(quit));

  ShellCommandDefinition fullScreen;
  fullScreen.id = kShellFullScreenCommand;
  fullScreen.title = [] { return std::string("全屏"); };
  fullScreen.defaultShortcut = "F11";
  fullScreen.run = [](ShellApi& shell) { shell.toggleFullScreen(); };
  fullScreen.checked = [](const ShellViewState& view) { return view.fullScreen; };
  module.commands.push_back(std::move(fullScreen));

  ShellCommandDefinition alwaysOnTop;
  alwaysOnTop.id = kShellAlwaysOnTopCommand;
  alwaysOnTop.title = [] { return std::string("窗口始终置顶"); };
  alwaysOnTop.run = [](ShellApi& shell) { shell.toggleAlwaysOnTop(); };
  alwaysOnTop.checked = [](const ShellViewState& view) { return view.alwaysOnTop; };
  module.commands.push_back(std::move(alwaysOnTop));

  // “画面尺寸”子菜单：按下的那一刻把窗口调成相应大小，尺寸不记忆，所以没有对勾；窗口相关的三项排在一组，前面有分隔线
  ShellMenuItemDefinition screenSizeItem;
  screenSizeItem.menu = "settings";
  screenSizeItem.separatorBefore = true;
  screenSizeItem.submenu = contracts::ShellSubmenuDefinition{kShellScreenSizeSubmenu, "画面尺寸", [] {
    std::vector<contracts::ShellSubmenuEntry> entries;
    for (const int multiple : {1, 2, 3}) {
      contracts::ShellSubmenuEntry entry;
      entry.id = std::to_string(multiple) + "x";
      entry.label = entry.id;
      entry.run = [multiple](ShellApi& shell) { shell.setScreenSize(multiple); };
      entries.push_back(std::move(entry));
    }
    return entries;
  }};
  module.menuItems.push_back(std::move(screenSizeItem));

  ShellMenuItemDefinition fullScreenItem;
  fullScreenItem.menu = "settings";
  fullScreenItem.command = kShellFullScreenCommand;
  module.menuItems.push_back(std::move(fullScreenItem));

  ShellMenuItemDefinition alwaysOnTopItem;
  alwaysOnTopItem.menu = "settings";
  alwaysOnTopItem.command = kShellAlwaysOnTopCommand;
  module.menuItems.push_back(std::move(alwaysOnTopItem));

  ShellMenuItemDefinition quitItem;
  quitItem.menu = "file";
  quitItem.command = kShellQuitCommand;
  quitItem.separatorBefore = true;
  module.menuItems.push_back(std::move(quitItem));
  return module;
}

}  // namespace ember::win32::logic
