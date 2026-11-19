/*
 * ------------------------------------------------------------------------
 * 名称：app-frame.hpp
 * 说明：窗口框架，对应网页版组件库的 AppFrame：自上而下排布标题栏、页面区（显示区）、底部面板与状态栏，铺满窗口。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：只实现 Ember 用到的属性（见 docs/08 的用法表）：标题栏、页面区、底部面板（展开与否、高度、最小高度，不可拖动调整）、状态栏（普通或大尺寸）、窗口失去焦点；
 *       没有活动栏、边栏、标签栏与右边栏。底部面板的高度限制在“最小高度”与窗口高度的 70% 之间；
 *       有状态栏时，通知中心让出状态栏的高度再加 6 像素；页面区与底部面板各有一个区域控件，供对照与定位使用。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <memory>

#include "rujian/ui/notification.hpp"
#include "rujian/ui/status-bar.hpp"
#include "rujian/ui/title-bar.hpp"
#include "rujian/ui/widget.hpp"

namespace rujian::ui {

/** 窗口框架。 */
class AppFrame : public Widget {
 public:
  AppFrame();

  /** 设置标题栏、状态栏；返回新控件的指针。 */
  TitleBar* setTitleBar(std::unique_ptr<TitleBar> titleBar);
  StatusBar* setStatusBar(std::unique_ptr<StatusBar> statusBar);
  /** 设置页面区的内容（铺满页面区）。 */
  void setPage(std::unique_ptr<Widget> page);
  /** 设置底部面板的内容（铺满去掉顶部分割线的面板）。 */
  void setBottomPanel(std::unique_ptr<Widget> panel);
  [[nodiscard]] TitleBar* titleBar() const { return titleBar_; }
  [[nodiscard]] StatusBar* statusBar() const { return statusBar_; }
  /** 页面区与底部面板的区域控件（它们的 bounds 就是各自的区域）。 */
  [[nodiscard]] Widget* pageRegion() const { return page_; }
  [[nodiscard]] Widget* bottomRegion() const { return bottom_; }

  /** 底部面板是否展开。 */
  void setBottomPanelOpen(bool open);
  /** 底部面板的高度（含顶部分割线）与最小高度，默认 220 与 120。 */
  void setBottomPanelHeight(double height);
  void setBottomPanelMinHeight(double minHeight);
  /** 实际显示的底部面板高度：夹在最小高度与窗口高度的 70% 之间；没有展开时为 0。 */
  [[nodiscard]] double shownBottomHeight() const;
  /** 状态栏用大尺寸。 */
  void setStatusBarLarge(bool large);
  /** 窗口失去焦点：标题栏与状态栏换成失焦底色。 */
  void setWindowInactive(bool inactive);
  /** 登记通知中心：让它的位置随状态栏的高度变化。 */
  void setNotificationCenter(NotificationCenter* center);

  [[nodiscard]] Size measure(Size available) override { return available; }
  void arrange(const Rect& bounds) override;
  void paint(Canvas& canvas) override;

 private:
  TitleBar* titleBar_{nullptr};
  StatusBar* statusBar_{nullptr};
  Widget* page_{nullptr};
  Widget* bottom_{nullptr};
  NotificationCenter* notifications_{nullptr};
  bool bottomOpen_{false};
  bool statusBarLarge_{false};
  double bottomHeight_{220.0};
  double bottomMinHeight_{120.0};
};

}  // namespace rujian::ui
