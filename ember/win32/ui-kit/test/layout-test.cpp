/*
 * ------------------------------------------------------------------------
 * 名称：layout-test.cpp
 * 说明：弹性布局的测试：纵向拉伸与间距、横向居中、换行、grow、内边距、不可见控件不占位、有固定高度的控件不拉伸。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：用固定尺寸的方块做子控件，不依赖字体。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <memory>

#include "base/theme-tokens.hpp"
#include "rujian/ui/layout.hpp"
#include "rujian/ui/root.hpp"

using namespace rujian::ui;

namespace {

/** 固定尺寸的方块；fixedHeight 为真时不接受交叉轴拉伸的高度。 */
class Block final : public Widget {
 public:
  Block(double width, double height, bool fixedHeight = false) : size_(width, height), fixedHeight_(fixedHeight) {}
  [[nodiscard]] Size measure(Size) override { return size_; }
  [[nodiscard]] bool stretchesHeight() const override { return !fixedHeight_; }

 private:
  Size size_;
  bool fixedHeight_;
};

/** 把容器放进 Root 并布局，返回容器指针。 */
template <typename Container>
Container* attach(Root& root, std::unique_ptr<Container> container, Size surface) {
  Container* raw = container.get();
  root.setContent(std::move(container));
  root.layout(surface);
  return raw;
}

Block* add(Widget& parent, double width, double height, bool fixedHeight = false) {
  auto block = std::make_unique<Block>(width, height, fixedHeight);
  Block* raw = block.get();
  parent.addChild(std::move(block));
  return raw;
}

}  // namespace

TEST(Layout, StackStretchesChildrenToWidthAndSpacesThemByPageGap) {
  Root root(ThemeName::Dark, 1.0);
  auto* stack = attach(root, std::make_unique<Stack>(), Size{200, 100});
  Block* first = add(*stack, 50, 20);
  Block* second = add(*stack, 80, 30);
  root.layout(Size{200, 100});
  EXPECT_EQ(first->bounds(), (Rect{0, 0, 200, 20}));
  EXPECT_EQ(second->bounds(), (Rect{0, 30, 200, 30}));
}

TEST(Layout, StackMeasureIsWidestChildAndSumOfHeightsWithGaps) {
  Root root(ThemeName::Dark, 1.0);
  auto* stack = attach(root, std::make_unique<Stack>(), Size{200, 100});
  add(*stack, 50, 20);
  add(*stack, 80, 30);
  EXPECT_EQ(stack->measure({}).width, 80);
  EXPECT_EQ(stack->measure({}).height, 20 + 30 + tokens::space_page);
}

TEST(Layout, StackPaddingInsetsContent) {
  Root root(ThemeName::Dark, 1.0);
  auto* stack = attach(root, std::make_unique<Stack>(), Size{200, 100});
  stack->setPadding(Insets::all(16));
  Block* child = add(*stack, 10, 20);
  root.layout(Size{200, 100});
  EXPECT_EQ(child->bounds(), (Rect{16, 16, 168, 20}));
  EXPECT_EQ(stack->measure({}), (Size{10 + 32, 20 + 32}));
}

TEST(Layout, RowCentersChildrenVerticallyWithMediumGap) {
  Root root(ThemeName::Dark, 1.0);
  auto* row = attach(root, std::make_unique<Row>(), Size{300, 50});
  Block* tall = add(*row, 40, 22, true);
  Block* short_ = add(*row, 30, 10, true);
  root.layout(Size{300, 50});
  EXPECT_EQ(tall->bounds(), (Rect{0, 0, 40, 22}));
  EXPECT_EQ(short_->bounds(), (Rect{40 + tokens::space_md, 6, 30, 10}));
}

TEST(Layout, RowWrapsToNextLineWhenWidthIsNotEnough) {
  Root root(ThemeName::Dark, 1.0);
  auto* row = attach(root, std::make_unique<Row>(), Size{100, 100});
  Block* a = add(*row, 60, 20, true);
  Block* b = add(*row, 60, 20, true);
  Block* c = add(*row, 30, 20, true);
  root.layout(Size{100, 100});
  EXPECT_EQ(a->bounds().y, 0);
  EXPECT_EQ(b->bounds(), (Rect{0, 20 + tokens::space_md, 60, 20}));
  EXPECT_EQ(c->bounds().x, 60 + tokens::space_md);
  EXPECT_EQ(c->bounds().y, b->bounds().y);
}

TEST(Layout, RowMeasureWithLimitedWidthCountsWrappedLines) {
  Root root(ThemeName::Dark, 1.0);
  auto* row = attach(root, std::make_unique<Row>(), Size{300, 100});
  add(*row, 60, 20, true);
  add(*row, 60, 20, true);
  EXPECT_EQ(row->measure({}), (Size{60 + tokens::space_md + 60, 20}));
  EXPECT_EQ(row->measure(Size{100, 0}), (Size{60, 20 + tokens::space_md + 20}));
}

TEST(Layout, InvisibleChildrenTakeNoSpace) {
  Root root(ThemeName::Dark, 1.0);
  auto* row = attach(root, std::make_unique<Row>(), Size{300, 50});
  add(*row, 40, 22, true);
  Block* hidden = add(*row, 100, 22, true);
  Block* last = add(*row, 30, 22, true);
  hidden->setVisible(false);
  root.layout(Size{300, 50});
  EXPECT_EQ(last->bounds().x, 40 + tokens::space_md);
}

TEST(Layout, GrowDistributesRemainingMainAxisSpace) {
  Root root(ThemeName::Dark, 1.0);
  auto* actions = attach(root, std::make_unique<Actions>(), Size{200, 50});
  Block* fixed = add(*actions, 50, 20, true);
  Block* grows = add(*actions, 20, 20, true);
  actions->setGrow(grows, 1);
  root.layout(Size{200, 50});
  EXPECT_EQ(fixed->bounds().width, 50);
  EXPECT_EQ(grows->bounds().width, 200 - 50 - tokens::action_gap);
}

TEST(Layout, WrapStretchesChildHeightToLineButNotFixedHeightOnes) {
  Root root(ThemeName::Dark, 1.0);
  auto* wrap = attach(root, std::make_unique<Wrap>(), Size{300, 100});
  Block* tall = add(*wrap, 40, 40);
  Block* stretched = add(*wrap, 40, 10);
  Block* fixed = add(*wrap, 40, 10, true);
  root.layout(Size{300, 100});
  EXPECT_EQ(tall->bounds().height, 40);
  EXPECT_EQ(stretched->bounds().height, 40);
  EXPECT_EQ(fixed->bounds().height, 10);
  EXPECT_EQ(fixed->bounds().y, 0);
}

TEST(Layout, EmptyContainerMeasuresZeroAndArrangesWithoutChildren) {
  Root root(ThemeName::Dark, 1.0);
  auto* stack = attach(root, std::make_unique<Stack>(), Size{100, 100});
  EXPECT_EQ(stack->measure({}), (Size{0, 0}));
  stack->arrange(Rect{0, 0, 100, 100});
  EXPECT_EQ(stack->bounds(), (Rect{0, 0, 100, 100}));
}
