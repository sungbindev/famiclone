/*
 * ------------------------------------------------------------------------
 * 名称：file-table.cpp
 * 说明：文件列表的实现：行高 22，左右内边距 8（右边多留滚动条的位置），列间距 16；表头文字用表头色，固定在滚动区域顶部；行有悬停、选中、拖放目标三种底色，焦点行画向内的焦点框。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-25
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "rujian/ui/file-table.hpp"

#include <algorithm>
#include <cmath>

#include "base/theme-tokens.hpp"
#include "rujian/ui/scroll-area.hpp"
#include "rujian/ui/text-input.hpp"

namespace rujian::ui {

namespace {

constexpr int kVirtualKeyEscape = 0x1B;
constexpr int kVirtualKeyReturn = 0x0D;
constexpr int kVirtualKeySpace = 0x20;
constexpr int kVirtualKeyPrior = 0x21;
constexpr int kVirtualKeyNext = 0x22;
constexpr int kVirtualKeyEnd = 0x23;
constexpr int kVirtualKeyHome = 0x24;
constexpr int kVirtualKeyUp = 0x26;
constexpr int kVirtualKeyDown = 0x28;
constexpr int kVirtualKeyApps = 0x5D;
constexpr int kVirtualKeyF10 = 0x79;

constexpr double kRowHeight = tokens::tree_row_height;
constexpr double kPaddingX = 8.0;
/** 右边多留的宽度，滚动条盖在内容上，不挡住靠右的文字。 */
constexpr double kScrollbarRoom = 12.0;
/** 相邻两列之间的距离，列分割线在正中，两边的文字离它各 8。 */
constexpr double kColumnGap = 16.0;
constexpr double kMinimumNameWidth = 80.0;
/** 名称列没有给宽度时的默认宽度。 */
constexpr double kDefaultNameWidth = 220.0;
/** 拖动列分割线时，除名称列外每列至少的宽度，也是最后一列至少的宽度。 */
constexpr double kMinimumColumnWidth = 40.0;
/** 鼠标停在列分割线上多久后变成聚焦色（毫秒），与分栏的分隔条一致。 */
constexpr unsigned kDividerHoverDelayMilliseconds = 200;
/** 列分割线两侧可以抓住的范围。 */
constexpr double kDividerGrab = 4.0;
/** 表头竖线上下留的空隔。 */
constexpr double kDividerInset = 3.0;
/** 按下后移动超过这么远才算开始拖动。 */
constexpr double kDragThreshold = 4.0;
/** 翻页键一次移动的行数。 */
constexpr std::size_t kPageRows = 10;
/** 键盘触发右键菜单时，菜单离行左边的距离。 */
constexpr double kContextKeyOffset = 24.0;
constexpr double kFocusOffset = -1.0;
constexpr double kFocusWidth = 1.0;
/** 行内重命名的输入框比名称单元格多出的右边宽度。 */
constexpr double kEditorExtraWidth = 4.0;
/** 行的选中底色与焦点框右边留出的空白，与左边的内边距对称，不贴到右边缘。 */
constexpr double kRowMarginRight = tokens::space_page;
/** 弱化显示的行（如被剪切的）的图标与文字透明度。 */
constexpr double kDimmedOpacity = 0.5;
/** 没有行时说明文字离表头的距离。 */
constexpr double kEmptyTextTop = 12.0;

}  // namespace

FileTable::FileTable() {
  auto editor = std::make_unique<TextInput>();
  editor_ = static_cast<TextInput*>(addChild(std::move(editor)));
  editor_->setVisible(false);
  editor_->setOnEnter([this] { commitRename(); });
  editor_->setOnBlur([this] { commitRename(); });
}

void FileTable::setColumns(std::vector<FileTableColumn> columns) {
  columns_ = std::move(columns);
  invalidate();
}

