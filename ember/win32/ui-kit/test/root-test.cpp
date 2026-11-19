/*
 * ------------------------------------------------------------------------
 * 名称：root-test.cpp
 * 说明：根容器与按钮的交互测试：悬停链、按下、焦点遍历、鼠标捕获、控件移除与按钮点击。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：事件都经 Root 的输入入口送入，和窗口收到系统消息后的路径一致。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <memory>
#include <utility>
#include <vector>

#include "rujian/ui/button.hpp"
#include "rujian/ui/root.hpp"

using rujian::ui::Button;
using rujian::ui::MouseButton;
using rujian::ui::Point;
using rujian::ui::Rect;
using rujian::ui::Root;
using rujian::ui::Size;
using rujian::ui::ThemeName;
using rujian::ui::Widget;

namespace {

constexpr int kVirtualKeyTab = 0x09;
constexpr int kVirtualKeyReturn = 0x0D;
constexpr int kVirtualKeySpace = 0x20;

/** 把子控件按固定的矩形摆放的容器，便于测试命中。 */
class Fixed final : public Widget {
 public:
  void place(Widget* child, const Rect& rect) { placements_.emplace_back(child, rect); }
  void arrange(const Rect& bounds) override {
    Widget::arrange(bounds);
    for (const auto& [child, rect] : placements_) child->arrange(rect);
  }

 private:
  std::vector<std::pair<Widget*, Rect>> placements_;
};

/** 三个并排的按钮：a 在 (0,0,50,22)，b 在 (60,0,50,22)，c 在 (120,0,50,22)。 */
struct Fixture {
  Root root{ThemeName::Dark, 1.0};
  Fixed* page{nullptr};
  Button* a{nullptr};
  Button* b{nullptr};
  Button* c{nullptr};
  int clicksA{0};
  int clicksB{0};

  Fixture() {
    auto content = std::make_unique<Fixed>();
    page = content.get();
    const auto add = [&](const char* label, double x) {
      auto button = std::make_unique<Button>(label);
      button->setAutoIcon(false);
      Button* raw = button.get();
      page->addChild(std::move(button));
      page->place(raw, Rect{x, 0, 50, 22});
      return raw;
    };
    a = add("a", 0);
    b = add("b", 60);
    c = add("c", 120);
    a->setOnClick([this] { ++clicksA; });
    b->setOnClick([this] { ++clicksB; });
    root.setContent(std::move(content));
    root.layout(Size{200, 22});
  }
};

}  // namespace

TEST(Root, HoverMarksTargetAndAncestorsAndClearsOnLeave) {
  Fixture f;
  f.root.mouseMove(Point{10, 10}, 0);
  EXPECT_TRUE(f.a->hovered());
  EXPECT_TRUE(f.page->hovered());
  EXPECT_FALSE(f.b->hovered());
  f.root.mouseMove(Point{70, 10}, 0);
  EXPECT_FALSE(f.a->hovered());
  EXPECT_TRUE(f.b->hovered());
  f.root.mouseLeave();
  EXPECT_FALSE(f.b->hovered());
  EXPECT_FALSE(f.page->hovered());
}

TEST(Root, MouseDownPressesAndFocusesWithoutFocusRing) {
  Fixture f;
  f.root.mouseMove(Point{10, 10}, 0);
  f.root.mouseDown(Point{10, 10}, MouseButton::Left, 0);
  EXPECT_TRUE(f.a->pressed());
  EXPECT_TRUE(f.a->focused());
  EXPECT_FALSE(f.a->focusVisible());
  f.root.mouseUp(Point{10, 10}, MouseButton::Left, 0);
  EXPECT_FALSE(f.a->pressed());
  EXPECT_EQ(f.clicksA, 1);
}

TEST(Root, ReleasingOutsideTheButtonDoesNotClick) {
  Fixture f;
  f.root.mouseDown(Point{10, 10}, MouseButton::Left, 0);
  f.root.mouseMove(Point{100, 10}, 0);
  f.root.mouseUp(Point{100, 10}, MouseButton::Left, 0);
  EXPECT_EQ(f.clicksA, 0);
  EXPECT_FALSE(f.a->pressed());
}

