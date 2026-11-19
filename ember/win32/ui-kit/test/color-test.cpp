/*
 * ------------------------------------------------------------------------
 * 名称：color-test.cpp
 * 说明：颜色运算的测试。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：覆盖合成（不透明、半透明、全透明）、透明度调整与边界取值。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include "rujian/ui/color.hpp"

using rujian::ui::Color;
using rujian::ui::compose;

TEST(Color, ComponentsRoundTrip) {
  const Color color = Color::rgba(0x12, 0x34, 0x56, 0x78);
  EXPECT_EQ(color.r(), 0x12);
  EXPECT_EQ(color.g(), 0x34);
  EXPECT_EQ(color.b(), 0x56);
  EXPECT_EQ(color.a(), 0x78);
  EXPECT_EQ(color.argb, 0x78123456u);
}

TEST(Color, OpaqueForegroundReplacesBackground) {
  const Color result = compose(Color::rgba(10, 20, 30), Color::rgba(200, 200, 200));
  EXPECT_EQ(result, Color::rgba(10, 20, 30));
}

TEST(Color, TransparentForegroundKeepsBackground) {
  const Color background = Color::rgba(200, 100, 50);
  EXPECT_EQ(compose(rujian::ui::kTransparent, background), background);
}

TEST(Color, HalfAlphaOverOpaqueMixesChannels) {
  const Color result = compose(Color::rgba(255, 0, 0, 128), Color::rgba(0, 0, 255));
  EXPECT_EQ(result.a(), 255);
  EXPECT_NEAR(result.r(), 128, 1);
  EXPECT_NEAR(result.b(), 127, 1);
}

TEST(Color, BothTransparentGivesTransparent) { EXPECT_EQ(compose(rujian::ui::kTransparent, rujian::ui::kTransparent), rujian::ui::kTransparent); }

TEST(Color, WithOpacityScalesAlphaAndClampsFactor) {
  const Color color = Color::rgba(1, 2, 3, 200);
  EXPECT_EQ(color.withOpacity(0.5).a(), 100);
  EXPECT_EQ(color.withOpacity(2.0).a(), 200);
  EXPECT_EQ(color.withOpacity(-1.0).a(), 0);
  EXPECT_EQ(color.withOpacity(0.5).r(), 1);
}

TEST(Color, CurrentColorMarkerIsNotAnOrdinaryColor) {
  EXPECT_NE(rujian::ui::kCurrentColor, rujian::ui::kTransparent);
  EXPECT_NE(rujian::ui::kCurrentColor, Color::rgba(0, 0, 0, 0));
}
