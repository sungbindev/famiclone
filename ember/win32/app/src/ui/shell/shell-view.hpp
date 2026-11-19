/*
 * ------------------------------------------------------------------------
 * 名称：shell-view.hpp
 * 说明：外壳的界面：用控件库的窗口框架拼出标题栏与菜单、中间固定的显示区、底部面板、系统栏与通知，内容都来自外壳模型（登记表）；处理窗口级的快捷键与 Alt+助记字母、面板的自动收缩、焦点回到画面。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：对应 Electron 外壳的 Shell.tsx；规则见 docs/05-界面.md 第 2、3、5、6 节：没有活动栏、边栏、标签栏与右边栏；底部面板只经系统栏左侧的面板按钮展开，高度由内容决定，不能拖动、不保存；
 *       全屏时只显示画面（标题栏、系统栏与底部面板都隐藏）；显示区与面板内容是控件，由组装根按标识注入（界面各模块之间不互相引用）。
 *       模型变化时先标记，经 post 推迟到当前事件处理之后再重建，免得在按钮自己的点击回调里销毁按钮；每一部分内容没变时不重建（菜单打开期间状态变化不会把菜单弄关）。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "contracts/shell/shell-model.hpp"
#include "rujian/ui/notification.hpp"
#include "rujian/ui/root.hpp"
#include "rujian/ui/tooltip.hpp"
#include "ui/shell/main-window-view.hpp"

namespace ember::win32::ui {

/** 外壳的界面。 */
class ShellView final : public contracts::ShellNotifier {
 public:
  /** 控件的工厂：每次要显示时创建一个新的。 */
  using WidgetFactory = std::function<std::unique_ptr<rujian::ui::Widget>()>;

  /**
   * @param root 窗口的根容器，通知与悬停提示要用；必须比本对象活得久。
   * @param view 主窗口的内容，必须比本对象活得久。
   * @param model 外壳模型。
   * @param post 把任务推到当前事件处理之后执行（界面线程）。
   */
  ShellView(rujian::ui::Root& root, MainWindowView& view, contracts::ShellModel& model, std::function<void(std::function<void()>)> post);
  ~ShellView() override;
  ShellView(const ShellView&) = delete;
  ShellView& operator=(const ShellView&) = delete;

  /** 设置显示区的内容（整个应用只有一个）。 */
  void setScreen(std::unique_ptr<rujian::ui::Widget> screen);
  /** 登记底部面板页签的内容工厂；面板收起时内容销毁，展开时重新创建。 */
  void setPanelContent(const std::string& panelId, WidgetFactory factory);
  /** 登记把焦点还给画面区的函数：主窗口里点完鼠标、菜单关闭后调用。 */
  void setFocusScreen(std::function<void()> focus) { focusScreen_ = std::move(focus); }

  /** 立即按模型重建全部内容（启动时调用一次）。 */
  void refresh();

  /**
   * 窗口级的按键：通知面板的 Esc、Alt+助记字母打开菜单、命令的快捷键。
   * @param virtualKey Windows 虚拟键码。
   * @param modifiers 修饰键组合（Ctrl、Shift、Alt）。
   * @param repeat 是否是按住不放产生的重复事件。
   * @returns 是否已处理（已处理的按键不再交给控件）。
   */
  bool handleKeyDown(int virtualKey, unsigned modifiers, bool repeat);
  /** 在窗口里按下了鼠标（窗口坐标，逻辑像素）：面板的“自动收缩”据此收起面板。 */
  void handleMouseDown(rujian::ui::Point position);
  /** 在窗口里松开了鼠标：焦点还给画面区（焦点在文字输入控件上、菜单打开着时不还）。 */
  void handleMouseUp();
  /** 弹层此刻盖住的区域（窗口坐标）：点在弹层上不算点在面板之外。 */
  void setLayerRects(std::vector<rujian::ui::Rect> rects) { layerRects_ = std::move(rects); }

  /** 全屏时只显示画面。 */
  void setFullScreen(bool fullScreen);

  [[nodiscard]] rujian::ui::NotificationCenter& notifications() { return *center_; }
  [[nodiscard]] rujian::ui::MenuBar* menuBar() const { return menuBar_; }

  // ---- ShellNotifier ----
  void notify(const std::string& text, contracts::NotificationLevel level) override;

 private:
  void scheduleRefresh();
  void rebuildMenus();
  void rebuildStatusBar();
  void rebuildTitleBarItems();
  void rebuildPanel();
  void applyFullScreen();

  rujian::ui::Root& root_;
  MainWindowView& view_;
  contracts::ShellModel& model_;
  std::function<void(std::function<void()>)> post_;
  std::unique_ptr<rujian::ui::NotificationCenter> center_;
  std::unique_ptr<rujian::ui::Tooltip> tooltip_;
  rujian::ui::MenuBar* menuBar_{nullptr};
  std::map<std::string, WidgetFactory> panelFactories_;
  std::function<void()> focusScreen_;
  std::vector<rujian::ui::Rect> layerRects_;
  /** 系统栏左侧的面板按钮（点它们不算点在面板之外）。 */
  std::vector<rujian::ui::Widget*> panelButtons_;
  /** 每一部分上一次重建时的内容签名，没变就不重建。 */
  std::string menusSignature_;
  std::string statusSignature_;
  std::string titleSignature_;
  std::string panelSignature_;
  bool refreshPending_{false};
  bool fullScreen_{false};
  /** 对象存在期间为 true：推迟的重建执行时对象已经销毁就不处理。 */
  std::shared_ptr<bool> alive_{std::make_shared<bool>(true)};
};

}  // namespace ember::win32::ui
