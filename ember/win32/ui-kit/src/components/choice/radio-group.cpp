/*
 * ------------------------------------------------------------------------
 * 名称：radio-group.cpp
 * 说明：分段单选组的实现，尺寸与颜色取自生成的令牌，取值与 ui-choice-controls.css 的 ui-choice-group--segmented 一致。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：选项底色的优先级与样式表一致：按下 > 悬停 > 选中；悬停与按下的规则只对可用选项生效，选中的底色、强调线、加粗对禁用的选项也生效。
 *       整组在圆角框内裁剪，焦点环也被裁掉框外的部分（样式表里容器是 overflow: hidden）。
 * ------------------------------------------------------------------------
 */

#include "rujian/ui/radio-group.hpp"

#include <algorithm>
#include <cmath>

#include "base/theme-tokens.hpp"

namespace rujian::ui {

namespace {

constexpr int kVirtualKeyReturn = 0x0D;
constexpr int kVirtualKeySpace = 0x20;
constexpr int kVirtualKeyLeft = 0x25;
constexpr int kVirtualKeyUp = 0x26;
constexpr int kVirtualKeyRight = 0x27;
constexpr int kVirtualKeyDown = 0x28;
/** 容器与选项之间分隔线的边框宽度。 */
constexpr double kBorderWidth = 1.0;
/** outline: 1px。 */
constexpr double kFocusWidth = 1.0;
/** outline-offset: 2px。 */
constexpr double kFocusOffset = 2.0;

}  // namespace

void RadioGroup::setOptions(std::vector<RadioOption> options) {
  const std::string current = value();
  options_ = std::move(options);
  selected_ = -1;
  for (std::size_t i = 0; i < options_.size(); ++i) {
    if (options_[i].value == current) selected_ = static_cast<int>(i);
  }
  hoverIndex_ = -1;
  pressIndex_ = -1;
  invalidate();
}

void RadioGroup::setValue(const std::string& value) {
  selected_ = -1;
  for (std::size_t i = 0; i < options_.size(); ++i) {
    if (options_[i].value == value) selected_ = static_cast<int>(i);
  }
  invalidate();
}

std::string RadioGroup::value() const { return selected_ >= 0 ? options_[static_cast<std::size_t>(selected_)].value : std::string(); }

bool RadioGroup::itemEnabled(int index) const { return enabled() && index >= 0 && static_cast<std::size_t>(index) < options_.size() && !options_[static_cast<std::size_t>(index)].disabled; }

int RadioGroup::nextEnabled(int from, int step) const {
  const int count = static_cast<int>(options_.size());
  for (int offset = 1; offset <= count; ++offset) {
    const int index = (((from + step * offset) % count) + count) % count;
    if (itemEnabled(index)) return index;
  }
  return -1;
}

bool RadioGroup::focusable() const { return firstEnabled() >= 0; }

CursorKind RadioGroup::cursor() const { return itemEnabled(hoverIndex_) ? CursorKind::Hand : CursorKind::Arrow; }

TextStyle RadioGroup::textStyle(bool selected, Color color) const {
  TextStyle style;
  style.fontSize = tokens::segmented_font_size;
  style.fontWeight = selected ? static_cast<int>(tokens::segmented_selected_font_weight) : 400;
  style.lineHeight = 0;
  style.color = color;
  return style;
}

Size RadioGroup::measure(Size available) {
  const double border = borderWidth();
  const double height = tokens::segmented_height + 2 * border;
  if (available.width > 0) return Size{available.width, height};
  if (host() == nullptr) return Size{0, height};
  // 没有给定宽度时按最宽的选项等分：文字宽度加左右内边距，非首项再加左边的分隔线
  double widest = 0;
  for (std::size_t i = 0; i < options_.size(); ++i) {
    const double text = host()->measureText(options_[i].label, textStyle(static_cast<int>(i) == selected_, kTransparent), 0).width;
    widest = std::max(widest, text + 2 * tokens::segmented_padding_x + (i > 0 ? border : 0.0));
  }
  return Size{widest * static_cast<double>(options_.size()) + 2 * border, height};
}

double RadioGroup::borderWidth() const { return snapBorderWidth(kBorderWidth, host() != nullptr ? host()->scale() : 1.0); }

Rect RadioGroup::itemRect(std::size_t index) const {
  const Rect inner = bounds().inset(Insets::all(borderWidth()));
  const double track = options_.empty() ? 0.0 : inner.width / static_cast<double>(options_.size());
  return Rect{inner.x + track * static_cast<double>(index), inner.y, track, inner.height};
}

int RadioGroup::indexAt(Point point) const {
  for (std::size_t i = 0; i < options_.size(); ++i) {
    if (itemRect(i).contains(point)) return static_cast<int>(i);
  }
  return -1;
}

void RadioGroup::paint(Canvas& canvas) {
  const auto& colors = themeTokens(host()->theme());
  const Rect box = bounds();
  canvas.drawBox(box, BoxStyle{.radius = tokens::radius_lg, .borderWidth = kBorderWidth, .borderColor = colors.segmented_border});

  canvas.pushClip(box.inset(Insets::all(borderWidth())), std::max(0.0, tokens::radius_lg - borderWidth()));
  for (std::size_t i = 0; i < options_.size(); ++i) {
    const int index = static_cast<int>(i);
    const bool selected = index == selected_;
    const bool available = itemEnabled(index);
    const Rect item = itemRect(i);

    Color background = kTransparent;
    if (available && pressIndex_ == index) background = colors.option_active_bg;
    else if (available && hoverIndex_ == index) background = colors.option_hover_bg;
    else if (selected) background = colors.choice_selected_bg;
    // :focus-visible 时选项带 border-radius: radius-sm，底色与强调线跟着圆角裁剪
    const bool ringed = focusVisible() && index == focusIndex_;
    if (ringed) canvas.pushClip(item, tokens::radius_sm);
    canvas.fillRect(item, background);

    // 非首项的左边框是分隔线，画在底色之上
    const double borderLeft = i > 0 ? borderWidth() : 0.0;
    if (i > 0) canvas.drawLine(Rect{item.x, item.y, borderLeft, item.height}, colors.divider);
    if (selected) {
      // 选中的强调线是 inset box-shadow：它所在的内边距框先对齐设备像素，线的厚度（2px）不取整，上沿因此可能只盖住半个像素
      const double scale = host()->scale();
      const double left = std::round((item.x + borderLeft) * scale) / scale;
      const double right = std::round(item.right() * scale) / scale;
      const double bottom = std::round(item.bottom() * scale) / scale;
      if (ringed) {
        // 获得焦点的选项有圆角，内阴影沿圆角的内边缘延伸，不再是一条平直的线
        const double top = std::round(item.y * scale) / scale;
        canvas.fillInsetShadow(Rect{left, top, right - left, bottom - top}, tokens::radius_sm, Insets{0, 0, 0, borderLeft}, Point{0, -tokens::segmented_indicator_height}, colors.accent);
      } else {
        canvas.fillRectSmooth(Rect{left, bottom - tokens::segmented_indicator_height, right - left, tokens::segmented_indicator_height}, colors.accent);
      }
    }
    if (ringed) canvas.popClip();

    // 文字在去掉左边框的内容区里居中
    const Rect content{item.x + borderLeft, item.y, item.width - borderLeft, item.height};
    const TextStyle style = textStyle(selected, itemEnabled(index) ? colors.text : colors.disabled_text);
    const double width = canvas.measureText(options_[i].label, style, 0).width;
    canvas.drawText(options_[i].label, Rect{content.x + (content.width - width) / 2.0, content.y, width + 1, content.height}, style, VerticalAlign::Center);
  }
  if (focusVisible() && focusIndex_ >= 0 && static_cast<std::size_t>(focusIndex_) < options_.size()) {
    // 焦点环画在所有选项之上，被容器的圆角框裁剪
    canvas.strokeOutline(itemRect(static_cast<std::size_t>(focusIndex_)), tokens::radius_sm, kFocusOffset, kFocusWidth, colors.focus_border);
  }
  canvas.popClip();
}

void RadioGroup::select(int index) {
  if (!itemEnabled(index)) return;
  focusIndex_ = index;
  if (selected_ != index) {
    selected_ = index;
    invalidate();
    if (onChange_) onChange_(options_[static_cast<std::size_t>(index)].value);
  } else {
    invalidate();
  }
}

bool RadioGroup::onMouseDown(const MouseEvent& event) {
  if (event.button != MouseButton::Left) return false;
  pressIndex_ = indexAt(event.position);
  hoverIndex_ = pressIndex_;
  invalidate();
  return true;
}

bool RadioGroup::onMouseUp(const MouseEvent& event) {
  if (event.button != MouseButton::Left) return false;
  const int pressed = pressIndex_;
  pressIndex_ = -1;
  invalidate();
  if (pressed >= 0 && pressed == indexAt(event.position)) select(pressed);
  return true;
}

bool RadioGroup::onMouseMove(const MouseEvent& event) {
  const int index = indexAt(event.position);
  if (index != hoverIndex_) {
    hoverIndex_ = index;
    invalidate();
  }
  return true;
}

bool RadioGroup::onKeyDown(const KeyEvent& event) {
  if (!enabled() || options_.empty()) return false;
  switch (event.virtualKey) {
    case kVirtualKeySpace:
    case kVirtualKeyReturn:
      select(focusIndex_);
      return true;
    case kVirtualKeyUp:
    case kVirtualKeyLeft:
    case kVirtualKeyDown:
    case kVirtualKeyRight: {
      const int step = (event.virtualKey == kVirtualKeyUp || event.virtualKey == kVirtualKeyLeft) ? -1 : 1;
      const int next = nextEnabled(focusIndex_, step);
      if (next >= 0) select(next);
      return true;
    }
    default:
      return false;
  }
}

void RadioGroup::onStateChanged() {
  if (!wasFocused_ && focused()) focusIndex_ = selected_ >= 0 ? selected_ : firstEnabled();
  wasFocused_ = focused();
  if (!hovered()) hoverIndex_ = -1;
  if (!pressed()) pressIndex_ = -1;
  Widget::onStateChanged();
}

}  // namespace rujian::ui
