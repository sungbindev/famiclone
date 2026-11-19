/*
 * ------------------------------------------------------------------------
 * 名称：tooltip-test.cpp
 * 说明：悬停提示的测试：延迟弹出、已有提示时立即弹出下一个、点击与按键后收起且不再弹出、空提示盖住外层提示、左右与上下的定位、小控件与大控件的锚点、换行与版式提示的尺寸、提示不拦截鼠标。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：定时器由测试用 Root::fireTimer 手动触发；窗口 400×300。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "rujian/ui/button.hpp"
#include "rujian/ui/root.hpp"
#include "rujian/ui/tooltip.hpp"

using namespace rujian::ui;

namespace {

/** 按固定位置摆放子控件的容器。 */
class Canvas2D final : public Widget {
 public:
  void place(Widget* child, Rect rect) { rects_.emplace_back(child, rect); }
  void arrange(const Rect& bounds) override {
    Widget::arrange(bounds);
    for (const auto& [child, rect] : rects_) child->arrange(rect);
  }

 private:
  std::vector<std::pair<Widget*, Rect>> rects_;
};

/** 一个固定大小的块（大控件用）。 */
class Block final : public Widget {};

struct Scene {
  Root root{ThemeName::Dark, 1.0};
  Tooltip tip{root};
  Canvas2D* page{nullptr};
  Button* first{nullptr};
  Button* second{nullptr};
  Block* block{nullptr};

  Scene() {
    auto holder = std::make_unique<Canvas2D>();
    page = holder.get();
    auto a = std::make_unique<Button>("甲");
    first = a.get();
    auto b = std::make_unique<Button>("乙");
    second = b.get();
    auto c = std::make_unique<Block>();
    block = c.get();
    holder->addChild(std::move(a));
    holder->addChild(std::move(b));
    holder->addChild(std::move(c));
    holder->place(first, Rect{20, 100, 60, 24});
    holder->place(second, Rect{20, 160, 60, 24});
    holder->place(block, Rect{150, 20, 200, 120});
    root.setContent(std::move(holder));
    first->setTooltip("第一个提示");
    second->setTooltip("第二个提示");
    root.layout(Size{400, 300});
  }

  static Point center(const Rect& rect) { return Point{rect.x + rect.width / 2, rect.y + rect.height / 2}; }

  void hover(const Widget& widget) { root.mouseMove(center(widget.bounds()), 0); }

  /** 触发等待中的提示定时器。 */
  void wait() {
    for (const unsigned id : root.pendingTimers()) root.fireTimer(id);
  }
};

}  // namespace

TEST(Tooltip, AppearsOnlyAfterTheDelay) {
  Scene s;
  s.hover(*s.first);
  EXPECT_FALSE(s.tip.shown());
  EXPECT_EQ(s.root.pendingTimers().size(), 1u);
  s.wait();
  EXPECT_TRUE(s.tip.shown());
}

TEST(Tooltip, SitsRightOfASmallTargetWithAGapAndCentersOnIt) {
  Scene s;
  s.hover(*s.first);
  s.wait();
  const Rect target = s.first->bounds();
  EXPECT_EQ(s.tip.side(), Tooltip::Side::Right);
  EXPECT_DOUBLE_EQ(s.tip.bounds().x, target.right() + 10.0);
  EXPECT_NEAR(s.tip.bounds().y + s.tip.bounds().height / 2, target.y + target.height / 2, 0.01);
  EXPECT_NEAR(s.tip.arrowY(), target.y + target.height / 2, 0.01);
}

TEST(Tooltip, PlainTooltipHasTheTokenPadding) {
  Scene s;
  s.hover(*s.first);
  s.wait();
  const TextMetrics text = s.root.measureText("第一个提示", TextStyle{.fontSize = 12, .lineHeight = 1.45}, 0);
  EXPECT_NEAR(s.tip.bounds().width, text.width + 2 * 8.0 + 2.0, 0.6);
  EXPECT_NEAR(s.tip.bounds().height, text.height + 2 * 4.0 + 2.0, 0.6);
}

TEST(Tooltip, MovingToAnotherTargetWhileShownAppearsImmediately) {
  Scene s;
  s.hover(*s.first);
  s.wait();
  s.hover(*s.second);
  EXPECT_TRUE(s.tip.shown());
  EXPECT_TRUE(s.root.pendingTimers().empty());
  EXPECT_NEAR(s.tip.bounds().y + s.tip.bounds().height / 2, s.second->bounds().y + 12, 0.01);
}

TEST(Tooltip, MovingToATargetWithoutTooltipHidesIt) {
  Scene s;
  s.hover(*s.first);
  s.wait();
  s.root.mouseMove(Point{300, 250}, 0);
  EXPECT_FALSE(s.tip.shown());
  EXPECT_TRUE(s.root.pendingTimers().empty());
}

TEST(Tooltip, LeavingTheWindowHidesIt) {
  Scene s;
  s.hover(*s.first);
  s.wait();
  s.root.mouseLeave();
  EXPECT_FALSE(s.tip.shown());
}

