/*
 * ------------------------------------------------------------------------
 * 名称：checkbox.cpp
 * 说明：复选框控件的实现，尺寸、颜色、圆角、焦点环都取自生成的令牌，取值与 ui-choice-controls.css 一致。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：方框的状态优先级与样式表一致：禁用 > 已勾选（强调色随悬停、按下加深）> 按下 > 悬停 > 常态；
 *       对勾是样式表里“右边与下边各 2px 的边框、旋转 45 度”的方块，这里按同样的几何用两条线段画出。
 * ------------------------------------------------------------------------
 */

#include "rujian/ui/checkbox.hpp"

#include <algorithm>
#include <cmath>

#include "base/theme-tokens.hpp"

namespace rujian::ui {

namespace {

constexpr int kVirtualKeySpace = 0x20;
/** outline: 1px。 */
constexpr double kFocusWidth = 1.0;
/** outline-offset: 2px。 */
constexpr double kFocusOffset = 2.0;
/** 方框的边框宽度。 */
constexpr double kMarkBorder = 1.0;
/** 对勾：未旋转时是 5×9 的方块，左边缘距方框内侧 3px，右边与下边的边框各 2px。 */
constexpr double kCheckLeft = 3.0;
constexpr double kCheckWidth = 5.0;
constexpr double kCheckHeight = 9.0;
constexpr double kCheckStroke = 2.0;

}  // namespace

Checkbox::Checkbox(std::string label) : label_(std::move(label)) {}

void Checkbox::setLabel(std::string label) {
  label_ = std::move(label);
  invalidate();
}

void Checkbox::setChecked(bool checked) {
  if (checked_ == checked) return;
  checked_ = checked;
  invalidate();
}

TextStyle Checkbox::textStyle(Color color) const {
  TextStyle style;
  style.fontSize = tokens::font_size_body;
  style.fontWeight = 400;
  style.lineHeight = 0;
  style.color = color;
  return style;
}

Size Checkbox::measure(Size /*available*/) {
  if (host() == nullptr) return Size{};
  const TextMetrics text = host()->measureText(label_, textStyle(kTransparent), 0);
  const double gap = label_.empty() ? 0.0 : tokens::choice_gap;
  return Size{tokens::choice_mark_size + gap + text.width, std::max(tokens::choice_min_height, text.height)};
}

void Checkbox::paint(Canvas& canvas) {
  const auto& colors = themeTokens(host()->theme());
  const Rect box = bounds();
  const bool off = !enabled();
  const Rect mark{box.x, box.y + (box.height - tokens::choice_mark_size) / 2.0, tokens::choice_mark_size, tokens::choice_mark_size};

  // 方框：填充色与边框色
  Color fill = colors.choice_mark_bg;
  Color border = colors.choice_mark_border;
  if (off) {
    fill = checked_ ? colors.disabled_text : colors.disabled_bg;
    border = checked_ ? colors.disabled_text : colors.disabled_border;
  } else if (checked_) {
    fill = pressed() ? colors.accent_active : (hovered() ? colors.accent_hover : colors.accent);
    border = fill;
  } else if (pressed()) {
    fill = colors.choice_mark_active_bg;
    border = colors.choice_mark_border_active;
  } else if (hovered()) {
    fill = colors.choice_mark_hover_bg;
    border = colors.choice_mark_border_hover;
  }
  canvas.drawBox(mark, BoxStyle{.fill = fill, .radius = tokens::radius_sm, .borderWidth = kMarkBorder, .borderColor = border});

  if (checked_) {
    // 对勾的方块以方框的内侧（去掉边框）为参照，左上角在内侧左上角右移 3px
    const double markBorder = snapBorderWidth(kMarkBorder, host()->scale());
    canvas.fillRotatedCorner(mark.x + markBorder + kCheckLeft, mark.y + markBorder, kCheckWidth, kCheckHeight, kCheckStroke, 45.0, 0.0, colors.accent_foreground);
  }

  if (!label_.empty()) {
    const Rect textBox{box.x + tokens::choice_mark_size + tokens::choice_gap, box.y, std::max(0.0, box.width - tokens::choice_mark_size - tokens::choice_gap), box.height};
    canvas.drawText(label_, textBox, textStyle(off ? colors.disabled_text : colors.text), VerticalAlign::Center);
  }

  if (focusVisible()) {
    // outline: 1px solid; outline-offset: 2px，圆角跟着元素的 border-radius（radius-sm）
    canvas.strokeOutline(box, tokens::radius_sm, kFocusOffset, kFocusWidth, colors.focus_border);
  }
}

void Checkbox::toggle() {
  if (!enabled()) return;
  checked_ = !checked_;
  invalidate();
  if (onChange_) onChange_(checked_);
}

bool Checkbox::onMouseDown(const MouseEvent& event) { return event.button == MouseButton::Left; }

bool Checkbox::onMouseUp(const MouseEvent& event) {
  if (event.button != MouseButton::Left) return false;
  if (pressed() && bounds().contains(event.position)) toggle();
  return true;
}

bool Checkbox::onKeyDown(const KeyEvent& event) {
  if (event.virtualKey != kVirtualKeySpace) return false;
  toggle();
  return true;
}

}  // namespace rujian::ui
