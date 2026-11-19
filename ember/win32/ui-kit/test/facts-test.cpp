/*
 * ------------------------------------------------------------------------
 * 名称：facts-test.cpp
 * 说明：键值清单的测试：宽窗口两列（名称列宽 88、列间距 10、行间距 4）、窄窗口上下排列、内容可以是控件。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：窗口宽度大于 420 为宽窗口；子控件顺序是“名称、内容”成对排列。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <memory>

#include "rujian/ui/facts.hpp"
#include "rujian/ui/root.hpp"

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
  Facts* facts{nullptr};

  explicit Scene(double viewportWidth) {
    auto created = std::make_unique<Facts>();
    facts = created.get();
    root.setContent(std::move(created));
    facts->addFact("构建时间", "2026-11-19");
    facts->addFact("作者", std::make_unique<Block>(40));
    root.layout(Size{viewportWidth, 400});
    facts->arrange(Rect{0, 0, viewportWidth - 20, facts->measure(Size{viewportWidth - 20, 0}).height});
  }

  [[nodiscard]] const Rect& at(std::size_t index) const { return facts->children()[index]->bounds(); }
};

}  // namespace

TEST(Facts, WideWindowHasTwoColumns) {
  Scene s(500);
  EXPECT_DOUBLE_EQ(s.at(0).x, 0);
  EXPECT_DOUBLE_EQ(s.at(0).width, 88);
  EXPECT_DOUBLE_EQ(s.at(1).x, 88 + 10);
  EXPECT_DOUBLE_EQ(s.at(1).width, 480 - 88 - 10);
  EXPECT_DOUBLE_EQ(s.at(1).y, 0);
}

TEST(Facts, RowsAreSeparatedByTheRowGapAndTakeTheTallerCell) {
  Scene s(500);
  const double first = std::max(s.at(0).height, s.at(1).height);
  EXPECT_DOUBLE_EQ(s.at(2).y, first + 4);
  EXPECT_DOUBLE_EQ(s.at(3).height, 40);
  EXPECT_DOUBLE_EQ(s.facts->measure(Size{480, 0}).height, first + 4 + 40);
}

TEST(Facts, NarrowWindowStacksTheLabelAboveTheValue) {
  Scene s(400);
  EXPECT_DOUBLE_EQ(s.at(1).x, 0);
  EXPECT_DOUBLE_EQ(s.at(1).width, 380);
  EXPECT_DOUBLE_EQ(s.at(1).y, s.at(0).height);
  // 每条内容之后留 6
  EXPECT_DOUBLE_EQ(s.at(2).y, s.at(0).height + s.at(1).height + 6);
  EXPECT_DOUBLE_EQ(s.facts->measure(Size{380, 0}).height, s.at(2).y + s.at(2).height + 40 + 6);
}
