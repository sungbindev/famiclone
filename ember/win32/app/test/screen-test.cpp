/*
 * ------------------------------------------------------------------------
 * 名称：screen-test.cpp
 * 说明：画面规则的测试：版面计算、指针位置换算与扫描码对照，位置的原点在左上角。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <cmath>

#include "shared/input/scan-code-table.hpp"
#include "shared/screen/pointer-mapping.hpp"
#include "shared/screen/screen-layout.hpp"

namespace ember::win32::shared {
namespace {

TEST(ScreenLayout, DisplayAspectRatioFollowsTheAspect) {
  EXPECT_NEAR(displayAspectRatio(ScreenAspect::Ratio4x3, 256, 240), 4.0 / 3.0, 1e-10);
  EXPECT_NEAR(displayAspectRatio(ScreenAspect::Ratio8x7, 256, 240), 256.0 * 8 / 7 / 240, 1e-10);
  EXPECT_NEAR(displayAspectRatio(ScreenAspect::Ratio8x7, 256, 224), 256.0 * 8 / 7 / 224, 1e-10);
}

TEST(ScreenLayout, NominalRatioOfTheDefaultAspectIs128To105) {
  EXPECT_EQ(kDefaultScreenAspect, ScreenAspect::Ratio8x7);
  EXPECT_NEAR(nominalScreenRatio(kDefaultScreenAspect), 128.0 / 105.0, 1e-10);
  EXPECT_NEAR(nominalScreenRatio(ScreenAspect::Ratio4x3), 4.0 / 3.0, 1e-10);
  EXPECT_NEAR(nominalScreenRatio(ScreenAspect::Ratio8x7, 8), 256.0 * 8 / 7 / 224, 1e-10);
  EXPECT_NEAR(nominalScreenRatio(ScreenAspect::Ratio4x3, 8), 4.0 / 3.0, 1e-10);
}

TEST(ScreenLayout, CropLinesAreZeroWhenShowingAllLinesAndEightOtherwise) {
  EXPECT_EQ(cropLinesFor(true), 0);
  EXPECT_EQ(cropLinesFor(false), 8);
}

TEST(ScreenLayout, FitCentersTheScreenAndLeavesBlackBarsOnTheLongSide) {
  EXPECT_EQ(computeScreenRect(1600, 600, 256, 240, ScreenAspect::Ratio4x3, ScreenScaling::Fit), (ScreenRect{400, 0, 800, 600}));
  EXPECT_EQ(computeScreenRect(800, 900, 256, 240, ScreenAspect::Ratio4x3, ScreenScaling::Fit), (ScreenRect{0, 150, 800, 600}));
  const ScreenRect wide = computeScreenRect(2000, 480, 256, 240, ScreenAspect::Ratio8x7, ScreenScaling::Fit);
  EXPECT_EQ(wide.height, 480);
  EXPECT_EQ(wide.width, static_cast<int>(std::floor(480.0 * 256 * 8 / 7 / 240 + 0.5)));
  EXPECT_EQ(wide.x, (2000 - wide.width) / 2);
}

TEST(ScreenLayout, SmallerCanvasShrinksWithoutLeavingTheCanvas) {
  // 网页版的左下角原点给出 y=12，换成左上角原点是 100-(12+75)=13
  EXPECT_EQ(computeScreenRect(100, 100, 256, 240, ScreenAspect::Ratio4x3, ScreenScaling::Fit), (ScreenRect{0, 13, 100, 75}));
}

TEST(ScreenLayout, IntegerScalingTakesTheLargestWholeMultipleOfTheHeight) {
  EXPECT_EQ(computeScreenRect(1920, 1080, 256, 240, ScreenAspect::Ratio4x3, ScreenScaling::Integer), (ScreenRect{320, 60, 1280, 960}));
  const ScreenRect narrow = computeScreenRect(700, 2000, 256, 240, ScreenAspect::Ratio4x3, ScreenScaling::Integer);
  EXPECT_EQ(narrow.height, 480);
  EXPECT_EQ(narrow.width, 640);
  EXPECT_EQ(computeScreenRect(1280, 960, 256, 240, ScreenAspect::Ratio4x3, ScreenScaling::Integer), (ScreenRect{0, 0, 1280, 960}));
}

TEST(ScreenLayout, IntegerScalingFallsBackToFitWhenOneTimesDoesNotFit) {
  EXPECT_EQ(computeScreenRect(200, 150, 256, 240, ScreenAspect::Ratio4x3, ScreenScaling::Integer), computeScreenRect(200, 150, 256, 240, ScreenAspect::Ratio4x3, ScreenScaling::Fit));
}

TEST(ScreenLayout, InvalidSizesGiveAnEmptyRect) {
  EXPECT_EQ(computeScreenRect(0, 100, 256, 240, ScreenAspect::Ratio4x3, ScreenScaling::Fit), ScreenRect{});
  EXPECT_EQ(computeScreenRect(100, 0, 256, 240, ScreenAspect::Ratio4x3, ScreenScaling::Fit), ScreenRect{});
  EXPECT_EQ(computeScreenRect(100, 100, 0, 240, ScreenAspect::Ratio4x3, ScreenScaling::Integer), ScreenRect{});
  EXPECT_EQ(computeScreenRect(-1, 100, 256, 240, ScreenAspect::Ratio4x3, ScreenScaling::Fit), ScreenRect{});
  EXPECT_EQ(computeScreenRect(std::nan(""), 100, 256, 240, ScreenAspect::Ratio4x3, ScreenScaling::Fit), ScreenRect{});
}

TEST(PointerMapping, CenterOfTheCanvasIsTheCenterOfTheFrame) {
  PointerView view;
  view.canvasWidth = 1280;
  view.canvasHeight = 960;
  view.aspect = ScreenAspect::Ratio4x3;
  const auto point = toFramePoint(640, 480, view);
  ASSERT_TRUE(point.has_value());
  EXPECT_NEAR(point->x, 128, 0.01);
  EXPECT_NEAR(point->y, 120, 0.01);
}

TEST(PointerMapping, OutsideTheScreenGivesCoordinatesBeyondTheFrame) {
  PointerView view;
  view.canvasWidth = 1600;
  view.canvasHeight = 600;
  view.aspect = ScreenAspect::Ratio4x3;
  // 画面在 x=400..1200：画布左边缘在画面左边 400 像素外，换成帧坐标是 -128
  const auto left = toFramePoint(0, 0, view);
  ASSERT_TRUE(left.has_value());
  EXPECT_NEAR(left->x, -128, 0.01);
  EXPECT_NEAR(left->y, 0, 0.01);
}

TEST(PointerMapping, CroppedLinesAreAddedToTheRowNumber) {
  PointerView view;
  view.canvasWidth = 1280;
  view.canvasHeight = 960;
  view.aspect = ScreenAspect::Ratio4x3;
  view.cropLines = 8;
  const auto top = toFramePoint(0, 0, view);
  ASSERT_TRUE(top.has_value());
  EXPECT_NEAR(top->y, 8, 0.01);
  const auto bottom = toFramePoint(0, 960, view);
  ASSERT_TRUE(bottom.has_value());
  EXPECT_NEAR(bottom->y, 232, 0.01);
}

TEST(PointerMapping, NoCanvasGivesNothing) {
  PointerView view;
  EXPECT_FALSE(toFramePoint(10, 10, view).has_value());
  EXPECT_FALSE(frameUnitsPerPixel(view).has_value());
}

TEST(PointerMapping, UnitsPerPixelFollowTheScreenRect) {
  PointerView view;
  view.canvasWidth = 1280;
  view.canvasHeight = 960;
  view.aspect = ScreenAspect::Ratio4x3;
  const auto scale = frameUnitsPerPixel(view);
  ASSERT_TRUE(scale.has_value());
  EXPECT_NEAR(scale->x, 256.0 / 1280.0, 1e-10);
  EXPECT_NEAR(scale->y, 240.0 / 960.0, 1e-10);
}

TEST(ScanCodeTable, MapsPlainExtendedAndRejectsTheRest) {
  EXPECT_EQ(scanCodeToKeyCode(0x01, false, false), "Escape");
  EXPECT_EQ(scanCodeToKeyCode(0x1e, false, false), "KeyA");
  EXPECT_EQ(scanCodeToKeyCode(0x39, false, false), "Space");
  EXPECT_EQ(scanCodeToKeyCode(0x53, false, false), "NumpadDecimal");
  EXPECT_EQ(scanCodeToKeyCode(0x43, false, false), "F9");
  EXPECT_EQ(scanCodeToKeyCode(0x57, false, false), "F11");
  EXPECT_EQ(scanCodeToKeyCode(0x56, false, false), "IntlBackslash");
  EXPECT_EQ(scanCodeToKeyCode(0x1c, true, false), "NumpadEnter");
  EXPECT_EQ(scanCodeToKeyCode(0x48, true, false), "ArrowUp");
  EXPECT_EQ(scanCodeToKeyCode(0x5b, true, false), "MetaLeft");
  // 假 Shift、E1 前缀的 Pause、没有收录的扫描码
  EXPECT_FALSE(scanCodeToKeyCode(0x2a, true, false).has_value());
  EXPECT_FALSE(scanCodeToKeyCode(0x1d, false, true).has_value());
  EXPECT_FALSE(scanCodeToKeyCode(0x54, false, false).has_value());
  EXPECT_FALSE(scanCodeToKeyCode(0x00, false, false).has_value());
  EXPECT_FALSE(scanCodeToKeyCode(0x7f, false, false).has_value());
}

TEST(ScanCodeTable, EveryLetterAndDigitKeyHasItsOwnName) {
  EXPECT_EQ(scanCodeToKeyCode(0x10, false, false), "KeyQ");
  EXPECT_EQ(scanCodeToKeyCode(0x19, false, false), "KeyP");
  EXPECT_EQ(scanCodeToKeyCode(0x2c, false, false), "KeyZ");
  EXPECT_EQ(scanCodeToKeyCode(0x32, false, false), "KeyM");
  EXPECT_EQ(scanCodeToKeyCode(0x02, false, false), "Digit1");
  EXPECT_EQ(scanCodeToKeyCode(0x0b, false, false), "Digit0");
}

}  // namespace
}  // namespace ember::win32::shared
