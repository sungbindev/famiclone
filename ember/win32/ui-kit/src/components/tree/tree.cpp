/*
 * ------------------------------------------------------------------------
 * 名称：tree.cpp
 * 说明：树的实现：每行高 22，左右内边距 4，行首依次是展开箭头位置（14 宽）、图标（可选）、名称（单行、省略号），靠右可以有说明文字；子项每深一层右移 12；悬停、按下、选中各有底色，焦点行画向内的焦点框。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：点击在同一行内按下并松开才选中；点击展开箭头或双击有子项的行只展开与收起，不改变选中；方向键、Home、End 只移动焦点，回车与空格选中焦点行；
 *       右键在松开时触发右键菜单回调（和系统的习惯一致），不改变选中。
 * ------------------------------------------------------------------------
 */

#include "rujian/ui/tree.hpp"

#include <algorithm>

#include "base/theme-tokens.hpp"
#include "rujian/ui/scroll-area.hpp"

namespace rujian::ui {

namespace {

constexpr int kVirtualKeyReturn = 0x0D;
constexpr int kVirtualKeySpace = 0x20;
constexpr int kVirtualKeyEnd = 0x23;
constexpr int kVirtualKeyHome = 0x24;
constexpr int kVirtualKeyLeft = 0x25;
constexpr int kVirtualKeyUp = 0x26;
constexpr int kVirtualKeyRight = 0x27;
constexpr int kVirtualKeyDown = 0x28;
constexpr int kVirtualKeyApps = 0x5D;
constexpr int kVirtualKeyF10 = 0x79;
constexpr double kFocusOffset = -1.0;
constexpr double kFocusWidth = 1.0;
/** 右键还没有按下时 rightPressedRow_ 的值（-1 表示按在空白处）。 */
constexpr int kNoRightPress = -2;
/** 说明文字与名称之间的最小间距；点击展开箭头时左右多算的宽度。 */
constexpr double kTrailingGap = 8.0;
constexpr double kArrowSlop = 2.0;
/** 键盘触发右键菜单时，菜单离行左边的距离。 */
constexpr double kContextKeyOffset = 24.0;

}  // namespace

void Tree::setItems(std::vector<TreeItem> items) {
  items_ = std::move(items);
  hoverRow_ = -1;
  pressedRow_ = -1;
  rightPressedRow_ = kNoRightPress;
  rebuildRows();
  requestLayout();
  invalidate();
}

void Tree::setSelectedId(std::string id) {
  selectedId_ = std::move(id);
  invalidate();
}

void Tree::setOnSelect(SelectHandler handler) { onSelect_ = std::move(handler); }

void Tree::setOnContextMenu(ContextHandler handler) { onContext_ = std::move(handler); }

void Tree::setDefaultExpanded(bool expanded) {
  defaultExpanded_ = expanded;
  rebuildRows();
  requestLayout();
  invalidate();
}

void Tree::setExpanded(const std::string& id, bool expanded) {
  expanded_[id] = expanded;
  rebuildRows();
  requestLayout();
  invalidate();
}

bool Tree::isExpanded(const std::string& id) const {
  const auto found = expanded_.find(id);
  return found != expanded_.end() ? found->second : defaultExpanded_;
}

bool Tree::hasChildren(std::size_t itemIndex) const { return itemIndex + 1 < items_.size() && items_[itemIndex + 1].depth > items_[itemIndex].depth; }

void Tree::rebuildRows() {
  rows_.clear();
  // 收起的条目的子项（比它深的后续条目）不显示
  int hiddenDepth = -1;
  for (std::size_t i = 0; i < items_.size(); ++i) {
    if (hiddenDepth >= 0 && items_[i].depth > hiddenDepth) continue;
    hiddenDepth = -1;
    rows_.push_back(i);
    if (hasChildren(i) && !isExpanded(items_[i].id)) hiddenDepth = items_[i].depth;
  }
}

Size Tree::measure(Size available) {
  double height = tokens::tree_row_height * static_cast<double>(rows_.size());
  // 放在滚动区域里时撑满可见高度，条目下面的空白处也能点到树（右键菜单用）
  if (const auto* area = dynamic_cast<const ScrollArea*>(parent())) height = std::max(height, area->bounds().height);
  return Size{available.width, height};
}

Rect Tree::rowRect(std::size_t visibleIndex) const { return Rect{bounds().x, bounds().y + tokens::tree_row_height * static_cast<double>(visibleIndex), bounds().width, tokens::tree_row_height}; }

Rect Tree::arrowRect(std::size_t visibleIndex) const {
  const Rect row = rowRect(visibleIndex);
  const double x = row.x + tokens::tree_padding_x + items_[rows_[visibleIndex]].depth * tokens::tree_indent;
  return Rect{x, row.y + (row.height - tokens::icon_size) / 2.0, tokens::icon_size, tokens::icon_size};
}

int Tree::indexAt(Point point) const {
  if (!bounds().contains(point)) return -1;
  const int index = static_cast<int>((point.y - bounds().y) / tokens::tree_row_height);
  return index >= 0 && static_cast<std::size_t>(index) < rows_.size() ? index : -1;
}

std::size_t Tree::focusIndex() const {
  if (!focusId_.empty()) {
    for (std::size_t r = 0; r < rows_.size(); ++r) {
      if (items_[rows_[r]].id == focusId_) return r;
    }
  }
  for (std::size_t r = 0; r < rows_.size(); ++r) {
    if (items_[rows_[r]].id == selectedId_) return r;
  }
  return 0;
}

Rect Tree::focusRect() const { return rows_.empty() ? bounds() : rowRect(focusIndex()); }

void Tree::moveFocus(std::size_t visibleIndex) {
  focusId_ = items_[rows_[visibleIndex]].id;
  invalidate();
  // 让外层的滚动区域把焦点行滚进可见范围
  for (Widget* ancestor = parent(); ancestor != nullptr; ancestor = ancestor->parent()) ancestor->onDescendantFocused(this);
}

void Tree::choose(std::size_t visibleIndex) {
  selectedId_ = items_[rows_[visibleIndex]].id;
  invalidate();
  if (onSelect_) onSelect_(selectedId_);
}

void Tree::toggle(std::size_t visibleIndex) {
  const std::string id = items_[rows_[visibleIndex]].id;
  setExpanded(id, !isExpanded(id));
}

void Tree::showContext(const std::string& id, Point position) {
  if (onContext_) onContext_(id, position);
}

bool Tree::onMouseDown(const MouseEvent& event) {
  const int row = indexAt(event.position);
  if (event.button == MouseButton::Right) {
    rightPressedRow_ = row;
    return true;
  }
  if (event.button != MouseButton::Left) return false;
  pressedRow_ = row;
  invalidate();
  // 双击有子项的行：展开或收起（第一下已经选中了它）
  if (event.clickCount == 2 && row >= 0 && hasChildren(rows_[static_cast<std::size_t>(row)])) {
    pressedRow_ = -1;
    focusId_ = items_[rows_[static_cast<std::size_t>(row)]].id;
    toggle(static_cast<std::size_t>(row));
  }
  return true;
}

bool Tree::onMouseUp(const MouseEvent& event) {
  const int row = indexAt(event.position);
  if (event.button == MouseButton::Right) {
    if (rightPressedRow_ == kNoRightPress) return false;
    const int pressed = rightPressedRow_;
    rightPressedRow_ = kNoRightPress;
    if (row == pressed) {
      if (row >= 0) focusId_ = items_[rows_[static_cast<std::size_t>(row)]].id;
      showContext(row >= 0 ? items_[rows_[static_cast<std::size_t>(row)]].id : std::string(), event.position);
    }
    return true;
  }
  if (event.button != MouseButton::Left) return false;
  const int pressed = pressedRow_;
  pressedRow_ = -1;
  invalidate();
  if (row < 0 || row != pressed) return true;
  const std::size_t index = static_cast<std::size_t>(row);
  focusId_ = items_[rows_[index]].id;
  Rect arrow = arrowRect(index);
  arrow.x -= kArrowSlop;
  arrow.width += 2 * kArrowSlop;
  if (hasChildren(rows_[index]) && arrow.contains(event.position)) toggle(index);
  else choose(index);
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
  const bool contextKey = event.virtualKey == kVirtualKeyApps || (event.virtualKey == kVirtualKeyF10 && (event.modifiers & kModifierShift) != 0);
  if (rows_.empty()) {
    if (!contextKey) return false;
    showContext(std::string(), Point{bounds().x + kContextKeyOffset, bounds().y});
    return true;
  }
  const std::size_t current = focusIndex();
  const std::size_t itemIndex = rows_[current];
  const TreeItem& item = items_[itemIndex];
  if (contextKey) {
    const Rect row = rowRect(current);
    showContext(item.id, Point{row.x + kContextKeyOffset, row.bottom()});
    return true;
  }
  switch (event.virtualKey) {
    case kVirtualKeyDown:
      if (current + 1 < rows_.size()) moveFocus(current + 1);
      return true;
    case kVirtualKeyUp:
      if (current > 0) moveFocus(current - 1);
      return true;
    case kVirtualKeyHome:
      moveFocus(0);
      return true;
    case kVirtualKeyEnd:
      moveFocus(rows_.size() - 1);
      return true;
    case kVirtualKeyRight:
      if (!hasChildren(itemIndex)) return true;
      if (!isExpanded(item.id)) {
        focusId_ = item.id;
        setExpanded(item.id, true);
      } else if (current + 1 < rows_.size()) {
        moveFocus(current + 1);
      }
      return true;
    case kVirtualKeyLeft:
      if (hasChildren(itemIndex) && isExpanded(item.id)) {
        focusId_ = item.id;
        setExpanded(item.id, false);
        return true;
      }
      // 已经收起或没有子项：移到父项（往上找层级更浅的第一行）
      for (std::size_t r = current; r-- > 0;) {
        if (items_[rows_[r]].depth < item.depth) {
          moveFocus(r);
          break;
        }
      }
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
  TextStyle trailingStyle;
  trailingStyle.fontSize = tokens::tree_count_font_size;
  trailingStyle.lineHeight = 0;
  trailingStyle.color = colors.tree_count_color;
  trailingStyle.overflow = TextOverflow::Ellipsis;
  const std::size_t focus = rows_.empty() ? 0 : focusIndex();
  for (std::size_t i = 0; i < rows_.size(); ++i) {
    const TreeItem& item = items_[rows_[i]];
    const Rect row = rowRect(i);
    const bool selected = item.id == selectedId_;
    const bool hovered = static_cast<int>(i) == hoverRow_;
    const bool pressed = hovered && static_cast<int>(i) == pressedRow_;
    Color fill = kTransparent;
    if (selected) fill = pressed ? colors.option_selected_active_bg : (hovered ? colors.option_selected_hover_bg : colors.option_selected_bg);
    else if (hovered) fill = pressed ? colors.option_active_bg : colors.option_hover_bg;
    if (fill != kTransparent) canvas.drawBox(row, BoxStyle{.fill = fill, .radius = tokens::tree_radius});
    const Rect arrow = arrowRect(i);
    if (hasChildren(rows_[i])) canvas.drawIcon(isExpanded(item.id) ? IconId::ChevronDown : IconId::ChevronRight, arrow, colors.text_muted, tokens::icon_stroke_width);
    double x = arrow.x + tokens::icon_size + tokens::icon_text_gap;
    if (item.icon) {
      canvas.drawIcon(*item.icon, Rect{x, arrow.y, tokens::icon_size, tokens::icon_size}, colors.text_muted, tokens::icon_stroke_width);
      x += tokens::icon_size + tokens::icon_text_gap;
    }
    double right = row.right() - tokens::tree_padding_x;
    if (!item.trailing.empty()) {
      // 说明文字靠右，名称让出它的宽度
      const double width = std::min(host()->measureText(item.trailing, trailingStyle, 0).width + 1.0, std::max(0.0, (right - x) * 0.5));
      canvas.drawText(item.trailing, Rect{right - width, row.y, width, row.height}, trailingStyle, VerticalAlign::Center);
      right -= width + kTrailingGap;
    }
    canvas.drawText(item.label, Rect{x, row.y, std::max(0.0, right - x), row.height}, style, VerticalAlign::Center);
    if (focusVisible() && i == focus) canvas.strokeOutline(row, tokens::tree_radius, kFocusOffset, kFocusWidth, colors.focus_border);
  }
}

}  // namespace rujian::ui
