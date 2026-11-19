/*
 * ------------------------------------------------------------------------
 * 名称：main-window-view.hpp
 * 说明：主窗口的内容：窗口框架（标题栏、显示区、底部面板、系统栏）的空壳，以及系统不画阴影时自绘的 1 像素窗口边框。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：布局与 docs/05-界面.md 第 2 节一致：标题栏高 36、显示区铺满剩余空间、底部面板默认收起、系统栏用大尺寸（高 32）；
 *       标题栏里的图标、菜单与“更新”按钮，系统栏里的各项，显示区里的运行画面都由外壳模块与运行模块往里登记（阶段 7 起）；
 *       窗口的非客户区消息（缩放边框、拖动、系统按钮）由平台层处理，这里只回答“某个位置是标题栏的什么”并接收系统按钮的悬停、最大化与激活状态。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <memory>
#include <string>

#include "rujian/ui/app-frame.hpp"
#include "rujian/ui/title-bar.hpp"
#include "rujian/ui/widget.hpp"

namespace ember::win32::ui {

/** 主窗口的内容。 */
class MainWindowView : public rujian::ui::Widget {
 public:
  /** @param title 标题栏上的应用名。 */
  explicit MainWindowView(std::string title);

  /** 窗口框架。 */
  [[nodiscard]] rujian::ui::AppFrame& frame() { return *frame_; }

  /** 标题栏。 */
  [[nodiscard]] rujian::ui::TitleBar& titleBar() { return *titleBar_; }

  /** 窗口里一个位置（逻辑像素）是标题栏的什么：控件、空白或系统按钮。 */
  [[nodiscard]] rujian::ui::TitleBarHit hitTestWindow(rujian::ui::Point point) const;

  /** 系统按钮的悬停与按下状态。 */
  void setSystemButtonState(rujian::ui::TitleBarHit hover, rujian::ui::TitleBarHit pressed);

  /** 窗口是否最大化：最大化时“还原”图形替换“最大化”图形，也不画窗口边框。 */
  void setMaximized(bool maximized);

  /** 窗口是否是活动窗口：不活动时标题栏与系统栏换成失焦的底色。 */
  void setWindowActive(bool active);

  /** 是否自绘 1 像素窗口边框（系统关闭窗口合成、没有阴影时）。 */
  void setBorderVisible(bool visible);

  void paintChildren(rujian::ui::Canvas& canvas) override;

 private:
  rujian::ui::AppFrame* frame_{nullptr};
  rujian::ui::TitleBar* titleBar_{nullptr};
  bool maximized_{false};
  bool borderVisible_{false};
};

}  // namespace ember::win32::ui