TEST(Root, ReleasingOverAnotherButtonOnlyClicksNothing) {
  Fixture f;
  f.root.mouseDown(Point{10, 10}, MouseButton::Left, 0);
  f.root.mouseUp(Point{70, 10}, MouseButton::Left, 0);
  EXPECT_EQ(f.clicksA, 0);
  EXPECT_EQ(f.clicksB, 0);
}

TEST(Root, RightButtonDoesNotPressOrClick) {
  Fixture f;
  f.root.mouseDown(Point{10, 10}, MouseButton::Right, 0);
  f.root.mouseUp(Point{10, 10}, MouseButton::Right, 0);
  EXPECT_FALSE(f.a->pressed());
  EXPECT_EQ(f.clicksA, 0);
}

TEST(Root, ClickingEmptyAreaClearsFocus) {
  Fixture f;
  f.root.mouseDown(Point{10, 10}, MouseButton::Left, 0);
  f.root.mouseUp(Point{10, 10}, MouseButton::Left, 0);
  ASSERT_TRUE(f.a->focused());
  f.root.mouseDown(Point{190, 10}, MouseButton::Left, 0);
  EXPECT_FALSE(f.a->focused());
  EXPECT_EQ(f.root.focusedWidget(), nullptr);
}

TEST(Root, TabMovesFocusForwardWithRingAndWraps) {
  Fixture f;
  f.root.keyDown(kVirtualKeyTab, 0, false);
  EXPECT_TRUE(f.a->focusVisible());
  f.root.keyDown(kVirtualKeyTab, 0, false);
  EXPECT_TRUE(f.b->focusVisible());
  EXPECT_FALSE(f.a->focused());
  f.root.keyDown(kVirtualKeyTab, 0, false);
  f.root.keyDown(kVirtualKeyTab, 0, false);
  EXPECT_TRUE(f.a->focused());
}

TEST(Root, ShiftTabMovesBackwardAndStartsFromLast) {
  Fixture f;
  f.root.keyDown(kVirtualKeyTab, rujian::ui::kModifierShift, false);
  EXPECT_TRUE(f.c->focused());
  f.root.keyDown(kVirtualKeyTab, rujian::ui::kModifierShift, false);
  EXPECT_TRUE(f.b->focused());
}

TEST(Root, TabSkipsDisabledAndHiddenControls) {
  Fixture f;
  f.b->setEnabled(false);
  f.root.keyDown(kVirtualKeyTab, 0, false);
  f.root.keyDown(kVirtualKeyTab, 0, false);
  EXPECT_TRUE(f.c->focused());
  f.c->setVisible(false);
  f.root.keyDown(kVirtualKeyTab, 0, false);
  EXPECT_TRUE(f.a->focused());
}

TEST(Root, DisablingTheFocusedControlClearsFocus) {
  Fixture f;
  f.root.keyDown(kVirtualKeyTab, 0, false);
  ASSERT_TRUE(f.a->focused());
  f.a->setEnabled(false);
  EXPECT_EQ(f.root.focusedWidget(), nullptr);
}

TEST(Root, DisabledButtonIgnoresMouse) {
  Fixture f;
  f.a->setEnabled(false);
  f.root.mouseDown(Point{10, 10}, MouseButton::Left, 0);
  f.root.mouseUp(Point{10, 10}, MouseButton::Left, 0);
  EXPECT_EQ(f.clicksA, 0);
  EXPECT_FALSE(f.a->pressed());
}

TEST(Root, EnterClicksOnKeyDownAndSpaceOnKeyUp) {
  Fixture f;
  f.root.keyDown(kVirtualKeyTab, 0, false);
  f.root.keyDown(kVirtualKeyReturn, 0, false);
  EXPECT_EQ(f.clicksA, 1);
  f.root.keyDown(kVirtualKeyReturn, 0, true);
  EXPECT_EQ(f.clicksA, 1);
  f.root.keyDown(kVirtualKeySpace, 0, false);
  EXPECT_EQ(f.clicksA, 1);
  f.root.keyUp(kVirtualKeySpace, 0);
  EXPECT_EQ(f.clicksA, 2);
  f.root.keyUp(kVirtualKeySpace, 0);
  EXPECT_EQ(f.clicksA, 2);
}

