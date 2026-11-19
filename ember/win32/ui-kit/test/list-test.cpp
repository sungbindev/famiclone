/*
 * ------------------------------------------------------------------------
 * 名称：list-test.cpp
 * 说明：分隔线列表的测试：行之间留分割线的高度、行拉伸到列表宽度、去首尾内边距、放进面板内容区后自动去掉贴边的内边距。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：缩放 1.0，分割线 1；设置行上下内边距各 8，控件高 22。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <memory>

#include "rujian/ui/list.hpp"
#include "rujian/ui/panel.hpp"
#include "rujian/ui/root.hpp"
#include "rujian/ui/setting-row.hpp"

using namespace rujian::ui;

namespace {

/** 固定高度的内容。 */
class Block final : public Widget {
 public:
  explicit Block(double height) : height_(height) {}
  Size measure(Size available) override { return Size{available.width, height_}; }

 private:
  double height_;
};

std::unique_ptr<SettingRow> makeRow() {
  auto row = std::make_unique<SettingRow>();
  row->setLabel("名");
  row->setControl(std::make_unique<Block>(22));
  return row;
}

std::unique_ptr<List> makeList(int count) {
  auto list = std::make_unique<List>();
  for (int i = 0; i < count; ++i) list->addRow(makeRow());
  return list;
}

}  // namespace

TEST(List, RowsAreSeparatedByADividerLine) {
  Root root{ThemeName::Dark, 1.0};
  auto list = makeList(3);
  List* raw = list.get();
  root.setContent(std::move(list));
  root.layout(Size{800, 400});
  const double row = 22 + 16;
  EXPECT_DOUBLE_EQ(raw->measure(Size{800, 0}).height, 3 * row + 2);
  EXPECT_DOUBLE_EQ(raw->children()[1]->bounds().y, row + 1);
  EXPECT_DOUBLE_EQ(raw->children()[2]->bounds().y, 2 * row + 2);
  EXPECT_DOUBLE_EQ(raw->children()[0]->bounds().width, 800);
}

TEST(List, TrimEndsRemovesThePaddingOfTheFirstAndLastRow) {
  Root root{ThemeName::Dark, 1.0};
  auto list = makeList(3);
  List* raw = list.get();
  raw->setTrimEnds(true, true);
  root.setContent(std::move(list));
  root.layout(Size{800, 400});
  EXPECT_DOUBLE_EQ(raw->measure(Size{800, 0}).height, 22 + 8 + 1 + 38 + 1 + 22 + 8);
}

TEST(List, RowsAddedLaterStillFollowTheTrim) {
  Root root{ThemeName::Dark, 1.0};
  auto list = std::make_unique<List>();
  List* raw = list.get();
  raw->setTrimEnds(true, true);
  raw->addRow(makeRow());
  root.setContent(std::move(list));
  root.layout(Size{800, 400});
  EXPECT_DOUBLE_EQ(raw->measure(Size{800, 0}).height, 22);
}

TEST(Panel, AListBodyLosesItsPaddingNextToThePanelEdges) {
  Root root{ThemeName::Dark, 1.0};
  auto panel = std::make_unique<Panel>();
  Panel* raw = panel.get();
  panel->addContent(makeList(2));
  root.setContent(std::move(panel));
  root.layout(Size{800, 400});
  // 面板内容区的内边距 10 + 两行（第一行去上内边距、最后一行去下内边距）+ 分割线 + 边框 2
  EXPECT_DOUBLE_EQ(raw->measure(Size{800, 0}).height, 2 + 10 + (22 + 8) + 1 + (8 + 22) + 10);
}
