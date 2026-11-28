/*
 * ------------------------------------------------------------------------
 * 名称：row-list.cpp
 * 说明：信息行列表的实现：行高取最小高度与内容高度加上下内边距中较大的，各列在行内垂直居中；主文字占满剩余宽度，其余列取固定宽度或自然宽度。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：文字行高取正文行高倍数；行只有悬停底色（行不是按钮，按下不变），可点击列的悬停与按下另有列底色；圆角取行圆角。
 * ------------------------------------------------------------------------
 */

#include "rujian/ui/row-list.hpp"

#include <algorithm>

#include "base/theme-tokens.hpp"
#include "rujian/ui/label.hpp"

namespace rujian::ui {

namespace {

/** 行内文字的行高倍数（组件库在列表容器上设了正文行高）。 */
constexpr double kLineHeight = 1.45;

std::unique_ptr<Label> makeLabel() {
  auto label = std::make_unique<Label>();
  label->setFontSize(tokens::row_list_font_size);
  label->setLineHeight(kLineHeight);
  label->setWrap(false);
  return label;
}

}  // namespace

RowList::RowList() : FlexBox(FlexDirection::Column, false, tokens::row_list_gap, FlexAlign::Stretch) {}

RowListItem::RowListItem() {
  leading_ = static_cast<Label*>(addChild(makeLabel()));
  title_ = static_cast<Label*>(addChild(makeLabel()));
}

void RowListItem::setLeading(std::string leading) { setLeading(RowLeading{std::move(leading), std::nullopt, 0.0, false}); }

void RowListItem::setLeading(RowLeading leading) {
  hasLeading_ = true;
  leadingIcon_ = leading.icon;
  leadingWidth_ = leading.width;
  leadingEmphasized_ = leading.emphasized;
  leading_->setText(std::move(leading.text));
  invalidate();
}

void RowListItem::setTitle(std::string title) {
  title_->setText(std::move(title));
  invalidate();
}

void RowListItem::addCell(RowCell cell) {
  auto label = makeLabel();
  label->setText(cell.text);
  if (cell.muted) label->setColorProvider([](ThemeName theme) { return themeTokens(theme).row_list_muted_color; });
  cells_.push_back(static_cast<Label*>(addChild(std::move(label))));
  cellSpecs_.push_back(std::move(cell));
  cellHandlers_.emplace_back();
  cellEnabled_.push_back(true);
  invalidate();
}

void RowListItem::setCellText(std::size_t index, std::string text) {
  if (index >= cells_.size() || cellSpecs_[index].text == text) return;
  cellSpecs_[index].text = text;
  cells_[index]->setText(std::move(text));
  // 取文字宽度的列变了宽度，行内各列要重新摆放；还没有摆放过（没有宿主或没有尺寸）时等第一次摆放
  if (cellSpecs_[index].width <= 0 && host() != nullptr && bounds().width > 0) arrange(bounds());
}

void RowListItem::setActions(std::unique_ptr<Widget> actions) {
  if (actions_ != nullptr) removeChild(actions_);
  actions_ = actions ? addChild(std::move(actions)) : nullptr;
  invalidate();
}

RowListItem::Layout RowListItem::layout(double width) {
  Layout result;
  result.leadingWidth = hasLeading_ ? (leadingWidth_ > 0 ? leadingWidth_ : tokens::row_list_leading_width) : 0.0;
  double fixed = result.leadingWidth;
  int columns = 1 + (hasLeading_ ? 1 : 0);
  for (std::size_t i = 0; i < cells_.size(); ++i) {
    const double cellWidth = cellSpecs_[i].width > 0 ? cellSpecs_[i].width : cells_[i]->measure(Size{}).width;
    result.cellWidths.push_back(cellWidth);
    fixed += cellWidth;
    ++columns;
  }
  if (actions_ != nullptr) {
    result.actionsWidth = actions_->measure(Size{}).width;
    fixed += result.actionsWidth;
    ++columns;
  }
  const double gaps = tokens::row_list_column_gap * (columns - 1);
  result.mainWidth = std::max(0.0, width - 2 * tokens::row_list_padding_x - fixed - gaps);
  result.height = std::max(tokens::row_list_min_height, contentHeight(result) + 2 * tokens::row_list_padding_y);
  return result;
}

double RowListItem::contentHeight(const Layout& laid) {
  double height = title_->measure(Size{laid.mainWidth, 0}).height;
  if (hasLeading_) height = std::max(height, leading_->measure(Size{laid.leadingWidth, 0}).height);
  for (std::size_t i = 0; i < cells_.size(); ++i) height = std::max(height, cells_[i]->measure(Size{laid.cellWidths[i], 0}).height);
  if (actions_ != nullptr) height = std::max(height, actions_->measure(Size{}).height);
  return height;
}

Size RowListItem::measure(Size available) {
  const double leadingWidth = leadingWidth_ > 0 ? leadingWidth_ : tokens::row_list_leading_width;
  const double width = available.width > 0 ? available.width : 2 * tokens::row_list_padding_x + leadingWidth + title_->measure(Size{}).width;
  return Size{width, layout(width).height};
}

void RowListItem::arrange(const Rect& bounds) {
  Widget::arrange(bounds);
  const Layout laid = layout(bounds.width);
  leading_->setVisible(hasLeading_);
  const auto place = [&](Widget* widget, double x, double columnWidth) {
    const double height = widget->measure(Size{columnWidth, 0}).height;
    widget->arrange(Rect{x, bounds.y + (bounds.height - height) / 2.0, columnWidth, height});
  };
  double x = bounds.x + tokens::row_list_padding_x;
  if (hasLeading_) {
    // 图标占在前导列的最左边，文字排在它右边
    const double iconSpace = leadingIcon_ ? tokens::icon_size + tokens::icon_text_gap : 0.0;
    leadingIconRect_ = Rect{x, bounds.y + (bounds.height - tokens::icon_size) / 2.0, tokens::icon_size, tokens::icon_size};
    place(leading_, x + iconSpace, std::max(0.0, laid.leadingWidth - iconSpace));
    x += laid.leadingWidth + tokens::row_list_column_gap;
  }
  place(title_, x, laid.mainWidth);
  x += laid.mainWidth + tokens::row_list_column_gap;
  for (std::size_t i = 0; i < cells_.size(); ++i) {
    place(cells_[i], x, laid.cellWidths[i]);
    x += laid.cellWidths[i] + tokens::row_list_column_gap;
  }
  if (actions_ != nullptr) place(actions_, x, laid.actionsWidth);
}

bool RowListItem::onMouseDown(const MouseEvent& event) {
  if (event.button != MouseButton::Left) return false;
  pressedCell_ = cellAt(event.position);
  return true;
}

bool RowListItem::onMouseUp(const MouseEvent& event) {
  if (event.button != MouseButton::Left) return false;
  const int pressed = pressedCell_;
  pressedCell_ = -1;
  // 在同一列上按下并抬起才算点击；回调里可能改动本行，之后不再访问成员
  if (pressed >= 0 && pressed == cellAt(event.position)) {
    const auto handler = cellHandlers_[static_cast<std::size_t>(pressed)];
    handler(cellHitRect(static_cast<std::size_t>(pressed)));
  }
  return true;
}

bool RowListItem::onMouseMove(const MouseEvent& event) {
  const int cell = cellAt(event.position);
  if (cell != hoverCell_) {
    hoverCell_ = cell;
    invalidate();
  }
  return false;
}

void RowListItem::onStateChanged() {
  // 鼠标离开这一行后没有悬停的列
  if (!hovered()) hoverCell_ = -1;
  Widget::onStateChanged();
}

void RowListItem::setCellAction(std::size_t index, std::function<void(const Rect&)> handler) {
  if (index >= cells_.size()) return;
  cellHandlers_[index] = std::move(handler);
  invalidate();
}

void RowListItem::setCellActionEnabled(std::size_t index, bool enabled) {
  if (index >= cells_.size() || cellEnabled_[index] == enabled) return;
  cellEnabled_[index] = enabled;
  if (!enabled && hoverCell_ == static_cast<int>(index)) hoverCell_ = -1;
  invalidate();
}

Rect RowListItem::cellHitRect(std::size_t index) const {
  const Rect text = cells_[index]->bounds();
  return Rect{text.x - tokens::space_xs, bounds().y + tokens::space_xxs, text.width + 2 * tokens::space_xs, std::max(0.0, bounds().height - 2 * tokens::space_xxs)};
}

int RowListItem::cellAt(Point point) const {
  for (std::size_t i = 0; i < cells_.size(); ++i) {
    if (cellHandlers_[i] && cellEnabled_[i] && cellHitRect(i).contains(point)) return static_cast<int>(i);
  }
  return -1;
}

void RowListItem::paint(Canvas& canvas) {
  const auto& colors = themeTokens(host()->theme());
  // 行不是按钮：只有悬停底色，按下时不变
  if (hovered()) canvas.drawBox(bounds(), BoxStyle{.fill = colors.row_list_hover_bg, .radius = tokens::row_list_radius});
  // 可点击的列：悬停时在行底色上再加一块列底色，按下时更深
  if (hoverCell_ >= 0) {
    const bool down = pressed() && pressedCell_ == hoverCell_;
    canvas.drawBox(cellHitRect(static_cast<std::size_t>(hoverCell_)), BoxStyle{.fill = down ? colors.option_active_bg : colors.option_hover_bg, .radius = tokens::row_list_radius});
  }
  if (hasLeading_ && leadingIcon_) canvas.drawIcon(*leadingIcon_, leadingIconRect_, leadingEmphasized_ ? colors.accent : colors.text, tokens::icon_stroke_width);
}

}  // namespace rujian::ui
