/*
 * ------------------------------------------------------------------------
 * 名称：pad-diagram.cpp
 * 说明：手柄图的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-20
 * 备注：键按钮的尺寸与取色同 ember-input.css：圆角取 radius-lg（圆形键是半宽）、内边距左右 4、键名 10 号常规弱化色、绑定 11 号半粗（没有绑定时常规弱化色）、两行行高都是字号的 1 倍、行间 2，
 *       按钮里的两行文字整体上下居中；底色与状态同次要按钮，选中同列表选中项，等待按键是强调色的边框外加一圈同色的 1 像素外框，焦点是 1 像素的焦点环、偏移 2。
 * ------------------------------------------------------------------------
 */

#include "rujian/ui/pad-diagram.hpp"

#include <algorithm>

#include "base/theme-tokens.hpp"

namespace rujian::ui {

namespace {

constexpr int kVirtualKeyReturn = 0x0D;
constexpr int kVirtualKeySpace = 0x20;
/** 手柄图的最大宽度。 */
constexpr double kMaximumWidth = 560.0;
/** 键名的字号（组件库的 --font-size-micro）与绑定文字的字重（--font-weight-strong）。 */
constexpr double kRoleFontSize = 10.0;
constexpr int kStrongWeight = 600;
/** 焦点环：outline 1px，outline-offset 2px。 */
constexpr double kFocusWidth = 1.0;
constexpr double kFocusOffset = 2.0;

}  // namespace

PadKeyButton::PadKeyButton(std::string id, bool round) : id_(std::move(id)), round_(round) {}

void PadKeyButton::setContent(const PadHotspot& hotspot) {
  hotspot_ = hotspot;
  hotspot_.id = id_;
  setTooltip(hotspot_.tooltip);
  invalidate();
}

void PadKeyButton::paint(Canvas& canvas) {
  const auto& colors = themeTokens(host()->theme());
  const Rect box = bounds();
  const bool off = !enabled();
  const double radius = round_ ? std::min(box.width, box.height) / 2.0 : tokens::radius_lg;

  Color fill = pressed() ? colors.button_secondary_active_bg : (hovered() ? colors.button_secondary_hover_bg : colors.button_secondary_bg);
  if (hotspot_.selected) fill = pressed() ? colors.option_selected_active_bg : (hovered() ? colors.option_selected_hover_bg : colors.option_selected_bg);
  Color border = colors.input_border;
  if (hotspot_.learning) border = colors.accent;
  if (off) {
    fill = colors.disabled_bg;
    border = colors.disabled_border;
  }
  canvas.drawBox(box, BoxStyle{.fill = fill, .radius = radius, .borderWidth = tokens::space_hairline, .borderColor = border});
  // 等待按键：边框之外再有一圈同色的 1 像素外框（box-shadow: 0 0 0 1px）
  if (hotspot_.learning && !off) canvas.strokeOutline(box, radius, 0.0, tokens::space_hairline, colors.accent);

  // 两行文字整体上下居中：键名在上，绑定在下
  TextStyle roleStyle;
  roleStyle.fontSize = kRoleFontSize;
  roleStyle.lineHeight = 1.0;
  roleStyle.align = TextAlign::Center;
  roleStyle.overflow = TextOverflow::Ellipsis;
  roleStyle.color = off ? colors.disabled_text : colors.text_muted;
  TextStyle bindingStyle;
  bindingStyle.fontSize = tokens::font_size_caption;
  bindingStyle.lineHeight = 1.0;
  bindingStyle.align = TextAlign::Center;
  bindingStyle.overflow = TextOverflow::Ellipsis;
  bindingStyle.fontWeight = hotspot_.unbound ? 400 : kStrongWeight;
  bindingStyle.color = off ? colors.disabled_text : (hotspot_.unbound ? colors.text_muted : colors.text);
  const double roleHeight = roleStyle.fontSize * roleStyle.lineHeight;
  const double bindingHeight = bindingStyle.fontSize * bindingStyle.lineHeight;
  const double total = roleHeight + tokens::space_xxs + bindingHeight;
  const double top = box.y + (box.height - total) / 2.0;
  const double left = box.x + tokens::space_xs;
  const double width = std::max(0.0, box.width - 2 * tokens::space_xs);
  canvas.drawText(hotspot_.role, Rect{left, top, width, roleHeight}, roleStyle, VerticalAlign::Top);
  canvas.drawText(hotspot_.current, Rect{left, top + roleHeight + tokens::space_xxs, width, bindingHeight}, bindingStyle, VerticalAlign::Top);

  if (focusVisible()) canvas.strokeOutline(box, radius, kFocusOffset, kFocusWidth, colors.focus_border);
}

void PadKeyButton::pick() {
  if (enabled() && onPick_) onPick_(id_);
}

bool PadKeyButton::onMouseDown(const MouseEvent& event) { return event.button == MouseButton::Left; }

bool PadKeyButton::onMouseUp(const MouseEvent& event) {
  if (event.button != MouseButton::Left) return false;
  if (pressed() && bounds().contains(event.position)) pick();
  return true;
}

bool PadKeyButton::onKeyDown(const KeyEvent& event) {
  if (event.virtualKey == kVirtualKeyReturn) {
    if (!event.repeat) pick();
    return true;
  }
  if (event.virtualKey == kVirtualKeySpace) {
    spaceDown_ = true;
    return true;
  }
  return false;
}

bool PadKeyButton::onKeyUp(const KeyEvent& event) {
  if (event.virtualKey != kVirtualKeySpace) return false;
  if (spaceDown_) {
    spaceDown_ = false;
    pick();
  }
  return true;
}

void PadKeyButton::onStateChanged() {
  if (!focused()) spaceDown_ = false;
  Widget::onStateChanged();
}

PadDiagram::PadDiagram(PadDiagramLayout layout) : layout_(std::move(layout)) {
  for (const PadKeyShape& shape : layout_.keys) keys_.push_back(static_cast<PadKeyButton*>(addChild(std::make_unique<PadKeyButton>(shape.id, shape.round))));
}

void PadDiagram::setHotspots(const std::vector<PadHotspot>& hotspots) {
  for (const PadHotspot& hotspot : hotspots) {
    if (PadKeyButton* button = key(hotspot.id)) button->setContent(hotspot);
  }
}

void PadDiagram::setDisabled(bool disabled) {
  disabled_ = disabled;
  for (PadKeyButton* button : keys_) button->setEnabled(!disabled);
}

void PadDiagram::setOnPick(std::function<void(const std::string&)> handler) {
  for (PadKeyButton* button : keys_) button->setPickHandler(handler);
}

PadKeyButton* PadDiagram::key(const std::string& id) const {
  const auto found = std::find_if(keys_.begin(), keys_.end(), [&id](const PadKeyButton* button) { return button->id() == id; });
  return found == keys_.end() ? nullptr : *found;
}

Size PadDiagram::measure(Size available) {
  const double width = available.width > 0 ? std::min(available.width, kMaximumWidth) : kMaximumWidth;
  return Size{available.width > 0 ? available.width : width, width * layout_.canvasHeight / layout_.canvasWidth};
}

Rect PadDiagram::drawingRect(const Rect& bounds) const {
  const double width = std::min(bounds.width, kMaximumWidth);
  return Rect{bounds.x + (bounds.width - width) / 2.0, bounds.y, width, width * layout_.canvasHeight / layout_.canvasWidth};
}

void PadDiagram::arrange(const Rect& bounds) {
  setBounds(bounds);
  const Rect drawing = drawingRect(bounds);
  const double scale = drawing.width / layout_.canvasWidth;
  for (std::size_t i = 0; i < layout_.keys.size(); ++i) {
    const PadKeyShape& shape = layout_.keys[i];
    const double width = shape.width * scale;
    const double height = shape.round ? width : shape.height * scale;
    keys_[i]->arrange(Rect{drawing.x + shape.cx * scale - width / 2.0, drawing.y + shape.cy * scale - height / 2.0, width, height});
  }
}

void PadDiagram::paint(Canvas& canvas) {
  const auto& colors = themeTokens(host()->theme());
  const Rect drawing = drawingRect(bounds());
  const double scale = drawing.width / layout_.canvasWidth;
  const Rect body{drawing.x + layout_.body.x * scale, drawing.y + layout_.body.y * scale, layout_.body.width * scale, layout_.body.height * scale};
  canvas.drawBox(body, BoxStyle{.fill = colors.input_bg, .radius = layout_.bodyRadius * scale, .borderWidth = tokens::space_hairline, .borderColor = colors.input_border});
  const Rect cross{drawing.x + layout_.crossCenter.x * scale, drawing.y + layout_.crossCenter.y * scale, layout_.crossCenter.width * scale, layout_.crossCenter.height * scale};
  canvas.fillRect(cross, colors.button_secondary_bg);
}

}  // namespace rujian::ui
