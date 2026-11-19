/*
 * ------------------------------------------------------------------------
 * 名称：text-test.cpp
 * 说明：文字分段与排版度量的测试。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：排版度量依赖本机字体，所以只断言相对关系（宽窄、行数、行高），不断言具体像素。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include "base/font-roles.hpp"
#include "base/graphics.hpp"

using rujian::ui::FontRole;
using rujian::ui::fontRoleOf;
using rujian::ui::Graphics;
using rujian::ui::TextOverflow;
using rujian::ui::TextStyle;

TEST(FontRoles, ClassifiesByCharacter) {
  EXPECT_EQ(fontRoleOf(U'A'), FontRole::Latin);
  EXPECT_EQ(fontRoleOf(U'7'), FontRole::Latin);
  EXPECT_EQ(fontRoleOf(U'"'), FontRole::Latin);
  EXPECT_EQ(fontRoleOf(U'确'), FontRole::Cjk);
  EXPECT_EQ(fontRoleOf(U'，'), FontRole::Cjk);
  EXPECT_EQ(fontRoleOf(0x2014), FontRole::Punctuation);
  EXPECT_EQ(fontRoleOf(0x2018), FontRole::Punctuation);
  EXPECT_EQ(fontRoleOf(0x201D), FontRole::Punctuation);
  EXPECT_EQ(fontRoleOf(0x2026), FontRole::Punctuation);
}

TEST(FontRoles, NeighboursOfPunctuationRangeAreNotPunctuation) {
  EXPECT_EQ(fontRoleOf(0x2013), FontRole::Latin);
  EXPECT_EQ(fontRoleOf(0x201E), FontRole::Latin);
  EXPECT_EQ(fontRoleOf(0x2025), FontRole::Latin);
  EXPECT_EQ(fontRoleOf(0x2E7F), FontRole::Latin);
  EXPECT_EQ(fontRoleOf(0x2E80), FontRole::Cjk);
}

TEST(TextEngine, EmptyTextHasZeroWidth) {
  const auto metrics = Graphics::shared().text().measure("", TextStyle{}, 0);
  EXPECT_DOUBLE_EQ(metrics.width, 0);
}

TEST(TextEngine, SingleLineHeightIsFontSizeTimesLineHeight) {
  TextStyle style;
  style.fontSize = 12;
  style.lineHeight = 1.45;
  const auto metrics = Graphics::shared().text().measure("确定 OK", style, 0);
  EXPECT_NEAR(metrics.height, 12 * 1.45, 0.01);
}

TEST(TextEngine, LongerTextIsWiderAndLargerFontIsWider) {
  auto& text = Graphics::shared().text();
  TextStyle regular;
  regular.fontSize = 12;
  TextStyle large;
  large.fontSize = 24;
  EXPECT_GT(text.measure("确定确定", regular, 0).width, text.measure("确定", regular, 0).width);
  EXPECT_GT(text.measure("Hello", large, 0).width, text.measure("Hello", regular, 0).width);
}

TEST(TextEngine, MixedScriptTextIsLayoutAsOneRun) {
  TextStyle style;
  const auto both = Graphics::shared().text().measure("A确", style, 0);
  const auto first = Graphics::shared().text().measure("A", style, 0);
  const auto second = Graphics::shared().text().measure("确", style, 0);
  EXPECT_NEAR(both.width, first.width + second.width, 1.0);
}

TEST(TextEngine, WrapProducesMoreLinesWhenNarrow) {
  TextStyle style;
  style.wrap = true;
  auto& text = Graphics::shared().text();
  const auto wide = text.measure("one two three four five six", style, 1000);
  const auto narrow = text.measure("one two three four five six", style, 60);
  EXPECT_GT(narrow.height, wide.height);
  EXPECT_LE(narrow.width, 60.5);
}

TEST(TextEngine, EllipsisKeepsSingleLineAndFitsWidth) {
  TextStyle style;
  style.overflow = TextOverflow::Ellipsis;
  const auto metrics = Graphics::shared().text().measure("这是一段很长很长很长很长的文字", style, 60);
  EXPECT_NEAR(metrics.height, style.fontSize * style.lineHeight, 0.01);
  EXPECT_LE(metrics.width, 60.5);
}

TEST(TextEngine, PunctuationRangeStillMeasures) {
  TextStyle style;
  const auto metrics = Graphics::shared().text().measure("“好”—…", style, 0);
  EXPECT_GT(metrics.width, 0);
}
