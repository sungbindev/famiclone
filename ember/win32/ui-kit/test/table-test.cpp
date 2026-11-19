/*
 * ------------------------------------------------------------------------
 * 名称：table-test.cpp
 * 说明：表格的测试：列宽分配（放得下按最大宽度比例、不够时在最小与最大之间、最小宽度下限）、行高下限与内容撑高、详情行、行选中与回调、主副文字单元格。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：单元格用固定尺寸的 Block（最小宽度 = 最大宽度 = 自己的宽度），单元格左右内边距各 10。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "rujian/ui/root.hpp"
#include "rujian/ui/table.hpp"

using namespace rujian::ui;

namespace {

/** 固定宽高的内容。 */
class Block final : public Widget {
 public:
  Block(double width, double height) : width_(width), height_(height) {}
  Size measure(Size /*available*/) override { return Size{width_, height_}; }
  [[nodiscard]] bool stretchesWidth() const override { return false; }

 private:
  double width_;
  double height_;
};

std::vector<std::unique_ptr<Widget>> cells(double firstWidth, double secondWidth, double height = 10) {
  std::vector<std::unique_ptr<Widget>> result;
  result.push_back(std::make_unique<Block>(firstWidth, height));
  result.push_back(std::make_unique<Block>(secondWidth, height));
  return result;
}

struct Scene {
  Root root{ThemeName::Dark, 1.0};
  Table* table{nullptr};

  Scene() {
    auto created = std::make_unique<Table>();
    table = created.get();
    table->addColumn({"甲", 0, false, false});
    table->addColumn({"乙", 0, false, false});
    root.setContent(std::move(created));
  }

  void layout(double width) {
    root.layout(Size{width, 600});
    table->arrange(Rect{0, 0, width, table->measure(Size{width, 0}).height});
  }

  [[nodiscard]] const Rect& cell(std::size_t row, std::size_t column) const {
    // 子控件顺序：各列表头，然后按行加入的单元格
    return table->children()[2 + row * 2 + column]->bounds();
  }
};

}  // namespace

TEST(Table, ExtraWidthIsSharedInProportionToTheMaximumWidths) {
  Scene s;
  s.table->addRow(cells(100, 50));
  s.layout(300);
  // 最大宽度 120 与 70，多出 110：第一列 120 + 110 * 120 / 190，单元格再减去左右内边距
  const double first = 120 + 110.0 * 120 / 190;
  EXPECT_NEAR(s.cell(0, 0).width, first - 20, 1e-6);
  EXPECT_NEAR(s.cell(0, 1).x, first + 10, 1e-6);
}

TEST(Table, NarrowWidthFallsBackToTheMinimumAndKeepsTheColumnsInOrder) {
  Scene s;
  s.table->addRow(cells(100, 50));
  s.layout(100);
  EXPECT_DOUBLE_EQ(s.cell(0, 0).x, 10);
  EXPECT_GE(s.cell(0, 1).x, s.cell(0, 0).right());
}

TEST(Table, MinWidthOfAColumnIsRespected) {
  Root root{ThemeName::Dark, 1.0};
  auto created = std::make_unique<Table>();
  Table* table = created.get();
  table->addColumn({"甲", 200, false, false});
  table->addColumn({"乙", 0, false, false});
  table->addRow(cells(10, 10));
  root.setContent(std::move(created));
  root.layout(Size{250, 600});
  table->arrange(Rect{0, 0, 250, table->measure(Size{250, 0}).height});
  EXPECT_GE(table->children()[2]->bounds().width, 200 - 20);
}

TEST(Table, RowsHaveAMinimumHeightAndGrowWithTheirContent) {
  Scene s;
  s.table->addRow(cells(40, 40, 10));
  s.table->addRow(cells(40, 40, 50));
  s.layout(300);
  const double header = s.table->measure(Size{300, 0}).height - 31 - (50 + 8) - 2;
  EXPECT_GT(header, 20);
  // 表头与第一行之间、两行之间各有一条 1 像素的分割线；行的最小高度 32 包含它两侧各半条线
  EXPECT_DOUBLE_EQ(s.table->measure(Size{300, 0}).height, header + 1 + 31 + 1 + 58);
  // 单元格在行内垂直居中
  EXPECT_DOUBLE_EQ(s.cell(0, 0).y, header + 1 + (31 - 10) / 2.0);
}

TEST(Table, ARowDetailSitsBelowItsRowAcrossAllColumns) {
  Scene s;
  s.table->addRow(cells(40, 40, 10));
  const double without = [&] {
    s.layout(300);
    return s.table->measure(Size{300, 0}).height;
  }();
  s.table->setRowDetail(0, std::make_unique<Block>(60, 30));
  s.layout(300);
  // 加详情后第一行不再是最后一行，最小高度里多出下面的半条线（少 0.5），再加一条线与详情行
  EXPECT_DOUBLE_EQ(s.table->measure(Size{300, 0}).height, without - 0.5 + 1 + 30 + 16);
}

TEST(Table, ClickingARowSelectsItOnlyWhenItIsSelectable) {
  Scene s;
  s.table->addRow(cells(40, 40, 10));
  s.table->addRow(cells(40, 40, 10));
  s.layout(300);
  const double y = s.cell(1, 0).y + 2;
  s.root.mouseMove(Point{50, y}, 0);
  s.root.mouseDown(Point{50, y}, MouseButton::Left, 0);
  s.root.mouseUp(Point{50, y}, MouseButton::Left, 0);
  EXPECT_EQ(s.root.cursor(), CursorKind::Arrow);

  std::vector<std::size_t> picked;
  s.table->setOnRowSelect([&](std::size_t row) { picked.push_back(row); });
  s.root.mouseDown(Point{50, y}, MouseButton::Left, 0);
  s.root.mouseUp(Point{50, y}, MouseButton::Left, 0);
  EXPECT_EQ(picked, (std::vector<std::size_t>{1}));
  EXPECT_EQ(s.root.cursor(), CursorKind::Hand);
}

TEST(TableMainCell, DescriptionSitsTwoPixelsBelowTheText) {
  Root root{ThemeName::Dark, 1.0};
  auto created = std::make_unique<TableMainCell>();
  TableMainCell* cell = created.get();
  cell->setText("名称");
  cell->setDescription("说明");
  root.setContent(std::move(created));
  root.layout(Size{300, 100});
  cell->arrange(Rect{0, 0, 300, cell->measure(Size{300, 0}).height});
  EXPECT_DOUBLE_EQ(cell->children()[1]->bounds().y, cell->children()[0]->bounds().height + 2);
}
