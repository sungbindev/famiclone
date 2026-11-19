/*
 * ------------------------------------------------------------------------
 * 名称：scroll-area-test.cpp
 * 说明：滚动区域的测试：滚轮与键盘滚动、滑块长度与位置、拖动与点击轨道、滚动条的显示与渐隐（用手动触发的定时器）、焦点控件滚到可见。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：滚动区域放在 (10,10)，高 100 宽 100，内容高 300；轨道在 x 从 100 到 110，滑块长 33。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <memory>

#include "rujian/ui/button.hpp"
#include "rujian/ui/layout.hpp"
#include "rujian/ui/root.hpp"
#include "rujian/ui/scroll-area.hpp"

using namespace rujian::ui;

namespace {

constexpr int kVkTab = 0x09;
constexpr int kVkPageDown = 0x22;
constexpr int kVkEnd = 0x23;
constexpr int kVkHome = 0x24;
constexpr int kVkUp = 0x26;
constexpr int kVkDown = 0x28;

/** 固定高度的内容。 */
class Block final : public Widget {
 public:
  explicit Block(double height) : height_(height) {}
  Size measure(Size available) override { return Size{available.width, height_}; }

 private:
  double height_;
};

class Holder final : public Widget {
 public:
  void arrange(const Rect& bounds) override {
    Widget::arrange(bounds);
    for (const auto& child : children()) {
      const Size size = child->measure(Size{100, 0});
      child->arrange(Rect{10, 10, size.width, size.height});
    }
  }
};

struct Scene {
  Root root{ThemeName::Dark, 1.0};
  ScrollArea* area{nullptr};

  explicit Scene(bool animated = false, double contentHeight = 300) {
    auto holder = std::make_unique<Holder>();
    auto created = std::make_unique<ScrollArea>();
    area = created.get();
    area->setMaxHeight(100);
    area->setContent(std::make_unique<Block>(contentHeight));
    holder->addChild(std::move(created));
    root.setContent(std::move(holder));
    root.setAnimationsEnabled(animated);
    root.layout(Size{200, 200});
  }

  void fireAll() {
    for (const unsigned id : root.pendingTimers()) root.fireTimer(id);
  }
};

}  // namespace

TEST(ScrollArea, MeasureStopsAtTheMaxHeightAndSizesFromContentBelowIt) {
  Scene s;
  EXPECT_DOUBLE_EQ(s.area->measure(Size{100, 0}).height, 100);
  Scene small(false, 40);
  EXPECT_DOUBLE_EQ(small.area->measure(Size{100, 0}).height, 40);
}

TEST(ScrollArea, WheelScrollsContentAndClampsAtTheEnds) {
  Scene s;
  s.root.mouseWheel(Point{50, 50}, -1.0, 0);
  EXPECT_DOUBLE_EQ(s.area->scrollTop(), 50);
  EXPECT_DOUBLE_EQ(s.area->content()->bounds().y, 10 - s.area->scrollTop());
  s.root.mouseWheel(Point{50, 50}, -1000.0, 0);
  EXPECT_DOUBLE_EQ(s.area->scrollTop(), 200);
  s.root.mouseWheel(Point{50, 50}, 1000.0, 0);
  EXPECT_DOUBLE_EQ(s.area->scrollTop(), 0);
}

TEST(ScrollArea, ContentThatFitsDoesNotScrollAndLeavesTheWheelToParents) {
  Scene s(false, 60);
  s.root.mouseWheel(Point{50, 50}, -1.0, 0);
  EXPECT_DOUBLE_EQ(s.area->scrollTop(), 0);
  EXPECT_DOUBLE_EQ(s.area->maxScrollTop(), 0);
  EXPECT_DOUBLE_EQ(s.area->sliderRect().height, 0);
}

TEST(ScrollArea, SliderLengthAndPositionFollowTheScrollPosition) {
  Scene s;
  Rect slider = s.area->sliderRect();
  EXPECT_DOUBLE_EQ(slider.height, 33);
  EXPECT_DOUBLE_EQ(slider.y, 10);
  EXPECT_DOUBLE_EQ(slider.x, 102);
  EXPECT_DOUBLE_EQ(slider.width, 6);
  s.area->setScrollTop(200);
  slider = s.area->sliderRect();
  EXPECT_DOUBLE_EQ(slider.y, 10 + 67);
}

TEST(ScrollArea, SliderHasAMinimumLength) {
  Scene s(false, 10000);
  EXPECT_DOUBLE_EQ(s.area->sliderRect().height, 20);
}

