/*
 * ------------------------------------------------------------------------
 * 名称：tree.cpp
 * 说明：树的实现：每行高 22，左右内边距 4，行首是 14 宽的展开箭头位置（扁平条目时空着），名称单行用省略号；悬停、按下、选中各有底色，焦点行画向内的焦点框。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：点击在同一行内按下并松开才选中；方向键、Home、End 只移动焦点，回车与空格选中焦点行。
 * ------------------------------------------------------------------------
 */

#include "rujian/ui/tree.hpp"

#include <algorithm>

#include "base/theme-tokens.hpp"

namespace rujian::ui {

namespace {

constexpr int kVirtualKeyEnd = 0x23;
constexpr int kVirtualKeyHome = 0x24;
constexpr int kVirtualKeyUp = 0x26;
constexpr int kVirtualKeyDown = 0x28;
constexpr int kVirtualKeyReturn = 0x0D;
constexpr int kVirtualKeySpace = 0x20;
constexpr double kFocusOffset = -1.0;
constexpr double kFocusWidth = 1.0;

/** 名称起点距行左边的距离：左内边距加展开箭头位置加图标间距。 */
constexpr double kLabelOffset = tokens::tree_padding_x + tokens::icon_size + tokens::icon_text_gap;

}  // namespace

void Tree::setItems(std::vector<TreeItem> items) {
  items_ = std::move(items);
  focusRow_ = -1;
  hoverRow_ = -1;
  pressedRow_ = -1;
  invalidate();
}

void Tree::setSelectedId(std::string id) {
  selectedId_ = std::move(id);
  invalidate();
}

void Tree::setOnSelect(SelectHandler handler) { onSelect_ = std::move(handler); }

Size Tree::measure(Size available) { return Size{available.width, tokens::tree_row_height * static_cast<double>(items_.size())}; }

Rect Tree::rowRect(std::size_t index) const { return Rect{bounds().x, bounds().y + tokens::tree_row_height * static_cast<double>(index), bounds().width, tokens::tree_row_height}; }

int Tree::indexAt(Point point) const {
  if (!bounds().contains(point)) return -1;
  const int index = static_cast<int>((point.y - bounds().y) / tokens::tree_row_height);
  return index >= 0 && static_cast<std::size_t>(index) < items_.size() ? index : -1;
}

std::size_t Tree::focusIndex() const {
  if (focusRow_ >= 0 && static_cast<std::size_t>(focusRow_) < items_.size()) return static_cast<std::size_t>(focusRow_);
  for (std::size_t i = 0; i < items_.size(); ++i) {
    if (items_[i].id == selectedId_) return i;
  }
  return 0;
}

Rect Tree::focusRect() const { return items_.empty() ? bounds() : rowRect(focusIndex()); }

void Tree::moveFocus(std::size_t index) {
  focusRow_ = static_cast<int>(index);
  invalidate();
  // 让外层的滚动区域把焦点行滚进可见范围
  for (Widget* ancestor = parent(); ancestor != nullptr; ancestor = ancestor->parent()) ancestor->onDescendantFocused(this);
}

void Tree::choose(std::size_t index) {
  selectedId_ = items_[index].id;
  invalidate();
  if (onSelect_) onSelect_(selectedId_);
}

bool Tree::onMouseDown(const MouseEvent& event) {
  if (event.button != MouseButton::Left) return false;
  pressedRow_ = indexAt(event.position);
  invalidate();
  return true;
}

bool Tree::onMouseUp(const MouseEvent& event) {
  if (event.button != MouseButton::Left) return false;
  const int row = indexAt(event.position);
  const int pressed = pressedRow_;
  pressedRow_ = -1;
  invalidate();
  if (row >= 0 && row == pressed) {
    focusRow_ = row;
    choose(static_cast<std::size_t>(row));
  }
  return true;
}

bool Tree::onMouseMove(const MouseEvent& event) {
  const int row = indexAt(event.position);
  if (row != hoverRow_) {
    hoverRow_ = row;
    invalidate();
  }
  return false;
}

void Tree::onStateChanged() {
  Widget::onStateChanged();
  if (!hovered() && hoverRow_ != -1) hoverRow_ = -1;
}

bool Tree::onKeyDown(const KeyEvent& event) {
  if (items_.empty()) return false;
  const std::size_t current = focusIndex();
  switch (event.virtualKey) {
    case kVirtualKeyDown:
      if (current + 1 < items_.size()) moveFocus(current + 1);
      return true;
    case kVirtualKeyUp:
      if (current > 0) moveFocus(current - 1);
      return true;
    case kVirtualKeyHome:
      moveFocus(0);
      return true;
    case kVirtualKeyEnd:
      moveFocus(items_.size() - 1);
      return true;
    case kVirtualKeyReturn:
    case kVirtualKeySpace:
      choose(current);
      return true;
    default:
      return false;
  }
}

void Tree::paint(Canvas& canvas) {
  const auto& colors = themeTokens(host()->theme());
  TextStyle style;
  style.fontSize = tokens::tree_font_size;
  style.lineHeight = 0;
  style.color = colors.text;
  style.overflow = TextOverflow::Ellipsis;
  const std::size_t focus = items_.empty() ? 0 : focusIndex();
  for (std::size_t i = 0; i < items_.size(); ++i) {
    const Rect row = rowRect(i);
    const bool selected = items_[i].id == selectedId_;
    const bool hovered = static_cast<int>(i) == hoverRow_;
    const bool pressed = hovered && static_cast<int>(i) == pressedRow_;
    Color fill = kTransparent;
    if (selected) fill = pressed ? colors.option_selected_active_bg : (hovered ? colors.option_selected_hover_bg : colors.option_selected_bg);
    else if (hovered) fill = pressed ? colors.option_active_bg : colors.option_hover_bg;
    if (fill != kTransparent) canvas.drawBox(row, BoxStyle{.fill = fill, .radius = tokens::tree_radius});
    canvas.drawText(items_[i].label, Rect{row.x + kLabelOffset, row.y, std::max(0.0, row.width - kLabelOffset - tokens::tree_padding_x), row.height}, style, VerticalAlign::Center);
    if (focusVisible() && i == focus) canvas.strokeOutline(row, tokens::tree_radius, kFocusOffset, kFocusWidth, colors.focus_border);
  }
}

}  // namespace rujian::ui
