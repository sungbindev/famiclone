/*
 * ------------------------------------------------------------------------
 * 名称：row-list-test.cpp
 * 说明：信息行与工具栏的测试：行高下限、各列位置与宽度（前导 96、列间距 10、主文字占满剩余）、带图标与指定宽度的前导、行垂直居中；工具栏高度、操作靠右、放不下时换行。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：缩放 1.0；信息行子控件顺序是“前导、主文字、各信息列、操作”；工具栏子控件顺序是“说明、前导、操作”。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "rujian/ui/root.hpp"
#include "rujian/ui/row-list.hpp"
#include "rujian/ui/toolbar.hpp"

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

template <typename T>
struct Scene {
  Root root{ThemeName::Dark, 1.0};
  T* widget{nullptr};

  Scene() {
    auto created = std::make_unique<T>();
    widget = created.get();
    root.setContent(std::move(created));
  }

  void layout(double width) {
    root.layout(Size{width, 300});
    widget->arrange(Rect{0, 0, width, widget->measure(Size{width, 0}).height});
  }

  [[nodiscard]] const Rect& at(std::size_t index) const { return widget->children()[index]->bounds(); }
};

}  // namespace

TEST(RowListItem, HeightHasAMinimumAndColumnsAreLaidOutInOrder) {
  Scene<RowListItem> s;
  s.widget->setLeading("前导");
  s.widget->setTitle("标题");
  s.widget->addCell({"信息", 64, true});
  s.widget->setActions(std::make_unique<Block>(60, 22));
  s.layout(400);
  EXPECT_DOUBLE_EQ(s.widget->bounds().height, 26);
  // 子控件顺序：前导、主文字、信息列、操作
  EXPECT_DOUBLE_EQ(s.at(0).x, 10);
  EXPECT_DOUBLE_EQ(s.at(0).width, 96);
  EXPECT_DOUBLE_EQ(s.at(1).x, 10 + 96 + 10);
  EXPECT_DOUBLE_EQ(s.at(2).width, 64);
  EXPECT_DOUBLE_EQ(s.at(3).x, 400 - 10 - 60);
  EXPECT_DOUBLE_EQ(s.at(3).y, (26 - 22) / 2.0);
  // 主文字占满中间剩余的宽度
  EXPECT_DOUBLE_EQ(s.at(1).width, 400 - 20 - 96 - 64 - 60 - 3 * 10);
}

TEST(RowListItem, LeadingWithAnIconAndAWidthPutsTheTextAfterTheIcon) {
  Scene<RowListItem> s;
  s.widget->setLeading({"设备", IconId::DeviceFloppy, 60, true});
  s.widget->setTitle("标题");
  s.layout(400);
  // 前导列取指定的宽度，文字排在图标右边（图标 14 加间距 4），主文字紧跟前导列
  EXPECT_DOUBLE_EQ(s.at(0).x, 10 + 14 + 4);
  EXPECT_DOUBLE_EQ(s.at(0).width, 60 - 14 - 4);
  EXPECT_DOUBLE_EQ(s.at(1).x, 10 + 60 + 10);
}

TEST(RowListItem, TallContentGrowsTheRow) {
  Scene<RowListItem> s;
  s.widget->setTitle("标题");
  s.widget->setActions(std::make_unique<Block>(60, 40));
  s.layout(400);
  EXPECT_DOUBLE_EQ(s.widget->bounds().height, 40 + 4);
}

TEST(RowListItem, ChangingAnAutoWidthCellRearrangesTheRow) {
  Scene<RowListItem> s;
  s.widget->setTitle("标题");
  s.widget->addCell({"短", 0, false});
  s.widget->setActions(std::make_unique<Block>(60, 22));
  s.layout(400);
  const double before = s.at(1).width;
  s.widget->setCellText(0, "读写中（磁道 12）");
  EXPECT_LT(s.at(1).width, before);
  EXPECT_GT(s.at(2).width, 40);
  // 行尾操作的位置不变
  EXPECT_DOUBLE_EQ(s.at(3).x, 400 - 10 - 60);
}

TEST(RowListItem, AClickableCellShowsAHandAndCallsBackWithItsAreaWhenClicked) {
  Scene<RowListItem> s;
  s.widget->setTitle("标题");
  s.widget->addCell({"1:23 / 4:56", 72, true});
  s.widget->addCell({"已停止", 108, true});
  std::vector<Rect> clicks;
  s.widget->setCellAction(0, [&clicks](const Rect& area) { clicks.push_back(area); });
  s.layout(400);
  const Rect text = s.at(2);
  const Point inside{text.x + 5, text.y + text.height / 2};
  const Point other{s.at(3).x + 5, text.y + text.height / 2};

  s.root.mouseMove(inside, 0);
  EXPECT_EQ(s.root.cursor(), CursorKind::Hand) << "可点击的列上是手形";
  s.root.mouseMove(other, 0);
  EXPECT_EQ(s.root.cursor(), CursorKind::Arrow) << "没有设置点击的列上不是手形";
  s.root.mouseMove(inside, 0);
  s.root.mouseDown(inside, MouseButton::Left, 0);
  EXPECT_TRUE(clicks.empty()) << "按下时还不回调";
  s.root.mouseUp(inside, MouseButton::Left, 0);
  ASSERT_EQ(clicks.size(), 1U);
  // 点击区域比文字多出左右各 4 像素，高度贴着行
  EXPECT_DOUBLE_EQ(clicks[0].x, text.x - 4);
  EXPECT_DOUBLE_EQ(clicks[0].width, text.width + 8);
  EXPECT_DOUBLE_EQ(clicks[0].y, s.widget->bounds().y + 2);
}

TEST(RowListItem, PressingOnACellAndReleasingElsewhereIsNotAClick) {
  Scene<RowListItem> s;
  s.widget->setTitle("标题");
  s.widget->addCell({"1:23", 72, true});
  int clicks = 0;
  s.widget->setCellAction(0, [&clicks](const Rect&) { ++clicks; });
  s.layout(400);
  const Rect text = s.at(2);
  const Point inside{text.x + 5, text.y + text.height / 2};
  s.root.mouseMove(inside, 0);
  s.root.mouseDown(inside, MouseButton::Left, 0);
  s.root.mouseUp(Point{20, text.y + text.height / 2}, MouseButton::Left, 0);
  EXPECT_EQ(clicks, 0);
}

TEST(RowListItem, ADisabledOrClearedCellActionDoesNothing) {
  Scene<RowListItem> s;
  s.widget->setTitle("标题");
  s.widget->addCell({"1:23", 72, true});
  int clicks = 0;
  s.widget->setCellAction(0, [&clicks](const Rect&) { ++clicks; });
  s.layout(400);
  const Rect text = s.at(2);
  const Point inside{text.x + 5, text.y + text.height / 2};
  s.widget->setCellActionEnabled(0, false);
  s.root.mouseMove(inside, 0);
  EXPECT_EQ(s.root.cursor(), CursorKind::Arrow);
  s.root.mouseDown(inside, MouseButton::Left, 0);
  s.root.mouseUp(inside, MouseButton::Left, 0);
  EXPECT_EQ(clicks, 0) << "禁用后点击无效";
  s.widget->setCellActionEnabled(0, true);
  s.widget->setCellAction(0, nullptr);
  s.root.mouseMove(inside, 0);
  s.root.mouseDown(inside, MouseButton::Left, 0);
  s.root.mouseUp(inside, MouseButton::Left, 0);
  EXPECT_EQ(clicks, 0) << "取消回调后点击无效";
}

TEST(RowList, RowsHaveNoGap) {
  Scene<RowList> s;
  auto first = std::make_unique<RowListItem>();
  first->setTitle("甲");
  auto second = std::make_unique<RowListItem>();
  second->setTitle("乙");
  s.widget->addChild(std::move(first));
  s.widget->addChild(std::move(second));
  s.layout(400);
  EXPECT_DOUBLE_EQ(s.at(1).y, 26);
  EXPECT_DOUBLE_EQ(s.widget->measure(Size{400, 0}).height, 52);
}

TEST(Toolbar, ActionsAreOnTheRightWhenTheyFit) {
  Scene<Toolbar> s;
  s.widget->setLeading(std::make_unique<Block>(96, 22));
  s.widget->setDescription("共 3 项");
  s.widget->setActions(std::make_unique<Block>(78, 22));
  s.layout(500);
  // 内容高 22 加下内边距 6 加分割线 1
  EXPECT_DOUBLE_EQ(s.widget->bounds().height, 29);
  EXPECT_DOUBLE_EQ(s.at(2).x, 500 - 78);
  EXPECT_DOUBLE_EQ(s.at(1).x, 0);
  EXPECT_DOUBLE_EQ(s.at(0).x, 96 + 8);
}

TEST(Toolbar, ActionsWrapToASecondLineWhenTheyDoNotFit) {
  Scene<Toolbar> s;
  s.widget->setLeading(std::make_unique<Block>(96, 22));
  s.widget->setDescription("共 3 项");
  s.widget->setActions(std::make_unique<Block>(78, 22));
  s.layout(200);
  EXPECT_DOUBLE_EQ(s.at(2).x, 0);
  EXPECT_DOUBLE_EQ(s.at(2).y, 22 + 8);
  EXPECT_DOUBLE_EQ(s.widget->bounds().height, 22 + 8 + 22 + 6 + 1);
}

TEST(Toolbar, HeightHasAMinimum) {
  Scene<Toolbar> s;
  s.widget->setDescription("共 3 项");
  s.layout(300);
  EXPECT_DOUBLE_EQ(s.widget->bounds().height, 28);
}
