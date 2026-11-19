/*
 * ------------------------------------------------------------------------
 * 名称：row-list.cpp
 * 说明：信息行列表的实现：行高取最小高度与内容高度加上下内边距中较大的，各列在行内垂直居中；主文字占满剩余宽度，其余列取固定宽度或自然宽度。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：文字行高取正文行高倍数；悬停与按下的底色取行列表令牌，圆角取行圆角。
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

void RowListItem::setLeading(std::string leading) {
  hasLeading_ = true;
  leading_->setText(std::move(leading));
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
  invalidate();
}

void RowListItem::setActions(std::unique_ptr<Widget> actions) {
  if (actions_ != nullptr) removeChild(actions_);
  actions_ = actions ? addChild(std::move(actions)) : nullptr;
  invalidate();
}

RowListItem::Layout RowListItem::layout(double width) {
  Layout result;
  result.leadingWidth = hasLeading_ ? tokens::row_list_leading_width : 0.0;
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
  const double width = available.width > 0 ? available.width : 2 * tokens::row_list_padding_x + tokens::row_list_leading_width + title_->measure(Size{}).width;
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
    place(leading_, x, laid.leadingWidth);
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

bool RowListItem::onMouseDown(const MouseEvent& event) { return event.button == MouseButton::Left; }

void RowListItem::paint(Canvas& canvas) {
  const auto& colors = themeTokens(host()->theme());
  if (pressed()) canvas.drawBox(bounds(), BoxStyle{.fill = colors.row_list_active_bg, .radius = tokens::row_list_radius});
  else if (hovered()) canvas.drawBox(bounds(), BoxStyle{.fill = colors.row_list_hover_bg, .radius = tokens::row_list_radius});
}

}  // namespace rujian::ui
