/*
 * ------------------------------------------------------------------------
 * 名称：screen-overlay-test.cpp
 * 说明：画面右下角提示框的排版测试：每条提示一个框、上下排列、右边对齐、不重叠，没有提示时没有框。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-20
 * 备注：用控件库的离屏根容器排版。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "rujian/ui/root.hpp"
#include "ui/runtime/screen-overlay-view.hpp"

namespace ember::win32 {
namespace {

std::vector<rujian::ui::Rect> boxesOf(bool mouse, bool keyboard) {
  rujian::ui::Root root(rujian::ui::ThemeName::Dark, 1.0);
  auto view = std::make_unique<ui::ScreenOverlayView>(ui::OverlayState::Hidden, mouse, keyboard);
  const ui::ScreenOverlayView* pointer = view.get();
  root.setContent(std::move(view));
  root.layout(rujian::ui::Size{600, 400});
  return pointer->glassRects();
}

TEST(ScreenOverlayView, EachHintHasItsOwnBoxStackedAtTheBottomRight) {
  const auto boxes = boxesOf(true, true);
  ASSERT_EQ(boxes.size(), 2U);
  // 键盘独占的提示在上，鼠标捕获的在下；右边对齐，上下不重叠
  EXPECT_LT(boxes[0].y, boxes[1].y);
  EXPECT_LE(boxes[0].bottom(), boxes[1].y);
  EXPECT_DOUBLE_EQ(boxes[0].right(), boxes[1].right());
  EXPECT_LE(boxes[1].bottom(), 400.0);
  EXPECT_LE(boxes[0].right(), 600.0);
  EXPECT_GT(boxes[0].x, 0.0);
}

TEST(ScreenOverlayView, OnlyTheActiveHintsHaveBoxes) {
  EXPECT_TRUE(boxesOf(false, false).empty());
  EXPECT_EQ(boxesOf(true, false).size(), 1U);
  EXPECT_EQ(boxesOf(false, true).size(), 1U);
}

}  // namespace
}  // namespace ember::win32
