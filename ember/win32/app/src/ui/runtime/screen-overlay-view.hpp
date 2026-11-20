/*
 * ------------------------------------------------------------------------
 * 名称：screen-overlay-view.hpp
 * 说明：画面上的提示：没有开机显示“未开机”并指明怎么开机，断电显示“已断电”，暂停显示“已暂停”；运行中与还没读到状态时不显示；右下角有“键盘已被机器独占”与“点击捕获鼠标，双击右键释放”两个提示框，上下排列、各占一行（有哪个显示哪个）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：颜色与画面的黑底一样，不随主题变化（画面是被模拟的内容）；
 *       没有开机与断电时机器没有画面，提示的底色是纯黑（盖住残留的上一帧），暂停时是半透明，透出暂停的画面；
 *       提示框的底色与边框都是半透明的，有圆角（kCornerRadius），毛玻璃（框里的画面模糊）由画面子窗口在叠上位图之前完成，框的矩形由 glassRects 给出；
 *       这个控件只用来画：画面控件把它画到一张预乘透明度的位图上，交给画面子窗口叠在 Direct3D 的画面上（见 docs/08 第 5.5 节）。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <vector>

#include "rujian/ui/widget.hpp"

namespace ember::win32::ui {

/** 要显示的提示。 */
enum class OverlayState {
  /** 不显示状态提示（运行中、还没读到状态）。 */
  Hidden,
  /** 没有开机。 */
  NotPoweredOn,
  /** 已断电。 */
  PoweredOff,
  /** 已暂停。 */
  Paused
};

/** 画面提示。 */
class ScreenOverlayView final : public rujian::ui::Widget {
 public:
  /**
   * @param state 状态提示。
   * @param mouseHint 右下角的提示框里是否有捕获鼠标的提示。
   * @param keyboardHint 右下角的提示框里是否有键盘独占的提示。
   */
  ScreenOverlayView(OverlayState state, bool mouseHint, bool keyboardHint) : state_(state), mouseHint_(mouseHint), keyboardHint_(keyboardHint) {}

  /** 提示框的圆角半径（逻辑像素，取控件库的 radius-lg）。 */
  static constexpr double kCornerRadius = 6;

  /** 各个提示框的矩形（逻辑像素，相对本控件），从上到下；没有提示框时为空。在布局之后调用。 */
  [[nodiscard]] std::vector<rujian::ui::Rect> glassRects() const;

  void paint(rujian::ui::Canvas& canvas) override;

 private:
  /** 一个提示框：矩形、文字与文字排版后的大小。 */
  struct PanelBox {
    rujian::ui::Rect box;
    const char* text;
    rujian::ui::TextMetrics size;
  };

  [[nodiscard]] std::vector<PanelBox> layoutPanels() const;
  void paintState(rujian::ui::Canvas& canvas) const;
  void paintPanels(rujian::ui::Canvas& canvas) const;

  OverlayState state_;
  bool mouseHint_;
  bool keyboardHint_;
};

}  // namespace ember::win32::ui
