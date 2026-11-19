/*
 * ------------------------------------------------------------------------
 * 名称：setting-row-test.cpp
 * 说明：设置行的测试：两列（名称列最宽 420、控件列最宽 280、列间距 10）、各列垂直居中、空间不够时两列平分、错误文字在控件下方、窄窗口上下排列。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：窗口宽度不超过 720 为窄窗口；子控件顺序是“名称、说明、错误、控件”。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <memory>

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

struct Scene {
  Root root{ThemeName::Dark, 1.0};
  SettingRow* row{nullptr};

  Scene(double viewportWidth, double rowWidth, const char* description, const char* error) {
    auto created = std::make_unique<SettingRow>();
    row = created.get();
    root.setContent(std::move(created));
    row->setLabel("名称");
    row->setDescription(description);
    row->setControl(std::make_unique<Block>(22));
    row->setError(error);
    root.layout(Size{viewportWidth, 400});
    row->arrange(Rect{0, 0, rowWidth, row->measure(Size{rowWidth, 0}).height});
  }

  [[nodiscard]] const Rect& label() const { return row->children()[0]->bounds(); }
  [[nodiscard]] const Rect& description() const { return row->children()[1]->bounds(); }
  [[nodiscard]] const Rect& error() const { return row->children()[2]->bounds(); }
  [[nodiscard]] const Rect& control() const { return row->children()[3]->bounds(); }
};

}  // namespace

TEST(SettingRow, WideRowHasTwoColumnsWithFixedWidths) {
  Scene s(1000, 800, "说明", "");
  EXPECT_DOUBLE_EQ(s.label().x, 0);
  EXPECT_DOUBLE_EQ(s.label().width, 420);
  EXPECT_DOUBLE_EQ(s.control().x, 420 + 10);
  EXPECT_DOUBLE_EQ(s.control().width, 280);
}

TEST(SettingRow, ColumnsAreCenteredAndPaddedVertically) {
  Scene s(1000, 800, "", "");
  // 没有说明时名称列只有名称；控件 22 高，名称约 14 高，行高 = 22 + 上下内边距 16
  EXPECT_DOUBLE_EQ(s.row->measure(Size{800, 0}).height, 22 + 16);
  EXPECT_DOUBLE_EQ(s.control().y, 8);
  EXPECT_DOUBLE_EQ(s.label().y, 8 + (22 - s.label().height) / 2.0);
}

TEST(SettingRow, DescriptionSitsBelowTheLabelWithATwoPixelGap) {
  Scene s(1000, 800, "说明", "");
  EXPECT_DOUBLE_EQ(s.description().y, s.label().y + s.label().height + 2);
}

TEST(SettingRow, ColumnsShareTheSpaceWhenItIsNotEnough) {
  Scene s(1000, 400, "", "");
  // 可分的宽度 390：控件列取 min(280, 195)，名称列取余下的
  EXPECT_DOUBLE_EQ(s.control().width, 195);
  EXPECT_DOUBLE_EQ(s.label().width, 195);
  Scene medium(1000, 600, "", "");
  EXPECT_DOUBLE_EQ(medium.control().width, 280);
  EXPECT_DOUBLE_EQ(medium.label().width, 310);
}

TEST(SettingRow, ErrorIsBelowTheControl) {
  Scene s(1000, 800, "", "不合法");
  EXPECT_DOUBLE_EQ(s.error().y, s.control().y + 22 + 4);
  EXPECT_DOUBLE_EQ(s.error().width, 280);
}

TEST(SettingRow, NarrowWindowStacksTheTextAboveTheControl) {
  Scene s(600, 500, "说明", "");
  EXPECT_DOUBLE_EQ(s.control().x, 0);
  EXPECT_DOUBLE_EQ(s.control().width, 500);
  EXPECT_DOUBLE_EQ(s.label().y, 8);
  EXPECT_DOUBLE_EQ(s.control().y, 8 + s.label().height + 2 + s.description().height + 8);
}
