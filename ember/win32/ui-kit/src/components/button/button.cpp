/*
 * ------------------------------------------------------------------------
 * 名称：button.cpp
 * 说明：按钮控件的实现，尺寸、颜色、圆角、焦点环都取自生成的令牌，取值与 ui-button.css 一致。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：状态优先级与样式表一致：禁用 > 无底色的选中 > 切换按下 > 按下 > 悬停 > 常态；链接样式没有背景，悬停与按下时加下划线。
 * ------------------------------------------------------------------------
 */

#include "rujian/ui/button.hpp"

#include <algorithm>

#include "base/icon-rules.hpp"
#include "base/theme-tokens.hpp"

namespace rujian::ui {

namespace {

constexpr int kVirtualKeyReturn = 0x0D;
constexpr int kVirtualKeySpace = 0x20;
constexpr double kFocusOffset = 1.0;
/** outline: 1px。 */
constexpr double kFocusWidth = 1.0;
/** ui-button.css 没有设置行高，沿用页面的 normal。 */
constexpr double kNormalLineHeight = 0.0;

/** 按钮当前状态下的颜色。 */
struct Look {
  Color background;
  Color foreground;
};

}  // namespace

Button::Button(std::string label) : label_(std::move(label)) {}

void Button::setLabel(std::string label) {
  label_ = std::move(label);
  invalidate();
}

void Button::setVariant(ButtonVariant variant) {
  variant_ = variant;
  invalidate();
}

void Button::setIcon(std::optional<IconId> icon) {
  icon_ = icon;
  invalidate();
}

void Button::setAutoIcon(bool enabled) {
  autoIcon_ = enabled;
  invalidate();
}

void Button::setIconAtEnd(std::optional<bool> atEnd) {
  iconAtEnd_ = atEnd;
  invalidate();
}

void Button::setIconOnly(bool iconOnly) {
  iconOnly_ = iconOnly;
  invalidate();
}

void Button::setCompact(bool compact) {
  compact_ = compact;
  invalidate();
}

void Button::setToggled(std::optional<bool> toggled) {
  toggled_ = toggled;
  invalidate();
}

void Button::setFrameStyle(bool frame) {
  frame_ = frame;
  invalidate();
}

void Button::setMinWidth(double minWidth) {
  minWidth_ = minWidth;
  invalidate();
}

std::optional<Button::ResolvedIcon> Button::resolveIcon() const {
  if (variant_ == ButtonVariant::Link) return std::nullopt;
  if (icon_) return ResolvedIcon{*icon_, iconAtEnd_.value_or(false)};
  if (!autoIcon_) return std::nullopt;
  const auto matched = iconForLabel(label_);
  if (!matched) return std::nullopt;
  return ResolvedIcon{matched->icon, iconAtEnd_.value_or(matched->atEnd)};
}

TextStyle Button::textStyle(Color color) const {
  TextStyle style;
  style.fontSize = tokens::btn_font_size;
  style.fontWeight = static_cast<int>(tokens::btn_font_weight);
  style.lineHeight = kNormalLineHeight;
  style.color = color;
  if (variant_ == ButtonVariant::Link) {
    style.wrap = true;
    style.underline = hovered() || pressed();
  }
  return style;
}

Size Button::measure(Size available) {
  if (host() == nullptr) return Size{};
  if (variant_ == ButtonVariant::Link) {
    const TextMetrics text = host()->measureText(label_, textStyle(kTransparent), available.width);
    return Size{text.width, text.height};
  }
  const auto icon = resolveIcon();
  if (iconOnly_ && icon) return Size{tokens::btn_height, tokens::btn_height};
  double content = icon ? tokens::btn_icon_size : 0.0;
  if (!label_.empty()) {
    if (icon) content += tokens::icon_text_gap;
    content += host()->measureText(label_, textStyle(kTransparent), 0).width;
  }
  const double minimum = std::max(compact_ ? tokens::btn_compact_min_width : 0.0, minWidth_);
  return Size{std::max(minimum, content + 2 * tokens::btn_padding_x), tokens::btn_height};
}

void Button::paint(Canvas& canvas) {
  const auto& colors = themeTokens(host()->theme());
  Look look{kTransparent, colors.button_secondary_text};
  const bool ghost = variant_ == ButtonVariant::Ghost;
  if (!enabled()) {
    look = Look{colors.disabled_bg, colors.disabled_text};
  } else {
    switch (variant_) {
      case ButtonVariant::Secondary:
        look = Look{pressed() ? colors.button_secondary_active_bg : (hovered() ? colors.button_secondary_hover_bg : colors.button_secondary_bg), colors.button_secondary_text};
        break;
      case ButtonVariant::Primary:
        look = Look{pressed() ? colors.accent_active : (hovered() ? colors.accent_hover : colors.accent), colors.accent_foreground};
        break;
      case ButtonVariant::Danger:
        look = Look{pressed() ? colors.button_danger_active_bg : (hovered() ? colors.button_danger_hover_bg : colors.button_danger_bg), colors.button_danger_text};
        break;
      case ButtonVariant::Ghost:
        look = Look{pressed() ? colors.ghost_active_background : (hovered() ? colors.ghost_hover_background : kTransparent),
                    pressed() ? colors.ghost_active_foreground : (hovered() ? colors.ghost_hover_foreground : colors.ghost_foreground)};
        break;
      case ButtonVariant::Link:
        look = Look{kTransparent, colors.link};
        break;
    }
    if (toggled_.value_or(false) && variant_ != ButtonVariant::Link) {
      if (ghost) look = Look{colors.ghost_selected_background, colors.ghost_selected_foreground};
      else look.background = pressed() ? colors.option_selected_active_bg : (hovered() ? colors.option_selected_hover_bg : colors.option_selected_bg);
    }
  }

  const Rect box = bounds();
  const TextStyle style = textStyle(look.foreground);
  double focusRadius = 0;
  if (variant_ == ButtonVariant::Link) {
    canvas.drawText(label_, box, style, VerticalAlign::Top);
  } else {
    canvas.drawBox(box, BoxStyle{.fill = look.background, .radius = frame_ ? 0.0 : tokens::btn_radius});
    focusRadius = tokens::btn_radius;
    // 内容（图标加文字）作为一个整体在按钮里水平垂直居中
    const auto icon = resolveIcon();
    const bool onlyIcon = iconOnly_ && icon;
    const double textWidth = (label_.empty() || onlyIcon) ? 0.0 : canvas.measureText(label_, style, 0).width;
    double contentWidth = textWidth;
    if (icon) contentWidth += tokens::btn_icon_size + (textWidth > 0 ? tokens::icon_text_gap : 0.0);
    double x = box.x + (box.width - contentWidth) / 2.0;
    const Rect iconRect{0, box.y + (box.height - tokens::btn_icon_size) / 2.0, tokens::btn_icon_size, tokens::btn_icon_size};
    if (icon && !icon->atEnd) {
      canvas.drawIcon(icon->icon, Rect{x, iconRect.y, iconRect.width, iconRect.height}, look.foreground, tokens::icon_stroke_width);
      x += tokens::btn_icon_size + (textWidth > 0 ? tokens::icon_text_gap : 0.0);
    }
    if (textWidth > 0) canvas.drawText(label_, Rect{x, box.y, textWidth + 1, box.height}, style, VerticalAlign::Center);
    if (icon && icon->atEnd) canvas.drawIcon(icon->icon, Rect{x + textWidth + tokens::icon_text_gap, iconRect.y, iconRect.width, iconRect.height}, look.foreground, tokens::icon_stroke_width);
  }

  if (focusVisible() && !frame_) {
    // outline: 1px solid; outline-offset: 1px
    canvas.strokeOutline(box, focusRadius, kFocusOffset, kFocusWidth, colors.focus_border);
  }
}

void Button::click() {
  if (onClick_) onClick_();
}

bool Button::onMouseDown(const MouseEvent& event) { return event.button == MouseButton::Left; }

bool Button::onMouseUp(const MouseEvent& event) {
  if (event.button != MouseButton::Left) return false;
  if (pressed() && bounds().contains(event.position)) click();
  return true;
}

bool Button::onKeyDown(const KeyEvent& event) {
  if (event.virtualKey == kVirtualKeyReturn) {
    if (!event.repeat) click();
    return true;
  }
  if (event.virtualKey == kVirtualKeySpace) {
    spaceDown_ = true;
    return true;
  }
  return false;
}

bool Button::onKeyUp(const KeyEvent& event) {
  if (event.virtualKey != kVirtualKeySpace) return false;
  if (spaceDown_) {
    spaceDown_ = false;
    click();
  }
  return true;
}

void Button::onStateChanged() {
  if (!focused()) spaceDown_ = false;
  Widget::onStateChanged();
}

}  // namespace rujian::ui
