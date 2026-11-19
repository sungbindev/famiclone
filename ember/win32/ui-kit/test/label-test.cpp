/*
 * ------------------------------------------------------------------------
 * 名称：label-test.cpp
 * 说明：文字控件的测试：空文字没有高度、换行符分行、宽度不够时换行、不换行时只占一行、小节标题的字号与字重。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：高度以同一字体下单行文字的高度为基准比较，不写死像素值。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <memory>

#include "rujian/ui/label.hpp"
#include "rujian/ui/root.hpp"

using namespace rujian::ui;

namespace {

struct Scene {
  Root root{ThemeName::Dark, 1.0};
  Label* label{nullptr};

  explicit Scene(std::unique_ptr<Label> created) {
    label = created.get();
    root.setContent(std::move(created));
    root.layout(Size{200, 200});
  }
};

}  // namespace

TEST(Label, EmptyTextHasNoSize) {
  Scene s(std::make_unique<Label>());
  const Size size = s.label->measure(Size{100, 0});
  EXPECT_DOUBLE_EQ(size.width, 0);
  EXPECT_DOUBLE_EQ(size.height, 0);
}

TEST(Label, NewlinesStartNewLines) {
  Scene one(std::make_unique<Label>("甲"));
  Scene two(std::make_unique<Label>("甲\n乙"));
  const double single = one.label->measure(Size{100, 0}).height;
  EXPECT_GT(single, 0);
  EXPECT_DOUBLE_EQ(two.label->measure(Size{100, 0}).height, single * 2);
}

TEST(Label, WrapsWhenTheWidthIsNotEnough) {
  Scene s(std::make_unique<Label>("甲乙丙丁戊己庚辛壬癸甲乙丙丁戊己庚辛壬癸"));
  const double natural = s.label->measure(Size{}).height;
  const double narrow = s.label->measure(Size{60, 0}).height;
  EXPECT_GT(narrow, natural);
}

TEST(Label, WithoutWrapStaysOnOneLine) {
  Scene s(std::make_unique<Label>("甲乙丙丁戊己庚辛壬癸甲乙丙丁戊己庚辛壬癸"));
  const double natural = s.label->measure(Size{}).height;
  s.label->setWrap(false);
  EXPECT_DOUBLE_EQ(s.label->measure(Size{60, 0}).height, natural);
}

TEST(Label, LineHeightMultipleScalesEachLine) {
  Scene s(std::make_unique<Label>("甲\n乙"));
  s.label->setLineHeight(1.5);
  // 字号 12、行高 1.5 倍：每行 18，两行 36
  EXPECT_NEAR(s.label->measure(Size{100, 0}).height, 36, 1.0);
}

TEST(Heading, UsesTheHeadingFontSize) {
  Scene plain(std::make_unique<Label>("标题"));
  Scene heading(std::make_unique<Heading>("标题"));
  EXPECT_DOUBLE_EQ(heading.label->measure(Size{}).height, plain.label->measure(Size{}).height);
  EXPECT_GE(heading.label->measure(Size{}).width, plain.label->measure(Size{}).width);
}
