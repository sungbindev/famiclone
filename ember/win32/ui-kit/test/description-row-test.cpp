/*
 * ------------------------------------------------------------------------
 * 名称：description-row-test.cpp
 * 说明：说明行的测试：说明文字在左、操作在右并垂直居中，宽度不够时说明文字换行、行高随之变化，没有操作时说明占满整行。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-20
 * 备注：这个控件不在网页版组件库里，没有像素基线。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <memory>

#include "rujian/ui/button.hpp"
#include "rujian/ui/description-row.hpp"
#include "rujian/ui/root.hpp"

using namespace rujian::ui;

namespace {

struct Scene {
  Root root{ThemeName::Dark, 1.0};
  DescriptionRow* row{nullptr};
  Button* action{nullptr};

  explicit Scene(double width, bool withAction = true) {
    auto created = std::make_unique<DescriptionRow>();
    row = created.get();
    created->setText("点手柄上的键，再按下键盘上的键来设置。");
    if (withAction) {
      auto button = std::make_unique<Button>("清除");
      button->setCompact(true);
      action = button.get();
      created->setAction(std::move(button));
    }
    root.setContent(std::move(created));
    root.layout(Size{width, 100});
  }
};

}  // namespace

TEST(DescriptionRow, ActionSitsAtTheRightEdgeAndBothAreVerticallyCentered) {
  Scene s(500);
  const Rect action = s.action->bounds();
  EXPECT_DOUBLE_EQ(action.right(), 500.0);
  const Size natural = s.row->measure(Size{500, 0});
  EXPECT_NEAR(action.y + action.height / 2, natural.height / 2, 0.51);
  EXPECT_DOUBLE_EQ(natural.width, 500.0);
}

TEST(DescriptionRow, TextWrapsWhenThereIsNotEnoughWidthAndTheRowGetsTaller) {
  Scene wide(500);
  Scene narrow(120);
  EXPECT_GT(narrow.row->measure(Size{120, 0}).height, wide.row->measure(Size{500, 0}).height);
  // 窄时操作仍在最右，文字不压到它
  EXPECT_DOUBLE_EQ(narrow.action->bounds().right(), 120.0);
}

TEST(DescriptionRow, WithoutAnActionTheTextTakesTheWholeRow) {
  Scene s(300, false);
  EXPECT_EQ(s.row->text(), "点手柄上的键，再按下键盘上的键来设置。");
  EXPECT_GT(s.row->measure(Size{300, 0}).height, 0.0);
}
