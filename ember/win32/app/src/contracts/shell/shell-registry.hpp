/*
 * ------------------------------------------------------------------------
 * 名称：shell-registry.hpp
 * 说明：外壳的登记项类型与外壳接口：各模块登记显示区、对话框、命令（含默认快捷键）、菜单项（命令或子菜单）、底部面板页签、状态项、标题栏项、文件处理项与常驻部分，
 *       外壳只渲染登记表；命令与面板经外壳接口打开对话框、展开面板、通知外壳重算命令状态。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：登记项的标识以“模块名.”开头，外壳自己的以 shell. 开头，规则见 docs/06-构建与模块登记.md 第 3.1 节与 docs/05-界面.md；
 *       本文件只有数据与接口，不含任何控件：面板内容、画面区等控件由界面层按标识提供（ui/shell 的控件登记），所以这里的面板与状态项只描述“显示什么”。
 *       显示区只有一个（中间固定显示，没有标签页），其他界面都是对话框；每个底部面板页签在系统栏左侧有一个按钮。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "contracts/shell/dropped-files.hpp"

namespace ember::win32::contracts {

/** 菜单栏的菜单，按显示顺序；没有任何菜单项的菜单不显示。 */
struct ShellMenuInfo {
  const char* id;
  const char* label;
  /** Alt 加这个字母打开菜单。 */
  char mnemonic;
};

inline constexpr ShellMenuInfo kShellMenus[] = {{"file", "文件", 'F'}, {"machine", "机器", 'M'}, {"settings", "设置", 'S'}, {"help", "帮助", 'H'}};

/** 面板按钮与状态项用到的图标。 */
enum class ShellIcon { PlayerPlay, PlayerPause, DeviceFloppy };

/** 状态项与面板按钮的等级：success 只改文字颜色，warning 与 error 配对应的图标。 */
enum class ShellTone { Normal, Success, Warning, Error };

/** 通知的级别。 */
enum class NotificationLevel { Info, Success, Warning, Error };

/** 外壳当前的视图状态，命令用它决定菜单项是否可用、是否勾选。 */
struct ShellViewState {
  bool bottomPanelOpen{false};
  /** 底部面板里当前的页签；面板关闭时为空。 */
  std::string activePanelId;
  bool fullScreen{false};
  /** 窗口是否始终置顶。 */
  bool alwaysOnTop{false};
};

/** 外壳接口：显示区、面板与命令经它调用外壳。 */
class ShellApi {
 public:
  virtual ~ShellApi() = default;
  /** 打开对话框；已打开时聚焦它。 */
  virtual void openDialog(const std::string& dialogId) = 0;
  /** 点击面板按钮：面板关着就展开并切到该页签，开着而且就是该页签就收起，开着但在别的页签就切过去。 */
  virtual void togglePanel(const std::string& panelId) = 0;
  /** 进入或退出全屏。 */
  virtual void toggleFullScreen() = 0;
  /** 窗口置顶或取消置顶。 */
  virtual void toggleAlwaysOnTop() = 0;
  /** 退出应用。 */
  virtual void quit() = 0;
  /** 执行命令；命令不存在或当前不可用时什么都不做。 */
  virtual void runCommand(const std::string& commandId) = 0;
  /** 通知外壳重新计算菜单里命令的名称、勾选与可用状态，并重画状态栏（模块自己的状态变化后调用）。 */
  virtual void refreshCommands() = 0;
  /** 弹出通知。 */
  virtual void notify(const std::string& text, NotificationLevel level) = 0;
};

/** 对话框：独立的系统窗口，同一个对话框同时只开一个，再次打开时聚焦已有的；内容由界面层按标识提供。 */
struct ShellDialogDefinition {
  /** 标识，如 preferences.preferences。 */
  std::string id;
  /** 对话框标题。 */
  std::string title;
  /** 内容区初始宽度（逻辑像素）。 */
  int width{0};
  /** 内容区初始高度；0 表示按内容。 */
  int height{0};
  /** 最小宽度与最小高度；0 表示不限。 */
  int minWidth{0};
  int minHeight{0};
};

/** 命令：菜单项与快捷键都经命令执行动作。 */
struct ShellCommandDefinition {
  /** 标识，如 runtime.powerToggle。 */
  std::string id;
  /** 菜单里显示的名称；每次渲染菜单都重新取（如“开机”与“断电”的切换）。 */
  std::function<std::string()> title;
  /** 默认快捷键，如 Ctrl+Shift+K；全部命令之间不能重复；没有时为空。 */
  std::string defaultShortcut;
  std::function<void(ShellApi&)> run;
  /** 当前是否可用，没有设置时总是可用。 */
  std::function<bool(const ShellViewState&)> enabled;
  /** 菜单项是否勾选，没有设置时不是勾选项。 */
  std::function<bool(const ShellViewState&)> checked;
};

/** 子菜单里的一项：选中时执行 run，不登记成命令（没有快捷键，项目随数据变化，如最近打开的卡带）。 */
struct ShellSubmenuEntry {
  /** 在子菜单里唯一的标识。 */
  std::string id;
  std::string label;
  /** 右侧的提示文字，如“暂未支持”。 */
  std::string hint;
  bool disabled{false};
  /** 有值时是单选项：选中的一项前面打对勾，选中只执行 run，菜单关闭。 */
  std::optional<bool> checked;
  bool separatorBefore{false};
  std::function<void(ShellApi&)> run;
};