void FileTable::setDimmedIds(const std::vector<std::string>& ids) {
  bool changed = false;
  for (FileTableRow& row : rows_) {
    const bool dimmed = std::find(ids.begin(), ids.end(), row.id) != ids.end();
    if (row.dimmed == dimmed) continue;
    row.dimmed = dimmed;
    changed = true;
  }
  if (changed) invalidate();
}

void FileTable::setRows(std::vector<FileTableRow> rows) {
  rows_ = std::move(rows);
  std::set<std::string> kept;
  for (const FileTableRow& row : rows_) {
    if (selection_.count(row.id) != 0) kept.insert(row.id);
  }
  selection_ = std::move(kept);
  if (indexOf(anchorId_) < 0) anchorId_.clear();
  if (indexOf(focusId_) < 0) focusId_.clear();
  if (!renameId_.empty() && indexOf(renameId_) < 0) cancelRename();
  hoverRow_ = -1;
  pressedRow_ = -1;
  requestLayout();
  invalidate();
}

void FileTable::setEmptyText(std::string text) {
  emptyText_ = std::move(text);
  invalidate();
}

std::vector<std::string> FileTable::selectedIds() const {
  std::vector<std::string> ids;
  for (const FileTableRow& row : rows_) {
    if (selection_.count(row.id) != 0) ids.push_back(row.id);
  }
  return ids;
}

void FileTable::setSelection(const std::vector<std::string>& ids) {
  selection_.clear();
  for (const std::string& id : ids) {
    if (indexOf(id) >= 0) selection_.insert(id);
  }
  anchorId_ = selection_.empty() ? std::string() : *ids.begin();
  if (!anchorId_.empty() && selection_.count(anchorId_) == 0) anchorId_ = *selection_.begin();
  focusId_ = anchorId_;
  invalidate();
}

std::string FileTable::focusId() const { return rows_.empty() ? std::string() : rows_[focusIndex()].id; }

int FileTable::indexOf(const std::string& id) const {
  if (id.empty()) return -1;
  for (std::size_t i = 0; i < rows_.size(); ++i) {
    if (rows_[i].id == id) return static_cast<int>(i);
  }
  return -1;
}

std::size_t FileTable::focusIndex() const {
  const int focus = indexOf(focusId_);
  if (focus >= 0) return static_cast<std::size_t>(focus);
  for (std::size_t i = 0; i < rows_.size(); ++i) {
    if (selected(i)) return i;
  }
  return 0;
}

Rect FileTable::viewport() const {
  if (const auto* area = dynamic_cast<const ScrollArea*>(parent())) return area->bounds();
  return bounds();
}

Rect FileTable::headerRect() const { return Rect{bounds().x, std::max(bounds().y, viewport().y), bounds().width, kRowHeight}; }

Rect FileTable::rowRectAt(std::size_t index) const { return Rect{bounds().x, bounds().y + kRowHeight * static_cast<double>(index + 1), bounds().width, kRowHeight}; }

int FileTable::indexAt(Point point) const {
  if (!bounds().contains(point) || headerRect().contains(point)) return -1;
  const double offset = point.y - bounds().y - kRowHeight;
  if (offset < 0) return -1;
  const int index = static_cast<int>(offset / kRowHeight);
  return static_cast<std::size_t>(index) < rows_.size() ? index : -1;
}

std::optional<std::string> FileTable::rowIdAt(Point point) const {
  const int index = indexAt(point);
  if (index < 0) return std::nullopt;
  return rows_[static_cast<std::size_t>(index)].id;
}

Rect FileTable::rowRect(const std::string& id) const {
  const int index = indexOf(id);
  return index < 0 ? Rect{} : rowRectAt(static_cast<std::size_t>(index));
}

void FileTable::setDropHighlight(const std::string& id) {
  if (dropId_ == id) return;
  dropId_ = id;
  invalidate();
}

double FileTable::fixedColumnWidth(std::size_t index) const {
  const auto user = userWidths_.find(columns_[index].title);
  if (user != userWidths_.end()) return user->second;
  return columns_[index].width > 0 ? columns_[index].width : (index == 0 ? kDefaultNameWidth : kMinimumColumnWidth);
}