TEST(Tooltip, PressingHidesItAndItStaysHiddenUntilTheMouseLeavesTheTarget) {
  Scene s;
  s.hover(*s.first);
  s.wait();
  const Point inside = Scene::center(s.first->bounds());
  s.root.mouseDown(inside, MouseButton::Left, 0);
  s.root.mouseUp(inside, MouseButton::Left, 0);
  EXPECT_FALSE(s.tip.shown());
  s.root.mouseMove(Point{inside.x + 3, inside.y}, 0);
  EXPECT_TRUE(s.root.pendingTimers().empty());
  s.root.mouseMove(Point{300, 250}, 0);
  s.hover(*s.first);
  EXPECT_EQ(s.root.pendingTimers().size(), 1u);
}

TEST(Tooltip, KeyPressHidesIt) {
  Scene s;
  s.hover(*s.first);
  s.wait();
  s.root.keyDown(0x41, 0, false);
  EXPECT_FALSE(s.tip.shown());
}

TEST(Tooltip, WheelHidesIt) {
  Scene s;
  s.hover(*s.first);
  s.wait();
  s.root.mouseWheel(Scene::center(s.first->bounds()), -1, 0);
  EXPECT_FALSE(s.tip.shown());
}

TEST(Tooltip, AnEmptyTooltipCoversTheOuterOne) {
  Scene s;
  s.page->setTooltip("外层提示");
  s.first->setTooltip("");
  s.hover(*s.first);
  EXPECT_TRUE(s.root.pendingTimers().empty());
  s.hover(*s.second);
  EXPECT_EQ(s.root.pendingTimers().size(), 1u);
}

TEST(Tooltip, OuterTooltipAppliesToChildrenWithoutTheirOwn) {
  Scene s;
  s.page->setTooltip("外层提示");
  s.second->clearTooltip();
  s.hover(*s.second);
  s.wait();
  EXPECT_TRUE(s.tip.shown());
}

TEST(Tooltip, GoesToTheLeftWhenThereIsNoRoomOnTheRight) {
  Scene s;
  s.page->place(s.second, Rect{330, 160, 60, 24});
  s.root.layout(Size{400, 300});
  s.hover(*s.second);
  s.wait();
  EXPECT_EQ(s.tip.side(), Tooltip::Side::Left);
  EXPECT_DOUBLE_EQ(s.tip.bounds().right(), s.second->bounds().x - 10.0);
}

TEST(Tooltip, StaysInsideTheWindowVertically) {
  Scene s;
  s.page->place(s.first, Rect{20, 2, 60, 24});
  s.root.layout(Size{400, 300});
  s.hover(*s.first);
  s.wait();
  EXPECT_GE(s.tip.bounds().y, 8.0);
  EXPECT_GE(s.tip.arrowY(), s.tip.bounds().y + 10.0);
}

TEST(Tooltip, LargeTargetsAlignWithThePointer) {
  Scene s;
  s.block->setTooltip("块的提示");
  const Point pointer{200, 40};
  s.root.mouseMove(pointer, 0);
  s.wait();
  EXPECT_DOUBLE_EQ(s.tip.bounds().x, pointer.x + 12.0 + 10.0);
  EXPECT_NEAR(s.tip.arrowY(), pointer.y + 9.0, 0.01);
}

TEST(Tooltip, LongTextWrapsAtTheMaximumWidth) {
  Scene s;
  s.first->setTooltip(std::string(200, 'a') + " " + std::string(200, 'b'));
  s.hover(*s.first);
  s.wait();
  EXPECT_LE(s.tip.bounds().width, 320.0 + 0.001);
  EXPECT_GT(s.tip.bounds().height, 30.0);
}

TEST(Tooltip, NewLinesStayLineBreaks) {
  Scene s;
  s.first->setTooltip("一行");
  s.hover(*s.first);
  s.wait();
  const double single = s.tip.bounds().height;
  s.root.mouseLeave();
  s.first->setTooltip("一行\n二行");
  s.hover(*s.first);
  s.wait();
  EXPECT_GT(s.tip.bounds().height, single + 10.0);
}

TEST(Tooltip, RichTooltipIsTallerThanPlainAndHasTheHeadAndItems) {
  Scene s;
  s.first->setRichTooltip(TooltipContent{.title = "标题", .icon = IconId::InfoCircle, .items = {TooltipItem{.icon = IconId::Eye, .title = "小标题", .text = "正文说明"}, TooltipItem{.text = "第二条"}}});
  s.hover(*s.first);
  s.wait();
  const Rect rich = s.tip.bounds();
  EXPECT_GT(rich.height, 60.0);
  EXPECT_LE(rich.width, 380.0 + 0.001);
}

TEST(Tooltip, TheLayerDoesNotInterceptTheMouse) {
  Scene s;
  s.hover(*s.first);
  s.wait();
  const Point onTip = Scene::center(s.tip.bounds());
  s.root.mouseMove(onTip, 0);
  // 鼠标移到提示上，等于移到了下面的页面上：不属于带提示的控件，提示收起
  EXPECT_FALSE(s.tip.shown());
}

TEST(Tooltip, RemovingTheTargetWhileShownHidesIt) {
  Scene s;
  s.hover(*s.first);
  s.wait();
  s.page->removeChild(s.first);
  EXPECT_FALSE(s.tip.shown());
}
