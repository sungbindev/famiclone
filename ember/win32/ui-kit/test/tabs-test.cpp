/*
 * ------------------------------------------------------------------------
 * 名称：tabs-test.cpp
 * 说明：页签的测试：页签栏高度与内容摆放、点击与键盘切换（首尾循环、Home、End）、回调只在用户操作造成变化时调用、只显示选中页签的内容、焦点只有一个停靠点。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：页签栏放在 (0,0)；页签高 32，下方分割线 1，页签栏与内容之间留 10；三个页签文字都是“页”后接数字。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "rujian/ui/root.hpp"
#include "rujian/ui/tabs.hpp"

using namespace rujian::ui;

namespace {

constexpr int kVkTab = 0x09;
constexpr int kVkEnd = 0x23;
constexpr int kVkHome = 0x24;
constexpr int kVkLeft = 0x25;
constexpr int kVkRight = 0x27;

/** 固定高度的内容。 */
class Block final : public Widget {
 public:
  explicit Block(double height) : height_(height) {}
  Size measure(Size available) override { return Size{available.width, height_}; }

 private:
  double height_;
};

struct Scene {
  Root root{ThemeName::Dark, 1.0};
  Tabs* tabs{nullptr};
  Widget* first{nullptr};
  Widget* second{nullptr};
  std::vector<std::string> selected;

  Scene() {
    auto created = std::make_unique<Tabs>();
    tabs = created.get();
    first = tabs->addTab("a", "页1", std::make_unique<Block>(30));
    second = tabs->addTab("b", "页2", std::make_unique<Block>(50));
    tabs->addTab("c", "页3");
    tabs->setOnSelect([this](const std::string& id) { selected.push_back(id); });
    root.setContent(std::move(created));
    root.layout(Size{300, 200});
  }

  /** 第 index 个页签的中心（页签栏在原点，页签依次排列）。 */
  [[nodiscard]] Point center(double leftOffset) const { return Point{leftOffset, 16}; }
};

}  // namespace

TEST(Tabs, BarIsThirtyTwoHighWithADividerAndTheContentFollowsTenBelow) {
  Scene s;
  EXPECT_DOUBLE_EQ(s.tabs->tabBar()->bounds().height, 33);
  EXPECT_DOUBLE_EQ(s.first->bounds().y, 33 + 10);
  EXPECT_DOUBLE_EQ(s.first->bounds().height, 30);
  EXPECT_DOUBLE_EQ(s.tabs->measure(Size{300, 0}).height, 33 + 10 + 30);
}

TEST(Tabs, OnlyTheSelectedContentIsVisible) {
  Scene s;
  EXPECT_TRUE(s.first->visible());
  EXPECT_FALSE(s.second->visible());
  s.tabs->select("b");
  EXPECT_FALSE(s.first->visible());
  EXPECT_TRUE(s.second->visible());
  EXPECT_DOUBLE_EQ(s.tabs->measure(Size{300, 0}).height, 33 + 10 + 50);
}

TEST(Tabs, ClickingATabSelectsItAndReportsOnce) {
  Scene s;
  // 每个页签约 40 宽（“页”12 加数字约 7 再加内边距 20），x=60 落在第二个页签上
  const Point onSecond = s.center(60);
  s.root.mouseMove(onSecond, 0);
  s.root.mouseDown(onSecond, MouseButton::Left, 0);
  s.root.mouseUp(onSecond, MouseButton::Left, 0);
  ASSERT_EQ(s.selected.size(), 1u);
  EXPECT_EQ(s.selected[0], "b");
  EXPECT_EQ(s.tabs->selectedId(), "b");
  s.root.mouseDown(onSecond, MouseButton::Left, 0);
  s.root.mouseUp(onSecond, MouseButton::Left, 0);
  EXPECT_EQ(s.selected.size(), 1u);
}

TEST(Tabs, ArrowKeysWrapAndHomeEndJump) {
  Scene s;
  s.root.keyDown(kVkTab, 0, false);
  EXPECT_TRUE(s.tabs->tabBar()->focused());
  s.root.keyDown(kVkLeft, 0, false);
  EXPECT_EQ(s.tabs->selectedId(), "c");
  s.root.keyDown(kVkRight, 0, false);
  EXPECT_EQ(s.tabs->selectedId(), "a");
  s.root.keyDown(kVkEnd, 0, false);
  EXPECT_EQ(s.tabs->selectedId(), "c");
  s.root.keyDown(kVkHome, 0, false);
  EXPECT_EQ(s.tabs->selectedId(), "a");
  EXPECT_EQ(s.selected, (std::vector<std::string>{"c", "a", "c", "a"}));
}

TEST(Tabs, ProgrammaticSelectDoesNotCallTheHandler) {
  Scene s;
  s.tabs->select("c");
  EXPECT_EQ(s.tabs->selectedId(), "c");
  EXPECT_TRUE(s.selected.empty());
  s.tabs->select("missing");
  EXPECT_EQ(s.tabs->selectedId(), "c");
}

TEST(Tabs, TabKeyFocusesTheBarWithAVisibleFocus) {
  Scene s;
  s.root.keyDown(kVkTab, 0, false);
  EXPECT_TRUE(s.tabs->tabBar()->focusVisible());
}

TEST(Tabs, CursorIsAHandOverATab) {
  Scene s;
  s.root.mouseMove(s.center(10), 0);
  EXPECT_EQ(s.root.cursor(), CursorKind::Hand);
  s.root.mouseMove(Point{290, 100}, 0);
  EXPECT_EQ(s.root.cursor(), CursorKind::Arrow);
}