/** 子菜单。 */
struct ShellSubmenuDefinition {
  /** 标识，以模块名开头，如 runtime.machine；子菜单项的菜单项标识是“子菜单标识.项标识”。 */
  std::string id;
  /** 菜单里显示的名称。 */
  std::string title;
  /** 当前的子菜单项；每次渲染菜单时调用，没有项时这一项置灰。 */
  std::function<std::vector<ShellSubmenuEntry>()> entries;
  /** 有值时每次渲染菜单都用它的结果作为名称（如“选择机型”显示已选的机型名），没有时用 title。 */
  std::function<std::string()> titleText;
};

/** 菜单项：放在某个菜单里的一条命令，或一个子菜单（二选一）。 */
struct ShellMenuItemDefinition {
  /** 所在菜单的标识（kShellMenus 里的 id）。 */
  std::string menu;
  /** 命令标识；与 submenu 二选一。 */
  std::string command;
  std::optional<ShellSubmenuDefinition> submenu;
  /** 助记字母（菜单打开时按它直接选中）；没有时为空。 */
  std::string mnemonic;
  bool separatorBefore{false};
};

/** 面板按钮此刻的外观：图标、颜色与悬停提示随模块状态变化。 */
struct ShellPanelLook {
  ShellIcon icon{ShellIcon::PlayerPlay};
  ShellTone tone{ShellTone::Normal};
  /** 悬停提示；为空时用面板名称。 */
  std::string hint;
};

/** 面板按钮上的指示灯（如软驱读写灯）。 */
struct ShellPanelLamp {
  bool lit{false};
  /** 可访问名称，写明这盏灯表示什么。 */
  std::string label;
  /** 画在图标里：灯心在图标内的位置（百分比，0 到 100，从左上角算）；没有值时灯是文字后的小灯。 */
  std::optional<std::pair<double, double>> inIcon;
};

/** 底部面板：同时在系统栏左侧有一个只有图标的按钮，点击展开或收起面板并切到本面板的内容；面板内容由界面层按标识提供。 */
struct ShellPanelDefinition {
  /** 标识，如 runtime.panel。 */
  std::string id;
  /** 面板名称，也是状态栏按钮的悬停提示与可访问名称。 */
  std::string title;
  /** 按钮上的图标，没有 look 时使用。 */
  ShellIcon icon{ShellIcon::PlayerPlay};
  /** 按钮当前的外观；没有设置时用 icon 与 title。 */
  std::function<ShellPanelLook()> look;
  /** 为 true 时面板标题栏有“自动收缩”开关（开关状态由外壳保存），开关开着时点面板之外的地方就收起面板。 */
  bool autoCollapse{false};
  /** 按钮上的指示灯；没有设置或返回空值表示不显示。 */
  std::function<std::optional<ShellPanelLamp>()> lamp;
};

/** 状态项此刻显示的内容。 */
struct ShellStatusItemView {
  std::string text;
  /** 悬停提示，说明这一项是什么。 */
  std::string title;
  ShellTone tone{ShellTone::Normal};
};

/** 状态栏项：放在系统栏左侧或右侧，左侧的状态项排在面板按钮之后，右侧最后一项是通知按钮。 */
struct ShellStatusItemDefinition {
  /** 标识，如 runtime.host。 */
  std::string id;
  /** true 在右侧，false 在左侧。 */
  bool alignEnd{false};
  /** 此刻显示的内容；返回空值表示没有信息，这一项不显示，不用占位字符。 */
  std::function<std::optional<ShellStatusItemView>()> view;
  /** 点击状态项；没有设置时状态项不是按钮。 */
  std::function<void(ShellApi&)> click;
};

/** 标题栏项此刻显示的内容。 */
struct ShellTitleBarItemView {
  std::string text;
  std::string hint;
};

/** 标题栏项：放在标题栏右侧、系统窗口按钮左边（如“更新”按钮）。 */
struct ShellTitleBarItemDefinition {
  /** 标识，如 system.update。 */
  std::string id;
  /** 此刻显示的内容；返回空值表示没有内容要显示。 */
  std::function<std::optional<ShellTitleBarItemView>()> view;
  std::function<void(ShellApi&)> click;
};

/** 拖入窗口的文件的处理：按扩展名认领文件，打开的办法由模块自己决定（经主进程请求的 droppedId）。 */
struct ShellFileHandlerDefinition {
  /** 标识，如 runtime.cartridge。 */
  std::string id;
  /** 认领的扩展名，小写、不带点；全部处理项之间不能重复。 */
  std::vector<std::string> extensions;
  /** 打开拖入的文件；失败由模块自己用通知提示。 */
  std::function<void(const DroppedFile&)> open;
};

/** 一个模块在外壳中的登记项。 */
struct ShellModule {
  /** 模块名，与 schema 与目录名一致。 */
  std::string name;
  /** 是否提供显示区的内容：整个应用只能有一个模块提供。 */
  bool hasScreen{false};
  std::vector<ShellDialogDefinition> dialogs;
  std::vector<ShellCommandDefinition> commands;
  std::vector<ShellMenuItemDefinition> menuItems;
  std::vector<ShellPanelDefinition> bottomPanels;
  std::vector<ShellStatusItemDefinition> statusItems;
  std::vector<ShellTitleBarItemDefinition> titleBarItems;
  std::vector<ShellFileHandlerDefinition> fileHandlers;
  /** 常驻部分：外壳启动后接上外壳接口，维护模块自己的状态、订阅核心事件，返回断开的函数；没有设置时不接。 */
  std::function<std::function<void()>(ShellApi&)> attach;
};

}  // namespace ember::win32::contracts
