/*
 * ------------------------------------------------------------------------
 * 名称：title-bar.hpp
 * 说明：标题栏，对应网页版组件库的 TitleBar：左侧是应用图标、应用名（主名与副名）与菜单栏，右侧依次是操作区（如“更新”按钮）与折叠按钮，最右边是三个系统按钮；空白处可拖动窗口。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：窗口用 WM_NCCALCSIZE 把标题栏并入客户区，系统按钮由本控件自绘，行为全部交给系统：窗口的 WM_NCHITTEST 调用 hitTestWindow，
 *       把 Minimize、Maximize、Close 转成 HTMINBUTTON、HTMAXBUTTON、HTCLOSE，空白处转成 HTCAPTION，其余是客户区；
 *       悬停与按下的状态由窗口根据非客户区鼠标消息经 setSystemButtonState 告诉本控件。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "rujian/ui/button.hpp"
#include "rujian/ui/menu.hpp"
#include "rujian/ui/widget.hpp"

namespace rujian::ui {

/** 标题栏上一个位置的命中结果。 */
enum class TitleBarHit {
  /** 在标题栏的某个控件（菜单栏、操作区、折叠按钮）上，由控件自己处理。 */
  Client,
  /** 空白处，用来拖动窗口。 */
  Caption,
  Minimize,
  Maximize,
  Close
};

/** 标题栏右侧的折叠按钮。 */
struct TitleBarTool {
  /** 标识。 */
  std::string id;
  /** 名称，作为可访问名称。 */
  std::string label;
  /** 图标。 */
  IconId icon{IconId::Count};
  /** 对应区域是否展开；展开时按钮为选中态。 */
  bool expanded{false};
  /** 悬停提示，缺省为名称。 */
  std::string title;
  /** 对应区域当前不能展开或收起时为 true，按钮置灰且不可点。 */
  bool disabled{false};
};

/** 标题栏。 */
class TitleBar : public Widget {
 public:
  TitleBar();

  /** 应用名（主名）。 */
  void setTitle(std::string title);
  /** 副名：接在应用名后，用较淡、较细的文字显示。 */
  void setSubtitle(std::string subtitle);
  /** 应用图标：16×16 的控件，空表示没有。 */
  void setLogo(std::unique_ptr<Widget> logo);
  /** 菜单栏，空表示没有。 */
  MenuBar* setMenuBar(std::unique_ptr<MenuBar> menuBar);
  [[nodiscard]] MenuBar* menuBar() const { return menuBar_; }
  /** 加一个操作区里的控件（如“更新”按钮），排在折叠按钮之前，按加入顺序从左到右。 */
  Widget* addAction(std::unique_ptr<Widget> action);
  /** 去掉全部操作区的控件。 */
  void clearActions();
  /** 设置折叠按钮。 */
  void setTools(std::vector<TitleBarTool> tools);
  /** 点击折叠按钮时以按钮标识调用。 */
  void setOnToolClick(std::function<void(const std::string& id)> handler) { onToolClick_ = std::move(handler); }
  [[nodiscard]] Button* toolButton(std::size_t index) const { return index < toolButtons_.size() ? toolButtons_[index] : nullptr; }
  [[nodiscard]] const std::vector<Widget*>& actions() const { return actions_; }

  /** 是否显示三个系统按钮（只有窗口自绘标题栏时才显示）。 */
  void setWindowButtons(bool show);
  /** 窗口是否已最大化：最大化时中间的系统按钮显示“还原”图形。 */
  void setMaximized(bool maximized);
  /** 窗口是否是活动窗口：不活动时换成失焦的底色。 */
  void setWindowActive(bool active);
  /** 系统按钮的悬停与按下状态（不是系统按钮时传 Client）。 */
  void setSystemButtonState(TitleBarHit hover, TitleBarHit pressed);
  /** 标题栏上某点的命中结果（窗口坐标）；点不在标题栏内时为 Client。 */
  [[nodiscard]] TitleBarHit hitTestWindow(Point point) const;
  /** 系统按钮的区域（窗口坐标）；不是系统按钮或没有显示时为空矩形。 */
  [[nodiscard]] Rect systemButtonRect(TitleBarHit button) const;

  [[nodiscard]] Size measure(Size available) override;
  void arrange(const Rect& bounds) override;
  void paint(Canvas& canvas) override;
  void paintChildren(Canvas& canvas) override;
  [[nodiscard]] bool stretchesHeight() const override { return false; }

 private:
  [[nodiscard]] double borderWidth() const;
  [[nodiscard]] TextStyle titleStyle(Color color) const;
  [[nodiscard]] TextStyle subtitleStyle(Color color) const;
  [[nodiscard]] double titleWidth() const;
  [[nodiscard]] double subtitleWidth() const;
  void relayout();
  void paintSystemButtons(Canvas& canvas);

  std::string title_;
  std::string subtitle_;
  Widget* logo_{nullptr};
  MenuBar* menuBar_{nullptr};
  std::vector<Widget*> actions_;
  std::vector<Button*> toolButtons_;
  std::function<void(const std::string&)> onToolClick_;
  bool windowButtons_{false};
  bool maximized_{false};
  bool windowActive_{true};
  TitleBarHit hover_{TitleBarHit::Client};
  TitleBarHit pressed_{TitleBarHit::Client};
  /** 应用名文字的起点（窗口坐标）。 */
  double titleX_{0};
};

}  // namespace rujian::ui
