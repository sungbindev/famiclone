/*
 * ------------------------------------------------------------------------
 * 名称：slider.cpp
 * 说明：滑块控件的实现，尺寸与颜色取自生成的令牌，取值与 ui-slider.css 一致。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：轨道的填充是一条“两色硬边”的渐变：边缘落在填充比例处，每个设备像素要么是强调色要么是轨道底色，所以用对齐像素的裁剪分两次画；
 *       滑块中心的活动范围是轨道宽度减去滑块直径，填充边缘却按整条轨道的宽度算（与浏览器一致）。
 * ------------------------------------------------------------------------
 */

#include "rujian/ui/slider.hpp"

#include <algorithm>
#include <cmath>

#include "base/theme-tokens.hpp"

namespace rujian::ui {

namespace {

constexpr int kVirtualKeyPageUp = 0x21;
constexpr int kVirtualKeyPageDown = 0x22;
constexpr int kVirtualKeyEnd = 0x23;
constexpr int kVirtualKeyHome = 0x24;
constexpr int kVirtualKeyLeft = 0x25;
constexpr int kVirtualKeyUp = 0x26;
constexpr int kVirtualKeyRight = 0x27;
constexpr int kVirtualKeyDown = 0x28;
/** outline: 1px solid; outline-offset: 2px。 */
constexpr double kFocusWidth = 1.0;
constexpr double kFocusOffset = 2.0;
/** PageUp 与 PageDown 一次移动量程的这个比例。 */
constexpr double kPageFraction = 0.1;

}  // namespace

void Slider::setRange(double min, double max) {
  min_ = min;
  max_ = max;
  value_ = normalize(value_);
  invalidate();
}

void Slider::setStep(double step) {
  step_ = step > 0 ? step : 1;
  value_ = normalize(value_);
  invalidate();
}

void Slider::setValue(double value) {
  value_ = normalize(value);
  invalidate();
}

double Slider::normalize(double value) const {
  if (max_ <= min_) return min_;
  // 取整到离最小值最近的整数个步长，量程的最大值总是可以到达
  const double snapped = min_ + std::round((value - min_) / step_) * step_;
  return std::clamp(snapped, min_, max_);
}

double Slider::ratio() const { return max_ > min_ ? std::clamp((value_ - min_) / (max_ - min_), 0.0, 1.0) : 0.0; }

Size Slider::measure(Size /*available*/) { return Size{preferredWidth_ > 0 ? preferredWidth_ : tokens::slider_width, tokens::slider_height}; }

void Slider::setPreferredWidth(double width) {
  if (preferredWidth_ == width) return;
  preferredWidth_ = width;
  invalidate();
}

Rect Slider::fieldRect() const {
  const Rect box = bounds();
  return Rect{box.x, box.y + (box.height - tokens::slider_thumb_size) / 2.0, box.width, tokens::slider_thumb_size};
}

double Slider::valueAt(double x) const {
  const Rect field = fieldRect();
  const double travel = field.width - tokens::slider_thumb_size;
  if (travel <= 0) return min_;
  const double position = std::clamp((x - field.x - tokens::slider_thumb_size / 2.0) / travel, 0.0, 1.0);
  return normalize(min_ + position * (max_ - min_));
}

void Slider::change(double value) {
  value = normalize(value);
  if (value == value_) return;
  value_ = value;
  invalidate();
  if (onChange_) onChange_(value_);
}

void Slider::paint(Canvas& canvas) {
  const auto& colors = themeTokens(host()->theme());
  const Rect field = fieldRect();
  const bool off = !enabled();
  const Rect track{field.x, field.y + (field.height - tokens::slider_track_height) / 2.0, field.width, tokens::slider_track_height};
  const double radius = tokens::slider_track_height / 2.0;

  if (off) {
    canvas.drawBox(track, BoxStyle{.fill = colors.disabled_border, .radius = radius});
  } else {
    // 填充边缘的两侧各画一次整条轨道，用对齐像素的裁剪把它们分开
    const double edge = track.x + track.width * ratio();
    canvas.pushClip(Rect{track.x, track.y, edge - track.x, track.height});
    canvas.drawBox(track, BoxStyle{.fill = colors.accent, .radius = radius});
    canvas.popClip();
    canvas.pushClip(Rect{edge, track.y, track.right() - edge, track.height});
    canvas.drawBox(track, BoxStyle{.fill = colors.slider_track_bg, .radius = radius});
    canvas.popClip();
  }

  Color thumb = colors.accent;
  if (off) thumb = colors.disabled_text;
  else if (pressed()) thumb = colors.accent_active;
  else if (overField_) thumb = colors.accent_hover;
  const double size = tokens::slider_thumb_size;
  const double left = field.x + ratio() * (field.width - size);
  canvas.drawBox(Rect{left, field.y, size, size}, BoxStyle{.fill = thumb, .radius = size / 2.0});

  if (focusVisible()) canvas.strokeOutline(field, tokens::radius_sm, kFocusOffset, kFocusWidth, colors.focus_border);
}

bool Slider::onMouseDown(const MouseEvent& event) {
  if (event.button != MouseButton::Left || !fieldRect().contains(event.position)) return false;
  // 在轨道上按下：滑块立刻跳到指针位置，之后拖动跟随
  dragging_ = true;
  host()->captureMouse(this);
  change(valueAt(event.position.x));
  return true;
}

bool Slider::onMouseMove(const MouseEvent& event) {
  const bool over = fieldRect().contains(event.position);
  if (over != overField_) {
    overField_ = over;
    invalidate();
  }
  if (!dragging_) return false;
  change(valueAt(event.position.x));
  return true;
}

bool Slider::onMouseUp(const MouseEvent& event) {
  if (event.button != MouseButton::Left || !dragging_) return false;
  dragging_ = false;
  host()->releaseMouse(this);
  return true;
}

bool Slider::onKeyDown(const KeyEvent& event) {
  if (!enabled()) return false;
  const double page = std::max(step_, (max_ - min_) * kPageFraction);
  switch (event.virtualKey) {
    case kVirtualKeyRight:
    case kVirtualKeyUp:
      change(value_ + step_);
      return true;
    case kVirtualKeyLeft:
    case kVirtualKeyDown:
      change(value_ - step_);
      return true;
    case kVirtualKeyPageUp:
      change(value_ + page);
      return true;
    case kVirtualKeyPageDown:
      change(value_ - page);
      return true;
    case kVirtualKeyHome:
      change(min_);
      return true;
    case kVirtualKeyEnd:
      change(max_);
      return true;
    default:
      return false;
  }
}

void Slider::onStateChanged() {
  if (!hovered()) overField_ = false;
  if (!pressed() && dragging_ && host() != nullptr) {
    dragging_ = false;
    host()->releaseMouse(this);
  }
  Widget::onStateChanged();
}

}  // namespace rujian::ui
