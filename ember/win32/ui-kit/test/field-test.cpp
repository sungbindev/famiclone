/*
 * ------------------------------------------------------------------------
 * 名称：field-test.cpp
 * 说明：字段的测试：有标签时依次是标签、说明、控件（间距 2 与 6），没有说明时控件紧跟标签，没有标签时控件在前、说明在后（间距 4）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：子控件顺序固定为“标签、说明、控件”。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <memory>

#include "rujian/ui/field.hpp"
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
  Field* field{nullptr};

  Scene(const char* label, const char* description) {
    auto created = std::make_unique<Field>();
    field = created.get();
    root.setContent(std::move(created));
    field->setLabel(label);
    field->setDescription(description);
    field->setControl(std::make_unique<Block>(22));
    root.layout(Size{300, 300});
    field->arrange(Rect{0, 0, 300, field->measure(Size{300, 0}).height});
  }

  [[nodiscard]] const Rect& at(std::size_t index) const { return field->children()[index]->bounds(); }
};

}  // namespace

TEST(Field, LabelDescriptionControlAreStackedWithTheMargins) {
  Scene s("名称", "说明文字");
  EXPECT_DOUBLE_EQ(s.at(0).y, 0);
  EXPECT_DOUBLE_EQ(s.at(1).y, s.at(0).height + 2);
  EXPECT_DOUBLE_EQ(s.at(2).y, s.at(1).y + s.at(1).height + 6);
  EXPECT_DOUBLE_EQ(s.at(2).width, 300);
  EXPECT_DOUBLE_EQ(s.field->measure(Size{300, 0}).height, s.at(2).y + 22);
}

TEST(Field, WithoutDescriptionTheControlFollowsTheLabel) {
  Scene s("名称", "");
  EXPECT_DOUBLE_EQ(s.at(2).y, s.at(0).height + 2);
}

TEST(Field, WithoutLabelTheDescriptionComesAfterTheControl) {
  Scene s("", "说明文字");
  EXPECT_DOUBLE_EQ(s.at(2).y, 0);
  EXPECT_DOUBLE_EQ(s.at(1).y, 22 + 4);
  EXPECT_DOUBLE_EQ(s.field->measure(Size{300, 0}).height, 22 + 4 + s.at(1).height);
}