TEST(Root, SpaceDownThenFocusLossDoesNotClickOnLaterKeyUp) {
  Fixture f;
  f.root.keyDown(kVirtualKeyTab, 0, false);
  f.root.keyDown(kVirtualKeySpace, 0, false);
  f.root.keyDown(kVirtualKeyTab, 0, false);
  f.root.keyUp(kVirtualKeySpace, 0);
  EXPECT_EQ(f.clicksA, 0);
  EXPECT_EQ(f.clicksB, 0);
}

TEST(Root, RemovingFocusedHoveredControlClearsReferences) {
  Fixture f;
  f.root.mouseMove(Point{10, 10}, 0);
  f.root.keyDown(kVirtualKeyTab, 0, false);
  ASSERT_TRUE(f.a->focused());
  auto removed = f.page->removeChild(f.a);
  ASSERT_NE(removed, nullptr);
  EXPECT_EQ(f.root.focusedWidget(), nullptr);
  EXPECT_EQ(removed->host(), nullptr);
  // 移除后再移动鼠标、按键都不应访问已移除的控件
  f.root.mouseMove(Point{12, 10}, 0);
  f.root.keyDown(kVirtualKeyReturn, 0, false);
  EXPECT_EQ(f.clicksA, 0);
}

TEST(Root, DestroyingFocusedControlClearsFocus) {
  Fixture f;
  f.root.keyDown(kVirtualKeyTab, 0, false);
  ASSERT_TRUE(f.a->focused());
  f.page->removeChild(f.a).reset();
  EXPECT_EQ(f.root.focusedWidget(), nullptr);
}

TEST(Root, CaptureSendsMoveToCapturingControl) {
  struct Capturer final : Widget {
    int moves{0};
    bool onMouseMove(const rujian::ui::MouseEvent&) override {
      ++moves;
      return true;
    }
  };
  Fixture f;
  auto capturer = std::make_unique<Capturer>();
  Capturer* raw = capturer.get();
  f.page->addChild(std::move(capturer));
  f.page->place(raw, Rect{0, 0, 0, 0});
  f.root.layout(Size{200, 22});
  f.root.captureMouse(raw);
  f.root.mouseMove(Point{70, 10}, 0);
  EXPECT_EQ(raw->moves, 1);
  f.root.releaseMouse(raw);
  f.root.mouseMove(Point{70, 10}, 0);
  EXPECT_EQ(raw->moves, 1);
}

TEST(Root, ThemeChangeRequestsRepaint) {
  Fixture f;
  int repaints = 0;
  f.root.setInvalidateHandler([&repaints] { ++repaints; });
  f.root.setTheme(ThemeName::Light);
  EXPECT_EQ(f.root.theme(), ThemeName::Light);
  EXPECT_GT(repaints, 0);
}

TEST(Root, HoverChangeRequestsRepaint) {
  Fixture f;
  int repaints = 0;
  f.root.setInvalidateHandler([&repaints] { ++repaints; });
  f.root.mouseMove(Point{10, 10}, 0);
  EXPECT_GT(repaints, 0);
}

TEST(Button, MeasureGrowsWithLabelAndIcon) {
  Fixture f;
  // 控件挂到 Root 后才能度量文字
  auto first = std::make_unique<Button>("确定");
  auto second = std::make_unique<Button>("确定确定确定");
  auto third = std::make_unique<Button>("确定");
  first->setAutoIcon(false);
  second->setAutoIcon(false);
  third->setAutoIcon(false);
  third->setIcon(rujian::ui::IconId::Check);
  Button* p1 = first.get();
  Button* p2 = second.get();
  Button* p3 = third.get();
  f.page->addChild(std::move(first));
  f.page->addChild(std::move(second));
  f.page->addChild(std::move(third));
  EXPECT_DOUBLE_EQ(p1->measure({}).height, 22);
  EXPECT_GT(p2->measure({}).width, p1->measure({}).width);
  EXPECT_GT(p3->measure({}).width, p1->measure({}).width + 14);
  EXPECT_GE(p1->measure({}).width, 16);
}
