/*
 * ------------------------------------------------------------------------
 * 名称：table.cpp
 * 说明：数据表格的实现：列宽按内容的最小与最大宽度分配，行高取最小行高与内容加上下内边距中较大的，单元格在行内垂直居中；行之间画分割线，表头有底色与下边线。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：紧凑、平铺外观：行高下限 32，单元格内边距上下 4、左右 10，表头底色是正文色 6% 叠在卡片底色上；详情行上下内边距 8、左右 10。
 *       网页版里 border-collapse 的分割线落在行与行之间的格线上；这里把分割线画在后一行的顶边那一条里，行距不变。
 * ------------------------------------------------------------------------
 */

#include "rujian/ui/table.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

#include "base/theme-tokens.hpp"
#include "rujian/ui/geometry.hpp"
#include "rujian/ui/label.hpp"

namespace rujian::ui {

namespace {

constexpr double kBorderWidth = 1.0;
constexpr double kLineHeight = 1.45;
constexpr double kCellPaddingX = tokens::table_compact_cell_padding_x;
constexpr double kCellPaddingY = tokens::table_compact_cell_padding_y;

/** 网页版的布局单位是 1/64 像素，浮点数转成布局单位时向下取；表格的行高累加后会影响分割线落在哪个设备像素上。 */
double layoutUnit(double value) { return std::floor(value * 64.0) / 64.0; }

}  // namespace

Table::Table() = default;

void Table::addColumn(TableColumn column) {
  auto header = std::make_unique<Label>(column.title);
  header->setFontSize(tokens::table_header_font_size);
  header->setFontWeight(static_cast<int>(tokens::table_header_font_weight));
  header->setLineHeight(kLineHeight);
  header->setWrap(false);
  header->setColorProvider([](ThemeName theme) { return themeTokens(theme).table_header_color; });
  headers_.push_back(static_cast<Label*>(addChild(std::move(header))));
  columns_.push_back(std::move(column));
  invalidate();
}

std::unique_ptr<Label> Table::makeText(std::string text) {
  auto label = std::make_unique<Label>(std::move(text));
  label->setFontSize(tokens::table_compact_font_size);
  label->setLineHeight(kLineHeight);
  return label;
}

std::size_t Table::addRow(std::vector<std::unique_ptr<Widget>> cells) {
  Row row;
  for (std::size_t i = 0; i < cells.size() && i < columns_.size(); ++i) {
    if (auto* label = dynamic_cast<Label*>(cells[i].get())) {
      if (columns_[i].nowrap) label->setWrap(false);
      if (columns_[i].muted) label->setColorProvider([](ThemeName theme) { return themeTokens(theme).text_muted; });
    }
    row.cells.push_back(addChild(std::move(cells[i])));
  }
  rows_.push_back(std::move(row));
  invalidate();
  return rows_.size() - 1;
}

void Table::setRowDetail(std::size_t row, std::unique_ptr<Widget> detail) {
  if (row >= rows_.size()) return;
  if (rows_[row].detail != nullptr) removeChild(rows_[row].detail);
  rows_[row].detail = detail ? addChild(std::move(detail)) : nullptr;
  invalidate();
}

void Table::setSelectedRow(int row) {
  selectedRow_ = row;
  invalidate();
}

void Table::setOnRowSelect(SelectHandler handler) { onSelect_ = std::move(handler); }

double Table::borderWidth() const { return snapBorderWidth(kBorderWidth, host() != nullptr ? host()->scale() : 1.0); }

Table::Layout Table::layout(double width) {
  Layout result;
  const std::size_t count = columns_.size();
  std::vector<double> minimum(count, 0.0);
  std::vector<double> maximum(count, 0.0);
  for (std::size_t c = 0; c < count; ++c) {
    const double header = headers_[c]->measure(Size{}).width + 2 * kCellPaddingX;
    minimum[c] = std::max(columns_[c].minWidth, header);
    maximum[c] = std::max(minimum[c], header);
    for (const Row& row : rows_) {
      if (c >= row.cells.size()) continue;
      minimum[c] = std::max(minimum[c], row.cells[c]->measure(Size{1, 0}).width + 2 * kCellPaddingX);
      maximum[c] = std::max(maximum[c], row.cells[c]->measure(Size{}).width + 2 * kCellPaddingX);
    }
    maximum[c] = std::max(maximum[c], minimum[c]);
  }

  const double sumMin = std::accumulate(minimum.begin(), minimum.end(), 0.0);
  const double sumMax = std::accumulate(maximum.begin(), maximum.end(), 0.0);
  result.columnWidths.assign(count, 0.0);
  for (std::size_t c = 0; c < count; ++c) {
    if (width >= sumMax) {
      // 放得下：多出的宽度按最大内容宽度的比例分
      result.columnWidths[c] = sumMax > 0 ? maximum[c] + (width - sumMax) * maximum[c] / sumMax : 0.0;
    } else if (width > sumMin && sumMax > sumMin) {
      result.columnWidths[c] = minimum[c] + (maximum[c] - minimum[c]) * (width - sumMin) / (sumMax - sumMin);
    } else {
      result.columnWidths[c] = minimum[c];
    }
  }

  const double border = borderWidth();
  result.headerHeight = 0.0;
  for (Label* header : headers_) result.headerHeight = std::max(result.headerHeight, layoutUnit(header->measure(Size{}).height));
  result.headerHeight += 2 * kCellPaddingY;

  // 表头下边线与相邻两行（含详情行）之间的分割线各占独立的一条，行底色不覆盖它们
  double y = result.headerHeight + border;
  for (std::size_t r = 0; r < rows_.size(); ++r) {
    const Row& row = rows_[r];
    double content = 0.0;
    for (std::size_t c = 0; c < row.cells.size(); ++c) content = std::max(content, layoutUnit(row.cells[c]->measure(Size{result.columnWidths[c] - 2 * kCellPaddingX, 0}).height));
    const bool lastTr = r + 1 == rows_.size() && row.detail == nullptr;
    // 行的最小高度包含它自己两侧各半条分割线（最后一行下面没有线）
    const double minimumHeight = tokens::table_compact_row_height - border / 2.0 - (lastTr ? 0.0 : border / 2.0);
    const double height = std::max(minimumHeight, content + 2 * kCellPaddingY);
    result.rowTops.push_back(y);
    result.rowHeights.push_back(height);
    y += height;
    double detailHeight = 0.0;
    if (row.detail != nullptr) {
      y += border;
      detailHeight = row.detail->measure(Size{width - 2 * kCellPaddingX, 0}).height + 2 * tokens::space_md;
    }
    result.detailTops.push_back(y);
    result.detailHeights.push_back(detailHeight);
    y += detailHeight;
    if (r + 1 < rows_.size()) y += border;
  }
  result.height = y;
  return result;
}

Size Table::measure(Size available) {
  double width = available.width;
  if (width <= 0) {
    for (std::size_t c = 0; c < columns_.size(); ++c) {
      double widest = headers_[c]->measure(Size{}).width;
      for (const Row& row : rows_) widest = std::max(widest, row.cells[c]->measure(Size{}).width);
      width += std::max(columns_[c].minWidth, widest + 2 * kCellPaddingX);
    }
  }
  return Size{width, layout(width).height};
}

void Table::arrange(const Rect& bounds) {
  Widget::arrange(bounds);
  const Layout laid = layout(bounds.width);
  double x = bounds.x;
  for (std::size_t c = 0; c < columns_.size(); ++c) {
    const double columnWidth = laid.columnWidths[c];
    const double textHeight = headers_[c]->measure(Size{}).height;
    headers_[c]->arrange(Rect{x + kCellPaddingX, bounds.y + kCellPaddingY, std::max(0.0, columnWidth - 2 * kCellPaddingX), textHeight});
    for (std::size_t r = 0; r < rows_.size(); ++r) {
      Widget* cell = rows_[r].cells[c];
      const double cellWidth = std::max(0.0, columnWidth - 2 * kCellPaddingX);
      const double cellHeight = cell->measure(Size{cellWidth, 0}).height;
      cell->arrange(Rect{x + kCellPaddingX, bounds.y + laid.rowTops[r] + (laid.rowHeights[r] - cellHeight) / 2.0, cellWidth, cellHeight});
    }
    x += columnWidth;
  }
  for (std::size_t r = 0; r < rows_.size(); ++r) {
    if (rows_[r].detail == nullptr) continue;
    const double width = std::max(0.0, bounds.width - 2 * kCellPaddingX);
    rows_[r].detail->arrange(Rect{bounds.x + kCellPaddingX, bounds.y + laid.detailTops[r] + tokens::space_md, width, laid.detailHeights[r] - 2 * tokens::space_md});
  }
}

int Table::rowAt(Point point) {
  if (!bounds().contains(point)) return -1;
  const Layout laid = layout(bounds().width);
  const double y = point.y - bounds().y;
  for (std::size_t r = 0; r < rows_.size(); ++r) {
    if (y >= laid.rowTops[r] && y < laid.rowTops[r] + laid.rowHeights[r]) return static_cast<int>(r);
  }
  return -1;
}

CursorKind Table::cursor() const { return onSelect_ && hoverRow_ >= 0 ? CursorKind::Hand : CursorKind::Arrow; }

bool Table::onMouseDown(const MouseEvent& event) {
  if (event.button != MouseButton::Left || !onSelect_) return false;
  pressedRow_ = rowAt(event.position);
  return true;
}

bool Table::onMouseUp(const MouseEvent& event) {
  if (event.button != MouseButton::Left || !onSelect_) return false;
  const int row = rowAt(event.position);
  const int pressed = pressedRow_;
  pressedRow_ = -1;
  if (row >= 0 && row == pressed) {
    selectedRow_ = row;
    invalidate();
    onSelect_(static_cast<std::size_t>(row));
  }
  return true;
}

bool Table::onMouseMove(const MouseEvent& event) {
  const int row = rowAt(event.position);
  if (row != hoverRow_) {
    hoverRow_ = row;
    invalidate();
  }
  return false;
}

void Table::onStateChanged() {
  Widget::onStateChanged();
  if (!hovered() && hoverRow_ != -1) hoverRow_ = -1;
}

void Table::paint(Canvas& canvas) {
  const auto& colors = themeTokens(host()->theme());
  const Layout laid = layout(bounds().width);
  const double border = borderWidth();
  const Rect box = bounds();
  // 表头：底色（正文色 6% 叠在卡片底色上）与下边线
  const Color headerBg = compose(colors.text.withOpacity(0.06), colors.card_bg);
  canvas.fillRect(Rect{box.x, box.y, box.width, laid.headerHeight}, headerBg);
  canvas.fillRect(Rect{box.x, box.y + laid.headerHeight, box.width, border}, colors.divider);

  for (std::size_t r = 0; r < rows_.size(); ++r) {
    const Rect row{box.x, box.y + laid.rowTops[r], box.width, laid.rowHeights[r]};
    // \u5355\u5143\u683c\u7684\u5e95\u8272\u94fa\u5230\u5206\u5272\u7ebf\u7684\u4e2d\u5fc3\uff08\u5404\u5360\u7ebf\u5bbd\u7684\u4e00\u534a\uff09\uff0c\u518d\u5bf9\u9f50\u8bbe\u5907\u50cf\u7d20
    const Rect shade{row.x, row.y - border / 2.0, row.width, row.height + border};
    const bool selected = onSelect_ && static_cast<int>(r) == selectedRow_;
    const bool hovered = static_cast<int>(r) == hoverRow_;
    if (selected) canvas.fillRect(shade, hovered ? colors.option_selected_hover_bg : colors.option_selected_bg);
    else if (hovered) canvas.fillRect(shade, colors.table_row_hover);
    // 分割线在行与行之间的缝里：除第一行外每行的上方、每个详情行的上方
    if (r > 0) canvas.fillRect(Rect{box.x, row.y - border, box.width, border}, colors.table_row_divider);
    if (laid.detailHeights[r] > 0) canvas.fillRect(Rect{box.x, box.y + laid.detailTops[r] - border, box.width, border}, colors.table_row_divider);
  }
}

}  // namespace rujian::ui
