/*
 * ------------------------------------------------------------------------
 * 名称：status-bar.hpp
 * 说明：状态栏，对应网页版组件库的 StatusBar 与 StatusBarItem：窗口底部的一条，左侧放说明性的状态，右侧放状态项与通知按钮；
 *       状态项可以是文字，也可以是按钮；状态项可带图标、状态点、指示灯，有等级（颜色与图标），按钮状态项有按下态。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：状态栏有普通与大两种尺寸（对应 AppFrame 的 statusBarSize），尺寸由状态栏统一设置并同步给它的状态项；
 *       左侧放不下时，文字较长（超过 16 个字符）的状态项按文字长度的平方为权重收缩并以省略号结尾，较短的状态项与右侧保持原宽。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "rujian/ui/widget.hpp"

namespace rujian::ui {

/** 状态项的等级：success 只改文字颜色（图标有强调部分时，强调部分也填成成功色）；warning 与 error 在没有指定图标时配对应的图标。 */
enum class StatusItemTone { Normal, Success, Warning, Error };

/** 状态点：绿色（如运行中）、黄色（如已暂停）、灰色（如未开机）；状态不能只靠颜色，状态点旁必须有文字。 */
enum class StatusDot { None, Success, Warning, Neutral };

/** 状态项上的指示灯。 */
struct StatusLamp {
  /** 是否亮着。 */
  bool lit{false};
  /** 可访问名称，写明这盏灯表示什么，如“软驱读写灯”。 */
  std::string label;
  /** 画在图标里：灯的中心在图标内的位置（百分比，0 到 100，从左上角算）；灯是圆点，盖在图标的圆形部位上；没有值时灯是文字后的长方形小灯。 */
  std::optional<Point> inIcon;
  /** 把灯画成图标里的强调部分（图标数据里 fill 为 accent 的路径）：亮时填充灯的亮色，熄时填充暗色；图标没有强调部分时没有灯。 */
  bool onIconAccent{false};
};

/** 状态项。 */
class StatusBarItem : public Widget {
 public:
  /** @param text 文字（UTF-8）。 */
  explicit StatusBarItem(std::string text);

  void setText(std::string text);
  /** 文字前的图标；没有时等级为 warning、error 的状态项用对应的图标。 */
  void setIcon(std::optional<IconId> icon);
  void setTone(StatusItemTone tone);
  /** 文字前的状态点（有状态点时不显示图标）。 */
  void setDot(StatusDot dot);
  /** 指示灯：长方形小灯在文字后，或画在图标里（见 StatusLamp::inIcon）。 */
  void setLamp(std::optional<StatusLamp> lamp);
  /** 按下状态：用于开关面板的按钮，只改背景；仅按钮状态项有效。 */
  void setPressed(bool pressed);
  /** 只显示图标：文字不显示（悬停提示由提示控件处理）；需要同时有图标。 */
  void setIconOnly(bool iconOnly);
  /** 点击回调；设置后状态项是按钮（可悬停、按下、获得焦点、点击）。 */
  void setOnClick(std::function<void()> handler);
  /** 是否使用大尺寸（由所在的状态栏统一设置）。 */
  void setLarge(bool large);

  [[nodiscard]] const std::string& text() const { return text_; }
  [[nodiscard]] bool clickable() const { return static_cast<bool>(onClick_); }
  /** 放不下时的收缩权重：文字超过 16 个字符为字符数的平方，否则为 0（不收缩）。 */
  [[nodiscard]] double shrinkWeight() const;
  /** 指示灯（没有时为空）。 */
  [[nodiscard]] const std::optional<StatusLamp>& lamp() const { return lamp_; }

  [[nodiscard]] Size measure(Size available) override;
  [[nodiscard]] bool focusable() const override { return clickable() && enabled(); }
  [[nodiscard]] bool stretchesHeight() const override { return false; }
  [[nodiscard]] CursorKind cursor() const override { return clickable() ? CursorKind::Hand : CursorKind::Arrow; }
  void paint(Canvas& canvas) override;
  bool onMouseDown(const MouseEvent& event) override;
  bool onMouseUp(const MouseEvent& event) override;
  bool onKeyDown(const KeyEvent& event) override;
  bool onKeyUp(const KeyEvent& event) override;
  void onStateChanged() override;

 private:
  [[nodiscard]] std::optional<IconId> shownIcon() const;
  [[nodiscard]] bool lampInIcon() const;
  [[nodiscard]] double iconSize() const;
  [[nodiscard]] double dotSize() const;
  [[nodiscard]] double paddingX() const;
  [[nodiscard]] TextStyle textStyle(Color color) const;
  [[nodiscard]] double textWidth() const;
  void click();

  std::string text_;
  std::optional<IconId> icon_;
  StatusItemTone tone_{StatusItemTone::Normal};
  StatusDot dot_{StatusDot::None};
  std::optional<StatusLamp> lamp_;
  bool pressedFlag_{false};
  bool iconOnly_{false};
  bool large_{false};
  std::function<void()> onClick_;
  bool spaceDown_{false};
};

/** 状态栏。 */
class StatusBar : public Widget {
 public:
  StatusBar();

  /** 往左侧加一个状态项，返回它的指针。 */
  StatusBarItem* addStart(std::unique_ptr<StatusBarItem> item);
  /** 往右侧加一个状态项（右侧靠右排列），返回它的指针。 */
  StatusBarItem* addEnd(std::unique_ptr<StatusBarItem> item);
  /** 往右侧加一个别的控件（如通知按钮），保持原宽靠右排列。 */
  Widget* addEndWidget(std::unique_ptr<Widget> widget);
  /** 去掉全部状态项。 */
  void clear();
  /** 大尺寸状态栏：更高，文字、图标与按钮更大。 */
  void setLarge(bool large);
  [[nodiscard]] bool large() const { return large_; }
  /** 窗口是否是活动窗口：不活动时换成失焦的底色。 */
  void setWindowActive(bool active);

  [[nodiscard]] Size measure(Size available) override;
  void arrange(const Rect& bounds) override;
  void paint(Canvas& canvas) override;
  [[nodiscard]] bool stretchesHeight() const override { return false; }

 private:
  [[nodiscard]] double borderWidth() const;

  std::vector<StatusBarItem*> start_;
  std::vector<Widget*> end_;
  bool large_{false};
  bool windowActive_{true};
};

}  // namespace rujian::ui