TEST(ScrollArea, DraggingTheSliderScrollsProportionally) {
  Scene s;
  s.root.mouseMove(Point{50, 50}, 0);
  const Rect slider = s.area->sliderRect();
  const Point grab{slider.x + 3, slider.y + 10};
  s.root.mouseMove(grab, 0);
  s.root.mouseDown(grab, MouseButton::Left, 0);
  s.root.mouseMove(Point{grab.x, grab.y + 33.5}, 0);
  EXPECT_NEAR(s.area->scrollTop(), 100, 2);
  s.root.mouseMove(Point{grab.x + 80, grab.y + 500}, 0);
  EXPECT_DOUBLE_EQ(s.area->scrollTop(), 200);
  s.root.mouseUp(Point{grab.x, grab.y + 500}, MouseButton::Left, 0);
  s.root.mouseMove(Point{grab.x, grab.y}, 0);
  EXPECT_DOUBLE_EQ(s.area->scrollTop(), 200) << "松开之后不再跟随";
}

TEST(ScrollArea, PressingOnTheTrackJumpsTheSliderCenterToThePointer) {
  Scene s;
  s.root.mouseMove(Point{50, 50}, 0);
  s.root.mouseMove(Point{105, 80}, 0);
  s.root.mouseDown(Point{105, 80}, MouseButton::Left, 0);
  const Rect slider = s.area->sliderRect();
  EXPECT_NEAR(slider.y + slider.height / 2, 80, 1);
  s.root.mouseUp(Point{105, 80}, MouseButton::Left, 0);
}

TEST(ScrollArea, KeysScrollWhenAFocusedControlDoesNotHandleThem) {
  Scene s;
  s.root.mouseWheel(Point{50, 50}, 0.0, 0);
  s.root.keyDown(kVkDown, 0, false);
  EXPECT_DOUBLE_EQ(s.area->scrollTop(), 0) << "没有焦点时不处理按键";
}

TEST(ScrollArea, FocusedChildIsScrolledIntoView) {
  Root root(ThemeName::Dark, 1.0);
  auto holder = std::make_unique<Holder>();
  auto area = std::make_unique<ScrollArea>();
  ScrollArea* raw = area.get();
  raw->setMaxHeight(100);
  auto stack = std::make_unique<Stack>();
  Button* last = nullptr;
  for (int i = 0; i < 8; ++i) {
    auto button = std::make_unique<Button>("按钮");
    last = button.get();
    stack->addChild(std::move(button));
  }
  raw->setContent(std::move(stack));
  holder->addChild(std::move(area));
  root.setContent(std::move(holder));
  root.setAnimationsEnabled(false);
  root.layout(Size{200, 200});
  EXPECT_DOUBLE_EQ(raw->scrollTop(), 0);
  root.setFocus(last, true);
  EXPECT_GT(raw->scrollTop(), 0);
  EXPECT_LE(last->bounds().bottom(), raw->bounds().bottom() + 0.001);
  EXPECT_GE(last->bounds().y, raw->bounds().y - 0.001);
}

TEST(ScrollArea, ScrollbarShowsOnHoverAndHidesWhenTheMouseLeavesWithoutAnimation) {
  Scene s;
  EXPECT_DOUBLE_EQ(s.area->scrollbarOpacity(), 0);
  s.root.mouseMove(Point{50, 50}, 0);
  EXPECT_DOUBLE_EQ(s.area->scrollbarOpacity(), 1);
  s.root.mouseMove(Point{180, 180}, 0);
  EXPECT_DOUBLE_EQ(s.area->scrollbarOpacity(), 0);
}

TEST(ScrollArea, ScrollbarFadesInAndOutWithAnimationAndWaitsBeforeHiding) {
  Scene s(true);
  s.root.mouseMove(Point{50, 50}, 0);
  EXPECT_DOUBLE_EQ(s.area->scrollbarOpacity(), 0) << "渐显要靠定时器推进";
  for (int i = 0; i < 10 && s.area->scrollbarOpacity() < 1; ++i) s.fireAll();
  EXPECT_DOUBLE_EQ(s.area->scrollbarOpacity(), 1);
  s.root.mouseMove(Point{180, 180}, 0);
  s.fireAll();  // 动画定时器与等待隐藏的定时器各触发一次
  s.fireAll();
  for (int i = 0; i < 100 && s.area->scrollbarOpacity() > 0; ++i) s.fireAll();
  EXPECT_DOUBLE_EQ(s.area->scrollbarOpacity(), 0);
  EXPECT_TRUE(s.root.pendingTimers().empty());
}

TEST(ScrollArea, ScrollbarDoesNotReceiveTheMouseWhileItIsStillInvisible) {
  Scene s(true);
  // 渐显还没开始推进（不透明度为 0）时，轨道区域不响应鼠标
  s.root.mouseMove(Point{105, 80}, 0);
  s.root.mouseDown(Point{105, 80}, MouseButton::Left, 0);
  s.root.mouseUp(Point{105, 80}, MouseButton::Left, 0);
  EXPECT_DOUBLE_EQ(s.area->scrollTop(), 0);
}

TEST(ScrollArea, DestroyingWhileATimerIsPendingIsSafe) {
  auto scene = std::make_unique<Scene>(true);
  scene->root.mouseMove(Point{50, 50}, 0);
  ASSERT_FALSE(scene->root.pendingTimers().empty());
  scene.reset();
  SUCCEED();
}
