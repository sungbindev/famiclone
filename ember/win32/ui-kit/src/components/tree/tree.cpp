/*
 * ------------------------------------------------------------------------
 * 名称：tree.cpp
 * 说明：树的实现：每行高 22，左右内边距 4，行首依次是展开箭头位置（14 宽）、图标（可选）、名称（单行、省略号），靠右可以有说明文字；子项每深一层右移 12；悬停、按下、选中各有底色，焦点行画向内的焦点框。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：点击在同一行内按下并松开才选中；点击展开箭头或双击有子项的行只展开与收起，不改变选中（打开“单击展开”后，单击任何位置都既选中也展开或收起，双击不再单独处理）；方向键、Home、End 只移动焦点，回车与空格选中焦点行；
 *       右键在松开时触发右键菜单回调（和系统的习惯一致），不改变选中。
 * ------------------------------------------------------------------------
 */

#include "rujian/ui/tree.hpp"

#include <algorithm>
#include <cmath>

#include "base/theme-tokens.hpp"
#include "rujian/ui/scroll-area.hpp"
#include "rujian/ui/text-input.hpp"

namespace rujian::ui {

namespace {

constexpr int kVirtualKeyReturn = 0x0D;
constexpr int kVirtualKeyEscape = 0x1B;
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
/** 按下后移动超过这么远才算开始拖动。 */
constexpr double kDragThreshold = 4.0;
/** 行内重命名的输入框比名称区域多出的右边宽度。 */
constexpr double kEditorExtraWidth = 4.0;

}  // namespace

Tree::Tree() {
  auto editor = std::make_unique<TextInput>();
  editor_ = static_cast<TextInput*>(addChild(std::move(editor)));
  editor_->setVisible(false);
  editor_->setOnEnter([this] { commitRename(); });
  editor_->setOnBlur([this] { commitRename(); });
}

void Tree::setItems(std::vector<TreeItem> items) {
  items_ = std::move(items);
  hoverRow_ = -1;
  pressedRow_ = -1;
  rightPressedRow_ = kNoRightPress;
  dragging_ = false;
  dropFolder_.clear();
  dropValid_ = false;
  rebuildRows();
  if (!renameId_.empty() && itemIndexOf(renameId_) < 0) cancelRename();
  requestLayout();
  invalidate();
}

void Tree::setSelectedId(std::string id) {
  selectedId_ = std::move(id);
  invalidate();
}

std::string Tree::focusedId() const { return rows_.empty() ? std::string() : items_[rows_[focusIndex()]].id; }

int Tree::itemIndexOf(const std::string& id) const {
  if (id.empty()) return -1;
  for (std::size_t i = 0; i < items_.size(); ++i) {
    if (items_[i].id == id) return static_cast<int>(i);
  }
  return -1;
}

void Tree::reveal(const std::string& id) {
  const int index = itemIndexOf(id);
  if (index < 0) return;
  int depth = items_[static_cast<std::size_t>(index)].depth;
  for (int i = index - 1; i >= 0 && depth > 0; --i) {
    if (items_[static_cast<std::size_t>(i)].depth < depth) {
      expanded_[items_[static_cast<std::size_t>(i)].id] = true;
      depth = items_[static_cast<std::size_t>(i)].depth;
    }
  }
  rebuildRows();
  requestLayout();
  invalidate();
}

void Tree::beginRename(const std::string& id) {
  const int index = itemIndexOf(id);
  if (index < 0 || host() == nullptr) return;
  reveal(id);
  renameId_ = id;
  renameOriginal_ = items_[static_cast<std::size_t>(index)].label;
  editor_->setValue(renameOriginal_);
  editor_->setVisible(true);
  focusId_ = id;
  layoutEditor();
  // 让外层的滚动区域把这一行滚进可见范围
  for (Widget* ancestor = parent(); ancestor != nullptr; ancestor = ancestor->parent()) ancestor->onDescendantFocused(this);
  host()->setFocus(editor_, true);
  invalidate();
}

void Tree::cancelRename() {
  if (renameId_.empty()) return;
  renameId_.clear();
  editor_->setVisible(false);
  if (host() != nullptr && host()->focusedWidget() == editor_) host()->setFocus(this, true);
  invalidate();
}

void Tree::commitRename() {
  // 先清掉重命名状态：把焦点还给树时输入框的“失去焦点”回调会再进来一次
  if (renameId_.empty()) return;
  const std::string id = renameId_;
  const std::string original = renameOriginal_;
  const std::string name = editor_->value();
  renameId_.clear();
  editor_->setVisible(false);
  if (host() != nullptr && host()->focusedWidget() == editor_) host()->setFocus(this, true);
  invalidate();
  if (name != original && onRename_) onRename_(id, name);
}

void Tree::layoutEditor() {
  if (renameId_.empty()) return;
  std::size_t row = 0;
  while (row < rows_.size() && items_[rows_[row]].id != renameId_) ++row;
  if (row >= rows_.size()) return;
  const TreeItem& item = items_[rows_[row]];
  const Rect arrow = arrowRect(row);
  double x = arrow.x + tokens::icon_size + tokens::icon_text_gap;
  if (item.icon) x += tokens::icon_size + tokens::icon_text_gap;
  const Rect line = rowRect(row);
  // 输入框的文字起点对齐名称的文字，但不盖住前面的图标
  const double shift = tokens::input_padding_x + 1.0;
  const double left = std::max(x - shift, x - tokens::icon_text_gap + 1.0);
  const double width = std::max(0.0, line.right() - tokens::tree_padding_x - left) + kEditorExtraWidth;
  const double height = editor_->measure(Size{width, 0}).height;
  editor_->arrange(Rect{left, line.y + (line.height - height) / 2.0, width, height});
}

void Tree::arrange(const Rect& bounds) {
  setBounds(bounds);
  layoutEditor();
}

std::string Tree::dropTargetAt(Point point) const {
  const int row = indexAt(point);
  if (row < 0) return {};
  const std::size_t index = rows_[static_cast<std::size_t>(row)];
  if (items_[index].folder) return items_[index].id;
  // 放在非文件夹的行上：放进它所在的文件夹
  for (std::size_t i = index; i-- > 0;) {
    if (items_[i].depth < items_[index].depth) return items_[i].id;
  }
  return {};
}

void Tree::updateDrag(Point point) {
  dropFolder_ = dropTargetAt(point);
  dropValid_ = !canDrop_ || canDrop_(dragId_, dropFolder_);
  invalidate();
}

void Tree::finishDrag(Point point) {
  updateDrag(point);
  const std::string id = dragId_;
  const std::string target = dropFolder_;
  const bool valid = dropValid_;
  dragging_ = false;
  dragId_.clear();
  dropFolder_.clear();
  dropValid_ = false;
  invalidate();
  if (valid && onDrop_) onDrop_(id, target);
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
  pressPosition_ = event.position;
  dragging_ = false;
  invalidate();
  // 双击有子项的行：展开或收起（第一下已经选中了它）；单击展开的树里不再单独处理双击
  if (!toggleOnClick_ && event.clickCount == 2 && row >= 0 && hasChildren(rows_[static_cast<std::size_t>(row)])) {
    pressedRow_ = -1;
    focusId_ = items_[rows_[static_cast<std::size_t>(row)]].id;
    toggle(static_cast<std::size_t>(row));
    return true;
  }
  if (row >= 0 && host() != nullptr) host()->captureMouse(this);
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
  if (host() != nullptr) host()->releaseMouse(this);
  if (dragging_) {
    pressedRow_ = -1;
    finishDrag(event.position);
    return true;
  }
  const int pressed = pressedRow_;
  pressedRow_ = -1;
  invalidate();
  if (row < 0 || row != pressed) return true;
  const std::size_t index = static_cast<std::size_t>(row);
  focusId_ = items_[rows_[index]].id;
  const bool expandable = hasChildren(rows_[index]);
  if (toggleOnClick_) {
    // 单击任何位置（含箭头）都选中并展开或收起
    const std::string id = focusId_;
    choose(index);
    if (expandable) setExpanded(id, !isExpanded(id));
    return true;
  }
  Rect arrow = arrowRect(index);
  arrow.x -= kArrowSlop;
  arrow.width += 2 * kArrowSlop;
  if (expandable && arrow.contains(event.position)) toggle(index);
  else choose(index);
  return true;
}

bool Tree::onMouseMove(const MouseEvent& event) {
  const int row = indexAt(event.position);
  if (row != hoverRow_) {
    hoverRow_ = row;
    invalidate();
  }
  if (dragging_) {
    updateDrag(event.position);
  } else if (onDrop_ && pressedRow_ >= 0 && pressed() && std::hypot(event.position.x - pressPosition_.x, event.position.y - pressPosition_.y) > kDragThreshold) {
    dragging_ = true;
    dragId_ = items_[rows_[static_cast<std::size_t>(pressedRow_)]].id;
    updateDrag(event.position);
  }
  return false;
}

void Tree::onStateChanged() {
  Widget::onStateChanged();
  if (!hovered() && hoverRow_ != -1) hoverRow_ = -1;
}

bool Tree::onKeyDown(const KeyEvent& event) {
  if (!renameId_.empty()) {
    if (event.virtualKey != kVirtualKeyEscape) return false;
    cancelRename();
    return true;
  }
  if (onKey_ && onKey_(event)) return true;
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
      // 单击展开的树里回车与空格也展开或收起
      if (toggleOnClick_ && hasChildren(itemIndex)) setExpanded(item.id, !isExpanded(item.id));
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
  // 放在滚动区域里时，只画可见范围内的行
  Rect view = bounds();
  if (const auto* area = dynamic_cast<const ScrollArea*>(parent())) view = area->bounds();
  for (std::size_t i = 0; i < rows_.size(); ++i) {
    const Rect row = rowRect(i);
    if (row.bottom() < view.y || row.y > view.bottom()) continue;
    const TreeItem& item = items_[rows_[i]];
    const bool selected = item.id == selectedId_;
    const bool hovered = static_cast<int>(i) == hoverRow_;
    const bool pressed = hovered && static_cast<int>(i) == pressedRow_;
    Color fill = kTransparent;
    if (selected) fill = pressed ? colors.option_selected_active_bg : (hovered ? colors.option_selected_hover_bg : colors.option_selected_bg);
    else if (hovered) fill = pressed ? colors.option_active_bg : colors.option_hover_bg;
    if (dragging_ && dropValid_ && !dropFolder_.empty() && item.id == dropFolder_) fill = colors.tree_drop_target_bg;
    if (fill != kTransparent) canvas.drawBox(row, BoxStyle{.fill = fill, .radius = tokens::tree_radius});
    const Rect arrow = arrowRect(i);
    if (hasChildren(rows_[i])) canvas.drawIcon(isExpanded(item.id) ? IconId::ChevronDown : IconId::ChevronRight, arrow, colors.text_muted, tokens::icon_stroke_width);
    double x = arrow.x + tokens::icon_size + tokens::icon_text_gap;
    if (item.icon) {
      canvas.drawIcon(*item.icon, Rect{x, arrow.y, tokens::icon_size, tokens::icon_size}, colors.text_muted, tokens::icon_stroke_width);
      x += tokens::icon_size + tokens::icon_text_gap;
    }
    double right = row.right() - tokens::tree_padding_x;
    // 正在重命名的行，名称由输入框显示
    const bool editing = item.id == renameId_;
    if (!item.trailing.empty() && !editing) {
      // 说明文字靠右，名称让出它的宽度
      const double width = std::min(host()->measureText(item.trailing, trailingStyle, 0).width + 1.0, std::max(0.0, (right - x) * 0.5));
      canvas.drawText(item.trailing, Rect{right - width, row.y, width, row.height}, trailingStyle, VerticalAlign::Center);
      right -= width + kTrailingGap;
    }
    if (!editing) canvas.drawText(item.label, Rect{x, row.y, std::max(0.0, right - x), row.height}, style, VerticalAlign::Center);
    if (focusVisible() && i == focus) canvas.strokeOutline(row, tokens::tree_radius, kFocusOffset, kFocusWidth, colors.focus_border);
  }
}

}  // namespace rujian::ui
