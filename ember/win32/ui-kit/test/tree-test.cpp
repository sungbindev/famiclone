/*
 * ------------------------------------------------------------------------
 * 名称：tree-test.cpp
 * 说明：树的测试：每行 22 高、点击选中并回调（只在同一行按下松开时）、键盘只移动焦点、回车与空格选中、焦点移动时外层滚动区域跟着滚。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：树放在 (0,0)，宽 200；第 i 行的纵向范围是 22i 到 22(i+1)。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "rujian/ui/root.hpp"
#include "rujian/ui/scroll-area.hpp"
#include "rujian/ui/tree.hpp"

using namespace rujian::ui;

namespace {

constexpr int kVkTab = 0x09;
constexpr int kVkReturn = 0x0D;
constexpr int kVkSpace = 0x20;
constexpr int kVkEnd = 0x23;
constexpr int kVkHome = 0x24;
constexpr int kVkUp = 0x26;
constexpr int kVkDown = 0x28;

std::vector<TreeItem> makeItems(int count) {
  std::vector<TreeItem> items;
  for (int i = 0; i < count; ++i) items.push_back({"id" + std::to_string(i), "条目" + std::to_string(i)});
  return items;
}

struct Scene {
  Root root{ThemeName::Dark, 1.0};
  Tree* tree{nullptr};
  std::vector<std::string> selected;

  explicit Scene(int count = 5) {
    auto created = std::make_unique<Tree>();
    tree = created.get();
    tree->setItems(makeItems(count));
    tree->setOnSelect([this](const std::string& id) { selected.push_back(id); });
    root.setContent(std::move(created));
    root.layout(Size{200, 400});
  }

  void click(double y) {
    const Point point{50, y};
    root.mouseMove(point, 0);
    root.mouseDown(point, MouseButton::Left, 0);
    root.mouseUp(point, MouseButton::Left, 0);
  }
};

}  // namespace

TEST(Tree, EachRowIsTwentyTwoHigh) {
  Scene s;
  EXPECT_DOUBLE_EQ(s.tree->measure(Size{200, 0}).height, 5 * 22);
}

TEST(Tree, ClickingARowSelectsItAndReports) {
  Scene s;
  s.click(22 * 2 + 5);
  EXPECT_EQ(s.tree->selectedId(), "id2");
  EXPECT_EQ(s.selected, (std::vector<std::string>{"id2"}));
}

TEST(Tree, PressingOnOneRowAndReleasingOnAnotherDoesNotSelect) {
  Scene s;
  s.root.mouseMove(Point{50, 5}, 0);
  s.root.mouseDown(Point{50, 5}, MouseButton::Left, 0);
  s.root.mouseMove(Point{50, 60}, 0);
  s.root.mouseUp(Point{50, 60}, MouseButton::Left, 0);
  EXPECT_TRUE(s.selected.empty());
}

TEST(Tree, ArrowKeysMoveTheFocusRowWithoutSelecting) {
  Scene s;
  s.tree->setSelectedId("id1");
  s.root.keyDown(kVkTab, 0, false);
  EXPECT_TRUE(s.tree->focusVisible());
  EXPECT_DOUBLE_EQ(s.tree->focusRect().y, 22);
  s.root.keyDown(kVkDown, 0, false);
  EXPECT_DOUBLE_EQ(s.tree->focusRect().y, 44);
  s.root.keyDown(kVkEnd, 0, false);
  EXPECT_DOUBLE_EQ(s.tree->focusRect().y, 88);
  s.root.keyDown(kVkDown, 0, false);
  EXPECT_DOUBLE_EQ(s.tree->focusRect().y, 88);
  s.root.keyDown(kVkHome, 0, false);
  s.root.keyDown(kVkUp, 0, false);
  EXPECT_DOUBLE_EQ(s.tree->focusRect().y, 0);
  EXPECT_EQ(s.tree->selectedId(), "id1");
  EXPECT_TRUE(s.selected.empty());
}

TEST(Tree, EnterAndSpaceSelectTheFocusRow) {
  Scene s;
  s.root.keyDown(kVkTab, 0, false);
  s.root.keyDown(kVkDown, 0, false);
  s.root.keyDown(kVkReturn, 0, false);
  EXPECT_EQ(s.tree->selectedId(), "id1");
  s.root.keyDown(kVkDown, 0, false);
  s.root.keyDown(kVkSpace, 0, false);
  EXPECT_EQ(s.selected, (std::vector<std::string>{"id1", "id2"}));
}

TEST(Tree, MovingTheFocusScrollsTheEnclosingScrollArea) {
  Root root{ThemeName::Dark, 1.0};
  auto area = std::make_unique<ScrollArea>();
  ScrollArea* rawArea = area.get();
  auto tree = std::make_unique<Tree>();
  tree->setItems(makeItems(20));
  Tree* rawTree = tree.get();
  area->setContent(std::move(tree));
  area->setMaxHeight(110);
  root.setContent(std::move(area));
  root.setAnimationsEnabled(false);
  root.layout(Size{200, 110});
  root.keyDown(kVkTab, 0, false);
  for (int i = 0; i < 8; ++i) root.keyDown(kVkDown, 0, false);
  // 第 8 行（0 起）底边在 198，区域高 110，所以要滚动
  EXPECT_GT(rawArea->scrollTop(), 0);
  EXPECT_LE(rawTree->focusRect().bottom(), rawArea->bounds().bottom());
}
