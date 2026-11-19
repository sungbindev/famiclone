/*
 * ------------------------------------------------------------------------
 * 名称：button.cpp
 * 说明：按钮控件的实现，尺寸、颜色、圆角、焦点环都取自生成的令牌，取值与 ui-button.css 一致。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：焦点环对应 outline: 1px solid var(--focus-border)、outline-offset: 1px，圆角为按钮圆角加偏移再加线宽。
 * ------------------------------------------------------------------------
 */

#include "rujian/ui/button.hpp"

#include "base/theme-tokens.hpp"

namespace rujian::ui {

namespace {

constexpr int kVirtualKeyReturn = 0x0D;
constexpr int kVirtualKeySpace = 0x20;
constexpr double kFocusOffset = 1.0;
/** outline: 1px 在网页版里实测的线宽是 2/3 个逻辑像素（各缩放比例下的覆盖率都符合）。 */
constexpr double kFocusWidth = 2.0 / 3.0;
/** ui-button.css 没有设置行高，沿用页面的 normal。 */
constexpr double kNormalLineHeight = 0.0;

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

Size Button::measure(Size /*available*/) {
  double content = 0;
  if (icon_) content += tokens::btn_icon_size;
  if (!label_.empty() && host() != nullptr) {
    TextStyle style;
    style.fontSize = tokens::btn_font_size;
    style.fontWeight = static_cast<int>(tokens::btn_font_weight);
    style.lineHeight = kNormalLineHeight;
    if (icon_) content += tokens::icon_text_gap;
    content += host()->measureText(label_, style, 0).width;
  }
  return Size{content + 2 * tokens::btn_padding_x, tokens::btn_height};
}

void Button::paint(Canvas& canvas) {
  const auto& colors = themeTokens(host()->theme());
  Color background;
  Color foreground;
  if (!enabled()) {
    background = colors.disabled_bg;
    foreground = colors.disabled_text;
  } else if (variant_ == ButtonVariant::Primary) {
    foreground = colors.accent_foreground;
    background = pressed() ? colors.accent_active : (hovered() ? colors.accent_hover : colors.accent);
  } else {
    foreground = colors.button_secondary_text;
    background = pressed() ? colors.button_secondary_active_bg : (hovered() ? colors.button_secondary_hover_bg : colors.button_secondary_bg);
  }

  const Rect box = bounds();
  canvas.drawBox(box, BoxStyle{.fill = background, .radius = tokens::btn_radius});

  // 内容（图标加文字）作为一个整体在按钮里水平垂直居中
  TextStyle style;
  style.fontSize = tokens::btn_font_size;
  style.fontWeight = static_cast<int>(tokens::btn_font_weight);
  style.lineHeight = kNormalLineHeight;
  style.color = foreground;
  const double textWidth = label_.empty() ? 0 : canvas.measureText(label_, style, 0).width;
  double contentWidth = textWidth;
  if (icon_) contentWidth += tokens::btn_icon_size + (label_.empty() ? 0 : tokens::icon_text_gap);
  double x = box.x + (box.width - contentWidth) / 2.0;
  if (icon_) {
    canvas.drawIcon(*icon_, Rect{x, box.y + (box.height - tokens::btn_icon_size) / 2.0, tokens::btn_icon_size, tokens::btn_icon_size}, foreground, tokens::icon_stroke_width);
    x += tokens::btn_icon_size + tokens::icon_text_gap;
  }
  if (!label_.empty()) canvas.drawText(label_, Rect{x, box.y, textWidth + 1, box.height}, style, VerticalAlign::Center);

  if (focusVisible()) {
    // outline: 1px solid; outline-offset: 1px 在网页版里是沿“边框向外偏移 1px”的位置描一条线
    canvas.strokeRoundedRect(box.outset(Insets::all(kFocusOffset)), tokens::btn_radius + kFocusOffset, kFocusWidth, colors.focus_border);
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
