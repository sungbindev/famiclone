/*
 * ------------------------------------------------------------------------
 * 名称：main-window-view.cpp
 * 说明：主窗口内容的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "ui/shell/main-window-view.hpp"

#include "rujian/ui/canvas.hpp"
#include "rujian/ui/host.hpp"
#include "rujian/ui/theme.hpp"

namespace ember::win32::ui {

namespace {

/** 窗口边框的宽度（逻辑像素）。 */
constexpr double kBorderWidth = 1.0;

}  // namespace

MainWindowView::MainWindowView(std::string title) {
  auto frame = std::make_unique<rujian::ui::AppFrame>();
  frame_ = frame.get();
  auto titleBar = std::make_unique<rujian::ui::TitleBar>();
  titleBar->setTitle(std::move(title));
  titleBar->setWindowButtons(true);
  titleBar_ = frame_->setTitleBar(std::move(titleBar));
  frame_->setStatusBarLarge(true);
  frame_->setStatusBar(std::make_unique<rujian::ui::StatusBar>());
  frame_->setPage(std::make_unique<rujian::ui::Widget>());
  // 底部面板收起；展开的高度由面板内容决定，最小高度为 0
  frame_->setBottomPanel(std::make_unique<rujian::ui::Widget>());
  frame_->setBottomPanelMinHeight(0);
  frame_->setBottomPanelOpen(false);
  addChild(std::move(frame));
}

rujian::ui::TitleBarHit MainWindowView::hitTestWindow(rujian::ui::Point point) const { return titleBar_->hitTestWindow(point); }

void MainWindowView::setSystemButtonState(rujian::ui::TitleBarHit hover, rujian::ui::TitleBarHit pressed) { titleBar_->setSystemButtonState(hover, pressed); }

void MainWindowView::setMaximized(bool maximized) {
  maximized_ = maximized;
  titleBar_->setMaximized(maximized);
  invalidate();
}

void MainWindowView::setWindowActive(bool active) {
  frame_->setWindowInactive(!active);
  titleBar_->setWindowActive(active);
}

void MainWindowView::setBorderVisible(bool visible) {
  if (borderVisible_ == visible) return;
  borderVisible_ = visible;
  invalidate();
}

void MainWindowView::paintChildren(rujian::ui::Canvas& canvas) {
  Widget::paintChildren(canvas);
  if (borderVisible_ && !maximized_) canvas.strokeOutline(bounds(), 0, -kBorderWidth, kBorderWidth, rujian::ui::windowBorderColor(host()->theme()));
}

}  // namespace ember::win32::ui
