/*
 * ------------------------------------------------------------------------
 * 名称：slider.hpp
 * 说明：滑块控件，对应网页版组件库的 Slider：一条轨道（左边已填充部分用强调色）加一个圆形滑块，拖动或按键在最小值与最大值之间调整数值。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：行为与浏览器的 range 输入框一致：在轨道上按下，滑块立刻跳到指针位置并可继续拖动；方向键按步长增减，PageUp/PageDown 按量程的 10%，Home/End 到两端；
 *       只有控件中间 12 像素高的“输入框”区域响应鼠标（外面的 22 像素高是布局用的空白）。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>

#include "rujian/ui/widget.hpp"

namespace rujian::ui {

/** 滑块。 */
class Slider : public Widget {
 public:
  Slider() = default;

  /** 设置量程，默认 0 到 100；max 不大于 min 时滑块固定在最左边。 */
  void setRange(double min, double max);
  /** 设置步长，默认 1。 */
  void setStep(double step);
  /** 设置宽度（逻辑像素）；0 表示用令牌里的默认宽度（对应页面样式给滑块指定 width 或 flex-basis）。 */
  void setPreferredWidth(double width);
  /** 设置当前值（按量程与步长取整），不触发 onChange。 */
  void setValue(double value);
  [[nodiscard]] double value() const { return value_; }
  /** 用户改变数值后回调新的值；回调之前控件已经更新。 */
  void setOnChange(std::function<void(double)> handler) { onChange_ = std::move(handler); }

  [[nodiscard]] Size measure(Size available) override;
  [[nodiscard]] bool focusable() const override { return true; }
  [[nodiscard]] bool stretchesWidth() const override { return false; }
  [[nodiscard]] bool stretchesHeight() const override { return false; }
  [[nodiscard]] CursorKind cursor() const override { return enabled() && overField_ ? CursorKind::Hand : CursorKind::Arrow; }
  void paint(Canvas& canvas) override;
  bool onMouseDown(const MouseEvent& event) override;
  bool onMouseUp(const MouseEvent& event) override;
  bool onMouseMove(const MouseEvent& event) override;
  bool onKeyDown(const KeyEvent& event) override;
  void onStateChanged() override;

 private:
  /** 响应鼠标的“输入框”区域：控件水平铺满、垂直居中、高 12 像素。 */
  [[nodiscard]] Rect fieldRect() const;
  /** 当前值占量程的比例，0 到 1。 */
  [[nodiscard]] double ratio() const;
  /** 把窗口横坐标换成取整后的数值。 */
  [[nodiscard]] double valueAt(double x) const;
  /** 取整到步长并限制在量程内。 */
  [[nodiscard]] double normalize(double value) const;
  /** 改成新值，变化时重绘并回调。 */
  void change(double value);

  double min_{0};
  double max_{100};
  double step_{1};
  double preferredWidth_{0};
  double value_{0};
  bool dragging_{false};
  /** 鼠标是否在“输入框”区域上（悬停色只在这里生效）。 */
  bool overField_{false};
  std::function<void(double)> onChange_;
};

}  // namespace rujian::ui