double FileTable::lastColumnMinimum() const {
  if (columns_.size() == 1) return kMinimumNameWidth;
  return std::max(kMinimumColumnWidth, columns_.back().width);
}

std::vector<double> FileTable::columnWidths() const {
  std::vector<double> widths(columns_.size(), 0.0);
  if (columns_.empty()) return widths;
  const std::size_t last = columns_.size() - 1;
  const double total = bounds().width - kPaddingX - kScrollbarRoom - kColumnGap * static_cast<double>(last);
  double fixed = 0.0;
  for (std::size_t i = 0; i < last; ++i) {
    widths[i] = fixedColumnWidth(i);
    fixed += widths[i];
  }
  widths[last] = std::max(lastColumnMinimum(), total - fixed);
  return widths;
}

double FileTable::dividerX(std::size_t index, const std::vector<double>& widths) const {
  double x = bounds().x + kPaddingX;
  for (std::size_t i = 0; i <= index && i < widths.size(); ++i) x += widths[i] + (i < index ? kColumnGap : 0.0);
  return x + kColumnGap / 2.0;
}

int FileTable::dividerAt(Point point) const {
  const Rect header = headerRect();
  if (columns_.size() < 2 || !header.contains(point)) return -1;
  const std::vector<double> widths = columnWidths();
  for (std::size_t i = 0; i + 1 < columns_.size(); ++i) {
    if (std::abs(point.x - dividerX(i, widths)) <= kDividerGrab) return static_cast<int>(i);
  }
  return -1;
}

void FileTable::resizeTo(double x) {
  const std::size_t index = static_cast<std::size_t>(resizeIndex_);
  const std::vector<double>& start = resizeStartWidths_;
  // 线左边那一列变宽或变窄，线右边的列跟着移动，没有上限（超出可见宽度的部分靠横向滚动看）
  const double lowest = (index == 0 ? kMinimumNameWidth : kMinimumColumnWidth) - start[index];
  const double delta = std::max(lowest, x - resizeStartX_);
  userWidths_[columns_[index].title] = start[index] + delta;
  requestLayout();
  invalidate();
}

void FileTable::setHoverDivider(int divider) {
  if (divider == hoverDivider_) return;
  hoverDivider_ = divider;
  dividerShown_ = false;
  if (host() == nullptr) return;
  if (dividerTimer_ != 0) {
    host()->stopTimer(dividerTimer_);
    dividerTimer_ = 0;
  }
  if (divider >= 0) {
    if (host()->animationsEnabled()) dividerTimer_ = host()->startTimer(this, kDividerHoverDelayMilliseconds);
    else dividerShown_ = true;
  }
  invalidate();
}

void FileTable::onTimer(unsigned id) {
  if (id != dividerTimer_) return;
  dividerTimer_ = 0;
  dividerShown_ = true;
  invalidate();
}

Rect FileTable::nameCellRect(std::size_t index) const {
  const Rect row = rowRectAt(index);
  const std::vector<double> widths = columnWidths();
  const double iconSpace = rows_[index].icon ? tokens::icon_size + tokens::icon_text_gap : 0.0;
  return Rect{row.x + kPaddingX + iconSpace, row.y, std::max(0.0, (widths.empty() ? 0.0 : widths[0]) - iconSpace), row.height};
}

Size FileTable::measure(Size available) {
  double height = kRowHeight * static_cast<double>(rows_.size() + 1);
  // 内容不够高时撑满滚动区域，空白处也能点击与右键
  if (const auto* area = dynamic_cast<const ScrollArea*>(parent())) height = std::max(height, area->bounds().height);
  return Size{available.width, height};
}

Size FileTable::minimumSize() {
  if (columns_.empty()) return Size{};
  const std::size_t last = columns_.size() - 1;
  double width = kPaddingX + kScrollbarRoom + kColumnGap * static_cast<double>(last) + lastColumnMinimum();
  for (std::size_t i = 0; i < last; ++i) width += fixedColumnWidth(i);
  return Size{width, 0};
}

