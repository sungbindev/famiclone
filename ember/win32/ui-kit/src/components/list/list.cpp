/*
 * ------------------------------------------------------------------------
 * 名称：list.cpp
 * 说明：分隔线列表的实现：行纵向排列、拉伸到列表宽度，相邻两行之间留一条分割线的高度并画线。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：网页版里分割线是后一行的上边框，宽度取设备像素的整数倍（至少 1 个设备像素）。
 * ------------------------------------------------------------------------
 */

#include "rujian/ui/list.hpp"

#include <algorithm>
#include <vector>

#include "base/theme-tokens.hpp"
#include "rujian/ui/geometry.hpp"

namespace rujian::ui {

namespace {

constexpr double kBorderWidth = 1.0;

}  // namespace

void ListRow::setTrim(bool top, bool bottom) {
  trimTop_ = top;
  trimBottom_ = bottom;
  invalidate();
}

ListRow* List::addRow(std::unique_ptr<ListRow> row) {
  ListRow* raw = row.get();
  addChild(std::move(row));
  applyTrim();
  invalidate();
  return raw;
}

void List::setTrimEnds(bool start, bool end) {
  trimStart_ = start;
  trimEnd_ = end;
  applyTrim();
  invalidate();
}

void List::applyTrim() {
  const std::vector<ListRow*> visible = rows();
  for (std::size_t i = 0; i < visible.size(); ++i) visible[i]->setTrim(trimStart_ && i == 0, trimEnd_ && i + 1 == visible.size());
}

double List::dividerWidth() const { return snapBorderWidth(kBorderWidth, host() != nullptr ? host()->scale() : 1.0); }

std::vector<ListRow*> List::rows() const {
  std::vector<ListRow*> result;
  for (const auto& child : children()) {
    if (child->visible()) result.push_back(static_cast<ListRow*>(child.get()));
  }
  return result;
}

Size List::measure(Size available) {
  double height = 0.0;
  double widest = 0.0;
  bool first = true;
  for (ListRow* row : rows()) {
    const Size size = row->measure(Size{available.width, 0});
    height += (first ? 0.0 : dividerWidth()) + size.height;
    widest = std::max(widest, size.width);
    first = false;
  }
  return Size{available.width > 0 ? available.width : widest, height};
}

void List::arrange(const Rect& bounds) {
  Widget::arrange(bounds);
  double y = bounds.y;
  bool first = true;
  for (ListRow* row : rows()) {
    if (!first) y += dividerWidth();
    const double height = row->measure(Size{bounds.width, 0}).height;
    row->arrange(Rect{bounds.x, y, bounds.width, height});
    y += height;
    first = false;
  }
}

void List::paint(Canvas& canvas) {
  const Color divider = themeTokens(host()->theme()).divider;
  const double width = dividerWidth();
  bool first = true;
  for (ListRow* row : rows()) {
    if (!first) canvas.fillRect(Rect{bounds().x, row->bounds().y - width, bounds().width, width}, divider);
    first = false;
  }
}

}  // namespace rujian::ui
