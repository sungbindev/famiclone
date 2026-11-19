/*
 * ------------------------------------------------------------------------
 * 名称：shell-model.hpp
 * 说明：外壳的界面模型与外壳依赖的端口：界面层从模型取菜单、面板按钮、状态项与标题栏项的当前内容，并把用户的操作（选菜单项、点面板按钮、按快捷键、拖入文件）交回模型；
 *       模型依赖的窗口控制、对话框与通知由端口抽象，窗口与控件的具体实现在界面层与平台层。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：模型不含任何控件与系统类型；模型内容变化（命令状态、面板展开、模块状态）时调用 setOnChanged 设置的回调，界面据此重新读取并重画。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "contracts/shell/shell-registry.hpp"

namespace ember::win32::contracts {

/** 菜单里的一项（已经算好名称、勾选与可用状态）。 */
struct ShellMenuEntry {
  /** 标识：命令的标识，或“子菜单标识.项标识”；选中时传回模型。 */
  std::string id;
  std::string label;
  std::string shortcut;
  std::string hint;
  bool disabled{false};
  std::optional<bool> checked;
  /** 单选项：选中后关闭菜单。 */
  bool radio{false};
  bool separatorBefore{false};
  std::string mnemonic;
  /** 子菜单的项；非空时这一项是子菜单。 */
  std::vector<ShellMenuEntry> items;
};

/** 菜单栏里的一个菜单。 */
struct ShellMenuModel {
  std::string id;
  std::string label;
  char mnemonic{0};
  std::vector<ShellMenuEntry> items;
};

/** 系统栏左侧的面板按钮。 */
struct ShellPanelButton {
  std::string id;
  std::string title;
  ShellIcon icon{ShellIcon::PlayerPlay};
  ShellTone tone{ShellTone::Normal};
  /** 悬停提示。 */
  std::string hint;
  /** 面板开着并停在这个页签时为按下状态。 */
  bool pressed{false};
  std::optional<ShellPanelLamp> lamp;
};

/** 状态项此刻的内容。 */
struct ShellStatusItemModel {
  std::string id;
  std::string text;
  std::string title;
  ShellTone tone{ShellTone::Normal};
  bool clickable{false};
};

/** 标题栏项此刻的内容。 */
struct ShellTitleBarItemModel {
  std::string id;
  std::string text;
  std::string hint;
};

/** 展开着的底部面板。 */
struct ShellOpenPanel {
  std::string id;
  std::string title;
  /** 面板标题栏是否有“自动收缩”开关，以及开关是否开着。 */
  bool autoCollapse{false};
  bool autoCollapseOn{false};
};

/** 外壳的界面模型。 */
class ShellModel {
 public:
  virtual ~ShellModel() = default;

  /** 菜单栏的菜单；没有任何项的菜单不在里面。 */
  [[nodiscard]] virtual std::vector<ShellMenuModel> menus() = 0;
  /** 选中了菜单项（标识见 ShellMenuEntry::id）。 */
  virtual void selectMenuItem(const std::string& itemId) = 0;
  /** Alt 加助记字母对应的菜单标识；字母不是菜单的助记字母或这个菜单没有项时没有值。 */
  [[nodiscard]] virtual std::optional<std::string> menuForShortcut(const std::string& shortcut) = 0;
  /** 把按键（Windows 虚拟键码，修饰键为 Shift=1、Ctrl=2、Alt=4 的位和）写成快捷键文字，如 Ctrl+O；不是合法的快捷键时没有值。 */
  [[nodiscard]] virtual std::optional<std::string> shortcutFromKey(int virtualKey, unsigned modifiers) const = 0;
  /**
   * 按了快捷键：找到命令并执行。
   * @param shortcut 快捷键文字，如 F9。
   * @param textEditing 焦点是否在文字输入控件里：是时不带 Ctrl 或 Alt 的快捷键不触发。
   * @returns 是否有命令认领了这个快捷键（命令置灰时认领了但不执行）。
   */
  virtual bool runShortcut(const std::string& shortcut, bool textEditing) = 0;

  [[nodiscard]] virtual std::vector<ShellPanelButton> panelButtons() = 0;
  [[nodiscard]] virtual std::vector<ShellStatusItemModel> statusItems(bool alignEnd) = 0;
  [[nodiscard]] virtual std::vector<ShellTitleBarItemModel> titleBarItems() = 0;
  virtual void clickStatusItem(const std::string& itemId) = 0;
  virtual void clickTitleBarItem(const std::string& itemId) = 0;

  /** 展开着的面板；没有展开时没有值。 */
  [[nodiscard]] virtual std::optional<ShellOpenPanel> openPanel() const = 0;
  virtual void togglePanel(const std::string& panelId) = 0;
  virtual void closePanel() = 0;
  /** 切换面板的“自动收缩”开关并保存。 */
  virtual void setPanelAutoCollapse(const std::string& panelId, bool on) = 0;
  /** 点在面板之外：展开着的面板的“自动收缩”开着就收起。 */
  virtual void pressedOutsidePanel() = 0;

  [[nodiscard]] virtual bool fullScreen() const = 0;
  /** 拖入窗口的文件（UTF-8 路径）：登记后按扩展名交给认领它的处理项，没有处理项认领的用通知说明。 */
  virtual void dropFiles(const std::vector<std::string>& paths) = 0;

  /** 设置内容变化时的回调；界面据此重新读取并重画。 */
  virtual void setOnChanged(std::function<void()> handler) = 0;
};

/** 窗口控制：全屏、置顶与退出，由窗口所在的层实现。 */
class ShellWindowControl {
 public:
  virtual ~ShellWindowControl() = default;
  virtual void toggleFullScreen() = 0;
  virtual void toggleAlwaysOnTop() = 0;
  virtual void quit() = 0;
};

/** 对话框的打开者：打开（或聚焦已有的）对话框，由界面层实现。 */
class ShellDialogOpener {
 public:
  virtual ~ShellDialogOpener() = default;
  virtual void openDialog(const std::string& dialogId) = 0;
};

/** 通知的发出者，由界面层实现。 */
class ShellNotifier {
 public:
  virtual ~ShellNotifier() = default;
  virtual void notify(const std::string& text, NotificationLevel level) = 0;
};

}  // namespace ember::win32::contracts