void FileTable::arrange(const Rect& bounds) {
  setBounds(bounds);
  layoutEditor();
}

Rect FileTable::focusRect() const {
  if (rows_.empty()) return bounds();
  // 多算上方表头的高度：滚到这一行时它不会被固定的表头盖住
  const Rect row = rowRectAt(focusIndex());
  return Rect{row.x, row.y - kRowHeight, row.width, row.height + kRowHeight};
}

void FileTable::notifySelection() {
  invalidate();
  if (onSelectionChange_) onSelectionChange_();
}

void FileTable::selectOnly(std::size_t index) {
  selection_.clear();
  selection_.insert(rows_[index].id);
  anchorId_ = rows_[index].id;
  focusId_ = rows_[index].id;
  notifySelection();
}

void FileTable::selectRange(std::size_t index) {
  const int anchor = indexOf(anchorId_);
  const std::size_t from = anchor >= 0 ? static_cast<std::size_t>(anchor) : index;
  selection_.clear();
  for (std::size_t i = std::min(from, index); i <= std::max(from, index); ++i) selection_.insert(rows_[i].id);
  if (anchor < 0) anchorId_ = rows_[index].id;
  focusId_ = rows_[index].id;
  notifySelection();
}

void FileTable::moveFocus(std::size_t index) {
  focusId_ = rows_[index].id;
  invalidate();
  // 让外层的滚动区域把焦点行滚进可见范围
  for (Widget* ancestor = parent(); ancestor != nullptr; ancestor = ancestor->parent()) ancestor->onDescendantFocused(this);
}

void FileTable::navigate(std::size_t index, bool extend, bool keepSelection) {
  index = std::min(index, rows_.size() - 1);
  if (extend) selectRange(index);
  else if (keepSelection) invalidate();
  else selectOnly(index);
  moveFocus(index);
}

void FileTable::showContext(Point position) {
  if (onContext_) onContext_(selectedIds(), position);
}

bool FileTable::onMouseDown(const MouseEvent& event) {
  const int row = indexAt(event.position);
  const bool ctrl = (event.modifiers & kModifierCtrl) != 0;
  const bool shift = (event.modifiers & kModifierShift) != 0;
  if (event.button == MouseButton::Right) {
    rightPressed_ = true;
    if (row < 0) {
      if (!selection_.empty()) {
        selection_.clear();
        notifySelection();
      }
    } else if (!selected(static_cast<std::size_t>(row))) {
      selectOnly(static_cast<std::size_t>(row));
    } else {
      focusId_ = rows_[static_cast<std::size_t>(row)].id;
      invalidate();
    }
    return true;
  }
  if (event.button != MouseButton::Left) return false;
  if (const int divider = dividerAt(event.position); divider >= 0) {
    resizeIndex_ = divider;
    resizeStartX_ = event.position.x;
    resizeStartWidths_ = columnWidths();
    if (host() != nullptr) host()->captureMouse(this);
    return true;
  }
  pressedRow_ = -1;
  dragged_ = false;
  collapseOnRelease_ = false;
  suppressRelease_ = false;
  if (row < 0) {
    if (!ctrl && !shift && !selection_.empty()) {
      selection_.clear();
      notifySelection();
    }
    return true;
  }
  const std::size_t index = static_cast<std::size_t>(row);
  if (event.clickCount == 2 && !ctrl && !shift) {
    // 双击：打开；第一下已经选中了它
    suppressRelease_ = true;
    if (!selected(index) || selection_.size() > 1) selectOnly(index);
    if (onActivate_) onActivate_(rows_[index].id);
    return true;
  }
  if (ctrl) {
    if (selected(index)) selection_.erase(rows_[index].id);
    else selection_.insert(rows_[index].id);
    anchorId_ = rows_[index].id;
    focusId_ = rows_[index].id;
    notifySelection();
  } else if (shift) {
    selectRange(index);
  } else if (selected(index)) {
    // 点在已选中的行上：可能是要拖动整个选区，松开时没有拖动才收成这一行
    collapseOnRelease_ = selection_.size() > 1;
    focusId_ = rows_[index].id;
    anchorId_ = rows_[index].id;
    invalidate();
  } else {
    selectOnly(index);
  }
  pressedRow_ = row;
  pressPosition_ = event.position;
  if (host() != nullptr) host()->captureMouse(this);
  return true;
}

