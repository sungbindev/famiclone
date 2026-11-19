/*
 * ------------------------------------------------------------------------
 * 名称：screen-overlay-view.hpp
 * 说明：画面上的提示：没有开机显示“未开机”并指明怎么开机，断电显示“已断电”，暂停显示“已暂停”；运行中与还没读到状态时不显示；机器带鼠标时，画面右下角的“点击画面捕获鼠标，双击右键释放”。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：颜色与画面的黑底一样，不随主题变化（画面是被模拟的内容）；
 *       没有开机与断电时机器没有画面，提示的底色是纯黑（盖住残留的上一帧），暂停时是半透明，透出暂停的画面。
 *       这个控件只用来画：画面控件把它画到一张预乘透明度的位图上，交给画面子窗口叠在 Direct3D 的画面上（见 docs/08 第 5.5 节）。
 * ------------------------------------------------------------------------
 */

#pragma once

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
   * @param captureHint 是否在右下角显示捕获鼠标的提示。
   */
  ScreenOverlayView(OverlayState state, bool captureHint) : state_(state), captureHint_(captureHint) {}

  void paint(rujian::ui::Canvas& canvas) override;

 private:
  void paintState(rujian::ui::Canvas& canvas) const;
  void paintCaptureHint(rujian::ui::Canvas& canvas) const;

  OverlayState state_;
  bool captureHint_;
};

}  // namespace ember::win32::ui
