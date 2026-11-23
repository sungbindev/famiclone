/*
 * ------------------------------------------------------------------------
 * 名称：status-bar-item.cpp
 * 说明：状态项的实现：状态点、图标（含画在图标里的指示灯）、文字、文字后的指示灯依次排列，按钮状态项有悬停、按下、按下态与键盘操作。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：各部分之间留 4 像素；颜色规则与网页版一致：按钮状态项悬停、按下、选中用无底色按钮的颜色，
 *       success 等级的文字颜色盖过它们，warning 与 error 只改图标颜色。
 * ------------------------------------------------------------------------
 */

#include <algorithm>

#include "base/theme-tokens.hpp"
#include "base/unicode.hpp"
#include "rujian/ui/status-bar.hpp"

namespace rujian::ui {

namespace {

constexpr int kVirtualKeyReturn = 0x0D;
constexpr int kVirtualKeySpace = 0x20;
/** 文字超过这个字数才参与收缩。 */
constexpr std::size_t kShrinkableTextLength = 16;
/** 次要项在二次收缩里的权重倍数：放不下时先收缩次要项。 */
constexpr double kSecondaryShrinkBoost = 4.0;
/** 画在图标里的灯的直径占图标边长的比例。 */
constexpr double kInIconLampRatio = 0.3;
/** 次要项前面的分隔圆点的直径。 */
constexpr double kSeparatorDotSize = 4.0;
/** 次要项的文字色在“弱化色”上再叠的不透明度。 */
constexpr double kSecondaryOpacity = 0.7;

}  // namespace

StatusBarItem::StatusBarItem(std::string text) : text_(std::move(text)) {}

void StatusBarItem::setText(std::string text) {
  text_ = std::move(text);
  invalidate();
}

void StatusBarItem::setIcon(std::optional<IconId> icon) {
  icon_ = icon;
  invalidate();
}

void StatusBarItem::setTone(StatusItemTone tone) {
  tone_ = tone;
  invalidate();
}

void StatusBarItem::setDot(StatusDot dot) {
  dot_ = dot;
  invalidate();
}

void StatusBarItem::setLamp(std::optional<StatusLamp> lamp) {
  lamp_ = std::move(lamp);
  invalidate();
}

void StatusBarItem::setPressed(bool pressed) {
  pressedFlag_ = pressed;
  invalidate();
}

void StatusBarItem::setSecondary(bool secondary) {
  secondary_ = secondary;
  invalidate();
}

double StatusBarItem::leadingWidth() const {
  // 次要项没有左边距，前面是分隔圆点；圆点与后面文字的间距，等于它与前一项文字的间距（前一项的右边距加项之间的间隔）
  if (secondary_) return kSeparatorDotSize + paddingX() + tokens::icon_text_gap;
  return paddingX();
}

void StatusBarItem::setIconOnly(bool iconOnly) {
  iconOnly_ = iconOnly;
  invalidate();
}

void StatusBarItem::setOnClick(std::function<void()> handler) {
  onClick_ = std::move(handler);
  invalidate();
}

void StatusBarItem::setLarge(bool large) {
  large_ = large;
  invalidate();
}

double StatusBarItem::shrinkWeight() const {
  if (iconOnly_) return 0.0;
  const std::size_t length = toUtf16(text_).size();
  return length > kShrinkableTextLength ? static_cast<double>(length) * static_cast<double>(length) : 0.0;
}

double StatusBarItem::forcedShrinkWeight() const {
  if (iconOnly_) return 0.0;
  const double length = static_cast<double>(toUtf16(text_).size());
  // 次要项先收缩
  return length * length * (secondary_ ? kSecondaryShrinkBoost : 1.0);
}

double StatusBarItem::textShrinkRoom() const { return iconOnly_ || host() == nullptr ? 0.0 : textWidth(); }

std::optional<IconId> StatusBarItem::shownIcon() const {
  if (dot_ != StatusDot::None) return std::nullopt;
  if (icon_) return icon_;
  if (tone_ == StatusItemTone::Warning) return IconId::AlertTriangle;
  if (tone_ == StatusItemTone::Error) return IconId::AlertCircle;
  return std::nullopt;
}

bool StatusBarItem::lampInIcon() const { return lamp_ && (lamp_->inIcon || lamp_->onIconAccent) && shownIcon(); }

double StatusBarItem::iconSize() const { return large_ ? tokens::status_bar_large_icon_size : tokens::status_bar_icon_size; }

double StatusBarItem::dotSize() const { return large_ ? tokens::status_bar_large_dot_size : tokens::status_bar_dot_size; }

double StatusBarItem::paddingX() const { return large_ ? tokens::status_bar_large_item_padding_x : tokens::status_bar_item_padding_x; }

TextStyle StatusBarItem::textStyle(Color color) const {
  TextStyle style;
  style.fontSize = large_ ? tokens::status_bar_large_font_size : tokens::status_bar_font_size;
  style.fontWeight = 400;
  style.lineHeight = 0;
  style.color = color;
  style.overflow = TextOverflow::Ellipsis;
  return style;
}

double StatusBarItem::textWidth() const { return host()->measureText(text_, textStyle(kTransparent), 0).width; }

Size StatusBarItem::measure(Size /*available*/) {
  if (host() == nullptr) return Size{};
  const double height = large_ ? tokens::status_bar_large_item_height : tokens::status_bar_item_height;
  double width = leadingWidth() + paddingX();
  int parts = 0;
  const auto add = [&](double size) {
    width += size;
    ++parts;
  };
  if (dot_ != StatusDot::None) add(dotSize());
  if (shownIcon()) add(iconSize());
  if (!iconOnly_) add(textWidth());
  if (lamp_ && !lampInIcon()) add(large_ ? tokens::status_bar_large_lamp_width : tokens::status_bar_lamp_width);
  if (parts > 1) width += (parts - 1) * tokens::space_xs;
  return Size{width, height};
}

void StatusBarItem::paint(Canvas& canvas) {
  const auto& colors = themeTokens(host()->theme());
  const Rect box = bounds();
  const bool button = clickable();
  const bool active = button && enabled() && pressed();
  const bool hover = button && enabled() && hovered();
  const bool selected = button && pressedFlag_;

  Color background = kTransparent;
  Color foreground = secondary_ ? colors.text_muted.withOpacity(kSecondaryOpacity) : colors.chrome_text;
  if (selected) {
    background = colors.ghost_selected_background;
    foreground = colors.ghost_selected_foreground;
  } else if (active) {
    background = colors.ghost_active_background;
    foreground = colors.ghost_active_foreground;
  } else if (hover) {
    background = colors.ghost_hover_background;
    foreground = colors.ghost_hover_foreground;
  }
  if (tone_ == StatusItemTone::Success) foreground = colors.status_success;
  if (background.a() != 0) canvas.drawBox(box, BoxStyle{.fill = background, .radius = tokens::radius_md});

  const double lampWidth = large_ ? tokens::status_bar_large_lamp_width : tokens::status_bar_lamp_width;
  const double lampHeight = large_ ? tokens::status_bar_large_lamp_height : tokens::status_bar_lamp_height;
  const auto icon = shownIcon();
  const bool plainLamp = lamp_ && !lampInIcon();
  // 文字之外的部分占的宽度，剩下的给文字（放不下时文字以省略号结尾）
  double fixed = leadingWidth() + paddingX();
  int parts = 0;
  const auto count = [&](double size) {
    fixed += size;
    ++parts;
  };
  if (dot_ != StatusDot::None) count(dotSize());
  if (icon) count(iconSize());
  if (!iconOnly_) ++parts;
  if (plainLamp) count(lampWidth);
  if (parts > 1) fixed += (parts - 1) * tokens::space_xs;
  const double available = std::max(0.0, box.width - fixed);

  double x = box.x;
  if (secondary_) {
    canvas.drawBox(Rect{x, box.y + (box.height - kSeparatorDotSize) / 2.0, kSeparatorDotSize, kSeparatorDotSize}, BoxStyle{.fill = foreground, .radius = kSeparatorDotSize / 2.0});
  }
  x += leadingWidth();
  if (dot_ != StatusDot::None) {
    Color dotColor = colors.text_muted;
    if (dot_ == StatusDot::Success) dotColor = colors.status_success;
    else if (dot_ == StatusDot::Warning) dotColor = colors.status_warning;
    const double size = dotSize();
    canvas.drawBox(Rect{x, box.y + (box.height - size) / 2.0, size, size}, BoxStyle{.fill = dotColor, .radius = size / 2.0});
    x += size + tokens::space_xs;
  }
  if (icon) {
    const double size = iconSize();
    Color iconColor = foreground;
    if (tone_ == StatusItemTone::Warning) iconColor = colors.status_warning;
    else if (tone_ == StatusItemTone::Error) iconColor = colors.status_error;
    const Rect iconBox{x, box.y + (box.height - size) / 2.0, size, size};
    Color accent = kTransparent;
    if (lamp_ && lamp_->onIconAccent) accent = lamp_->lit ? colors.status_bar_lamp_on : colors.status_bar_lamp_off;
    else if (tone_ == StatusItemTone::Success) accent = colors.status_success;
    canvas.drawIcon(*icon, iconBox, iconColor, tokens::icon_stroke_width, accent);
    if (lampInIcon() && lamp_->inIcon) {
      const double diameter = size * kInIconLampRatio;
      const double cx = iconBox.x + size * lamp_->inIcon->x / 100.0;
      const double cy = iconBox.y + size * lamp_->inIcon->y / 100.0;
      canvas.drawBox(Rect{cx - diameter / 2.0, cy - diameter / 2.0, diameter, diameter}, BoxStyle{.fill = lamp_->lit ? colors.status_bar_lamp_on : colors.status_bar_lamp_off, .radius = diameter / 2.0});
    }
    x += size + tokens::space_xs;
  }
  if (!iconOnly_) {
    const double width = std::min(available, textWidth());
    // 宽度为 0 的矩形表示不限宽，没有空间时不画文字，否则整段文字会画出去压到右边的项
    if (width > 0.0) canvas.drawText(text_, Rect{x, box.y, width + (width < available ? 1.0 : 0.0), box.height}, textStyle(foreground), VerticalAlign::Center);
    x += width + tokens::space_xs;
  }
  if (plainLamp) {
    canvas.drawBox(Rect{x, box.y + (box.height - lampHeight) / 2.0, lampWidth, lampHeight}, BoxStyle{.fill = lamp_->lit ? colors.status_bar_lamp_on : colors.status_bar_lamp_off, .radius = tokens::radius_sm});
  }
}

void StatusBarItem::click() {
  if (onClick_) onClick_();
}

bool StatusBarItem::onMouseDown(const MouseEvent& event) { return clickable() && event.button == MouseButton::Left; }

bool StatusBarItem::onMouseUp(const MouseEvent& event) {
  if (!clickable() || event.button != MouseButton::Left) return false;
  if (pressed() && bounds().contains(event.position)) click();
  return true;
}

bool StatusBarItem::onKeyDown(const KeyEvent& event) {
  if (!clickable()) return false;
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

bool StatusBarItem::onKeyUp(const KeyEvent& event) {
  if (!clickable() || event.virtualKey != kVirtualKeySpace) return false;
  if (spaceDown_) {
    spaceDown_ = false;
    click();
  }
  return true;
}

void StatusBarItem::onStateChanged() {
  if (!focused()) spaceDown_ = false;
  Widget::onStateChanged();
}

}  // namespace rujian::ui