bool FileTable::onMouseUp(const MouseEvent& event) {
  if (event.button == MouseButton::Right) {
    if (!rightPressed_) return false;
    rightPressed_ = false;
    showContext(event.position);
    return true;
  }
  if (event.button != MouseButton::Left) return false;
  if (host() != nullptr) host()->releaseMouse(this);
  if (resizeIndex_ >= 0) {
    resizeIndex_ = -1;
    setHoverDivider(dividerAt(event.position));
    return true;
  }
  const int pressed = pressedRow_;
  pressedRow_ = -1;
  if (suppressRelease_) {
    suppressRelease_ = false;
    return true;
  }
  if (collapseOnRelease_ && !dragged_ && pressed >= 0 && indexAt(event.position) == pressed) selectOnly(static_cast<std::size_t>(pressed));
  collapseOnRelease_ = false;
  dragged_ = false;
  return true;
}

bool FileTable::onMouseMove(const MouseEvent& event) {
  if (resizeIndex_ >= 0) {
    resizeTo(event.position.x);
    return true;
  }
  if (const int divider = dividerAt(event.position); divider != hoverDivider_) {
    setHoverDivider(divider);
  }
  const int row = indexAt(event.position);
  if (row != hoverRow_) {
    hoverRow_ = row;
    invalidate();
  }
  if (pressedRow_ >= 0 && pressed() && !dragged_ && std::hypot(event.position.x - pressPosition_.x, event.position.y - pressPosition_.y) > kDragThreshold) {
    dragged_ = true;
    collapseOnRelease_ = false;
    if (host() != nullptr) host()->releaseMouse(this);
    const std::vector<std::string> ids = selectedIds();
    if (onDragStart_ && !ids.empty()) onDragStart_(ids);
  }
  return false;
}

void FileTable::onStateChanged() {
  Widget::onStateChanged();
  if (!hovered() && hoverRow_ != -1) hoverRow_ = -1;
  if (!hovered() && resizeIndex_ < 0) setHoverDivider(-1);
}

bool FileTable::onKeyDown(const KeyEvent& event) {
  if (!renameId_.empty()) {
    if (event.virtualKey != kVirtualKeyEscape) return false;
    cancelRename();
    return true;
  }
  if (onKey_ && onKey_(event)) return true;
  const bool ctrl = (event.modifiers & kModifierCtrl) != 0;
  const bool shift = (event.modifiers & kModifierShift) != 0;
  if (event.virtualKey == kVirtualKeyApps || (event.virtualKey == kVirtualKeyF10 && shift)) {
    const Rect anchor = rows_.empty() ? headerRect() : rowRectAt(focusIndex());
    showContext(Point{anchor.x + kContextKeyOffset, anchor.bottom()});
    return true;
  }
  if (rows_.empty()) return false;
  const std::size_t current = focusIndex();
  switch (event.virtualKey) {
    case kVirtualKeyDown:
      navigate(std::min(current + 1, rows_.size() - 1), shift, ctrl);
      return true;
    case kVirtualKeyUp:
      navigate(current > 0 ? current - 1 : 0, shift, ctrl);
      return true;
    case kVirtualKeyNext:
      navigate(current + kPageRows, shift, ctrl);
      return true;
    case kVirtualKeyPrior:
      navigate(current > kPageRows ? current - kPageRows : 0, shift, ctrl);
      return true;
    case kVirtualKeyHome:
      navigate(0, shift, ctrl);
      return true;
    case kVirtualKeyEnd:
      navigate(rows_.size() - 1, shift, ctrl);
      return true;
    case kVirtualKeySpace:
      if (ctrl) {
        if (selected(current)) selection_.erase(rows_[current].id);
        else selection_.insert(rows_[current].id);
        anchorId_ = rows_[current].id;
        notifySelection();
      } else {
        selectOnly(current);
      }
      return true;
    case kVirtualKeyReturn:
      if (onActivate_) onActivate_(rows_[current].id);
      return true;
    default:
      return false;
  }
}

