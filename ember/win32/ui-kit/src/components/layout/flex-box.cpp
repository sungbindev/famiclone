/*
 * ------------------------------------------------------------------------
 * 名称：flex-box.cpp
 * 说明：弹性布局的实现：按主轴把子控件分行，行内按 grow 分配剩余空间，再按交叉轴对齐。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：纵向时只有一列；不换行的横向只有一行；换行时某个子控件放不下就另起一行，单个子控件比一行还宽时独占一行。
 * ------------------------------------------------------------------------
 */

#include <algorithm>

#include "base/theme-tokens.hpp"
#include "rujian/ui/layout.hpp"

namespace rujian::ui {

FlexBox::FlexBox(FlexDirection direction, bool wrap, double gap, FlexAlign align) : direction_(direction), wrap_(wrap), gap_(gap), align_(align) {}

void FlexBox::setGrow(const Widget* child, double grow) {
  grow_[child] = grow;
  invalidate();
}

void FlexBox::setPadding(const Insets& padding) {
  padding_ = padding;
  invalidate();
}

double FlexBox::growOf(const Widget* child) const {
  const auto found = grow_.find(child);
  return found == grow_.end() ? 0.0 : found->second;
}

std::vector<FlexBox::Line> FlexBox::breakLines(double mainLimit) {
  const bool row = direction_ == FlexDirection::Row;
  std::vector<Line> lines;
  Line current;
  const auto flush = [&] {
    if (!current.items.empty()) lines.push_back(std::move(current));
    current = Line{};
  };
  for (const auto& child : children()) {
    if (!child->visible()) continue;
    // 纵向时子控件的可用宽度就是容器宽度；横向时不限宽度，各取自然宽度
    const Size size = child->measure(row ? Size{} : Size{mainLimit, 0});
    const double main = row ? size.width : size.height;
    const double cross = row ? size.height : size.width;
    const double extra = current.items.empty() ? 0.0 : gap_;
    if (row && wrap_ && mainLimit > 0 && !current.items.empty() && current.main + extra + main > mainLimit) flush();
    current.main += (current.items.empty() ? 0.0 : gap_) + main;
    current.cross = std::max(current.cross, cross);
    current.items.push_back(child.get());
    current.sizes.push_back(size);
  }
  flush();
  return lines;
}

Size FlexBox::measure(Size available) {
  const bool row = direction_ == FlexDirection::Row;
  const double innerWidth = available.width > 0 ? std::max(0.0, available.width - padding_.horizontal()) : 0.0;
  const auto lines = breakLines(innerWidth);
  double main = 0;
  double cross = 0;
  if (row) {
    for (const Line& line : lines) {
      main = std::max(main, line.main);
      cross += line.cross;
    }
    if (lines.size() > 1) cross += gap_ * static_cast<double>(lines.size() - 1);
    return Size{main + padding_.horizontal(), cross + padding_.vertical()};
  }
  // 纵向：高度是各子控件高度加间距，宽度是最宽的子控件
  const Line empty;
  const Line& column = lines.empty() ? empty : lines.front();
  return Size{column.cross + padding_.horizontal(), column.main + padding_.vertical()};
}

void FlexBox::arrange(const Rect& bounds) {
  Widget::arrange(bounds);
  const Rect inner = bounds.inset(padding_);
  const bool row = direction_ == FlexDirection::Row;
  const auto lines = breakLines(inner.width);

  double crossOffset = row ? inner.y : inner.x;
  for (const Line& line : lines) {
    // 剩余的主轴空间按 grow 分配
    double totalGrow = 0;
    for (const Widget* item : line.items) totalGrow += growOf(item);
    const double mainSize = row ? inner.width : inner.height;
    const double free = totalGrow > 0 ? std::max(0.0, mainSize - line.main) : 0.0;
    // 纵向只有一列，列的交叉尺寸取容器宽度，子控件按对齐方式处理
    const double lineCross = row ? line.cross : inner.width;

    double mainOffset = row ? inner.x : inner.y;
    for (std::size_t i = 0; i < line.items.size(); ++i) {
      Widget* item = line.items[i];
      const Size natural = line.sizes[i];
      double main = row ? natural.width : natural.height;
      main += totalGrow > 0 ? free * growOf(item) / totalGrow : 0.0;
      double cross = row ? natural.height : natural.width;
      double crossPosition = crossOffset;
      const bool accepts = row ? item->stretchesHeight() : item->stretchesWidth();
      if (align_ == FlexAlign::Stretch && accepts) cross = lineCross;
      else if (align_ == FlexAlign::Center) crossPosition += (lineCross - cross) / 2.0;
      item->arrange(row ? Rect{mainOffset, crossPosition, main, cross} : Rect{crossPosition, mainOffset, cross, main});
      mainOffset += main + gap_;
    }
    crossOffset += line.cross + (row ? gap_ : 0.0);
  }
}

Stack::Stack() : FlexBox(FlexDirection::Column, false, tokens::space_page, FlexAlign::Stretch) {}
Row::Row() : FlexBox(FlexDirection::Row, true, tokens::space_md, FlexAlign::Center) {}
Wrap::Wrap() : FlexBox(FlexDirection::Row, true, tokens::space_sm, FlexAlign::Stretch) {}
Actions::Actions() : FlexBox(FlexDirection::Row, true, tokens::action_gap, FlexAlign::Center) {}

}  // namespace rujian::ui