void FileTable::beginRename(const std::string& id) {
  const int index = indexOf(id);
  if (index < 0 || host() == nullptr) return;
  const FileTableRow& row = rows_[static_cast<std::size_t>(index)];
  renameId_ = id;
  renameOriginal_ = row.cells.empty() ? std::string() : row.cells.front();
  editor_->setValue(renameOriginal_);
  editor_->setVisible(true);
  layoutEditor();
  // 让外层的滚动区域把这一行滚进可见范围
  focusId_ = id;
  for (Widget* ancestor = parent(); ancestor != nullptr; ancestor = ancestor->parent()) ancestor->onDescendantFocused(this);
  host()->setFocus(editor_, true);
  // 文件只选中扩展名之前的部分（获得焦点时输入框会全选，要在之后调整）
  if (!row.folder) editor_->selectBaseName();
  invalidate();
}

void FileTable::cancelRename() {
  if (renameId_.empty()) return;
  renameId_.clear();
  editor_->setVisible(false);
  if (host() != nullptr && host()->focusedWidget() == editor_) host()->setFocus(this, true);
  invalidate();
}

void FileTable::commitRename() {
  // 先清掉重命名状态：把焦点还给列表时输入框的“失去焦点”回调会再进来一次
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

void FileTable::layoutEditor() {
  if (renameId_.empty()) return;
  const int index = indexOf(renameId_);
  if (index < 0) return;
  const Rect cell = nameCellRect(static_cast<std::size_t>(index));
  // 输入框的文字起点对齐名称单元格的文字
  const double shift = tokens::input_padding_x + 1.0;
  const double width = cell.width + shift + kEditorExtraWidth;
  const double height = editor_->measure(Size{width, 0}).height;
  editor_->arrange(Rect{cell.x - shift, cell.y + (cell.height - height) / 2.0, width, height});
}

void FileTable::paint(Canvas& canvas) {
  const auto& colors = themeTokens(host()->theme());
  const Rect view = viewport();
  const std::vector<double> widths = columnWidths();
  const std::size_t focus = rows_.empty() ? 0 : focusIndex();

  TextStyle nameStyle;
  nameStyle.fontSize = tokens::tree_font_size;
  nameStyle.lineHeight = 0;
  nameStyle.color = colors.text;
  nameStyle.overflow = TextOverflow::Ellipsis;
  TextStyle cellStyle = nameStyle;
  cellStyle.color = colors.text_muted;

  for (std::size_t i = 0; i < rows_.size(); ++i) {
    const Rect row = rowRectAt(i);
    if (row.bottom() < view.y || row.y > view.bottom()) continue;
    const FileTableRow& item = rows_[i];
    const double opacity = item.dimmed ? kDimmedOpacity : 1.0;
    TextStyle rowNameStyle = nameStyle;
    rowNameStyle.color = nameStyle.color.withOpacity(opacity);
    TextStyle rowCellStyle = cellStyle;
    rowCellStyle.color = cellStyle.color.withOpacity(opacity);
    const Rect box{row.x, row.y, std::max(0.0, row.width - kRowMarginRight), row.height};
    const bool hovered = static_cast<int>(i) == hoverRow_;
    Color fill = kTransparent;
    if (item.id == dropId_) fill = colors.tree_drop_target_bg;
    else if (selected(i)) fill = hovered ? colors.option_selected_hover_bg : colors.option_selected_bg;
    else if (hovered) fill = colors.option_hover_bg;
    if (fill != kTransparent) canvas.drawBox(box, BoxStyle{.fill = fill, .radius = tokens::tree_radius});
    double x = row.x + kPaddingX;
    for (std::size_t c = 0; c < columns_.size() && c < widths.size(); ++c) {
      const double width = widths[c];
      const std::string& text = c < item.cells.size() ? item.cells[c] : std::string();
      double textX = x;
      double textWidth = width;
      if (c == 0 && item.icon) {
        canvas.drawIcon(*item.icon, Rect{x, row.y + (row.height - tokens::icon_size) / 2.0, tokens::icon_size, tokens::icon_size}, colors.text_muted.withOpacity(opacity), tokens::icon_stroke_width);
        textX += tokens::icon_size + tokens::icon_text_gap;
        textWidth -= tokens::icon_size + tokens::icon_text_gap;
      }
      // 正在重命名的行，名称由输入框显示
      const bool editing = c == 0 && item.id == renameId_;
      if (!text.empty() && !editing) {
        if (columns_[c].alignEnd && c + 1 < columns_.size()) {
          const double natural = host()->measureText(text, cellStyle, 0).width + 1.0;
          if (natural <= textWidth) {
            textX += textWidth - natural;
            textWidth = natural;
          }
        }
        canvas.drawText(text, Rect{textX, row.y, std::max(0.0, textWidth), row.height}, c == 0 ? rowNameStyle : rowCellStyle, VerticalAlign::Center);
      }
      x += width + kColumnGap;
    }
    if (focusVisible() && i == focus && renameId_.empty()) canvas.strokeOutline(box, tokens::tree_radius, kFocusOffset, kFocusWidth, colors.focus_border);
  }

  if (rows_.empty() && !emptyText_.empty()) {
    const double width = host()->measureText(emptyText_, cellStyle, 0).width + 1.0;
    canvas.drawText(emptyText_, Rect{view.x + std::max(0.0, (view.width - width) / 2.0), view.y + kRowHeight + kEmptyTextTop, width, kRowHeight}, cellStyle, VerticalAlign::Center);
  }

  // 表头：固定在可见区域顶部，盖住下面滚过去的行
  const Rect header = headerRect();
  canvas.fillRect(header, colors.bg);
  TextStyle headerStyle;
  headerStyle.fontSize = tokens::table_header_font_size;
  headerStyle.fontWeight = static_cast<int>(tokens::table_header_font_weight);
  headerStyle.lineHeight = 0;
  headerStyle.color = colors.table_header_color;
  headerStyle.overflow = TextOverflow::Ellipsis;
  double x = header.x + kPaddingX;
  for (std::size_t c = 0; c < columns_.size() && c < widths.size(); ++c) {
    double textX = x;
    double textWidth = widths[c];
    if (c == 0) {
      // 与名称列的文字对齐：图标的位置留空
      textX += tokens::icon_size + tokens::icon_text_gap;
      textWidth -= tokens::icon_size + tokens::icon_text_gap;
    }
    canvas.drawText(columns_[c].title, Rect{textX, header.y, std::max(0.0, textWidth), header.height}, headerStyle, VerticalAlign::Center);
    x += widths[c] + kColumnGap;
  }
  // 相邻两列之间的分割线：鼠标停够时间或正在拖动时画成聚焦色的粗条
  for (std::size_t c = 0; c + 1 < columns_.size() && c + 1 < widths.size(); ++c) {
    const double lineX = std::round(dividerX(c, widths));
    if (static_cast<int>(c) == resizeIndex_ || (static_cast<int>(c) == hoverDivider_ && dividerShown_)) {
      canvas.fillRect(Rect{lineX + 0.5 - tokens::space_xs / 2.0, header.y, tokens::space_xs, header.height}, colors.focus_border);
    } else {
      canvas.fillRect(Rect{lineX, header.y + kDividerInset, 1.0, header.height - 2 * kDividerInset}, colors.text_muted.withOpacity(0.35));
    }
  }
}

}  // namespace rujian::ui
