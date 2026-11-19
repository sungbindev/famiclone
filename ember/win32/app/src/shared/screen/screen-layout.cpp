/*
 * ------------------------------------------------------------------------
 * 名称：screen-layout.cpp
 * 说明：画面版面计算的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：取整的方式与网页版一致：Math.round 对正数是四舍五入（0.5 向上）。
 * ------------------------------------------------------------------------
 */

#include "shared/screen/screen-layout.hpp"

#include <algorithm>
#include <cmath>

namespace ember::win32::shared {

namespace {

/** 像素宽高比 8:7 的宽与高。 */
constexpr double kPixelAspectWidth = 8;
constexpr double kPixelAspectHeight = 7;

/** 同 JavaScript 的 Math.round：四舍五入，0.5 向上。 */
double roundHalfUp(double value) { return std::floor(value + 0.5); }

/** 把宽高限制在画布内并居中；居中的偏移与网页版（原点在左下角）取整后的位置相同。 */
ScreenRect centered(double canvasWidth, double canvasHeight, double width, double height) {
  const double fittedWidth = std::max(0.0, std::min(canvasWidth, roundHalfUp(width)));
  const double fittedHeight = std::max(0.0, std::min(canvasHeight, roundHalfUp(height)));
  const double x = std::floor((canvasWidth - fittedWidth) / 2);
  const double bottom = std::floor((canvasHeight - fittedHeight) / 2);
  return ScreenRect{static_cast<int>(x), static_cast<int>(canvasHeight - (bottom + fittedHeight)), static_cast<int>(fittedWidth), static_cast<int>(fittedHeight)};
}

}  // namespace

int cropLinesFor(bool showAllLines) { return showAllLines ? 0 : kCroppedLines; }

double displayAspectRatio(ScreenAspect aspect, double frameWidth, double frameHeight) {
  if (aspect == ScreenAspect::Ratio4x3) return 4.0 / 3.0;
  return (frameWidth * kPixelAspectWidth) / kPixelAspectHeight / frameHeight;
}

double nominalScreenRatio(ScreenAspect aspect, int cropLines) { return displayAspectRatio(aspect, kNominalFrameWidth, kNominalFrameHeight - cropLines * 2); }

ScreenRect computeScreenRect(double canvasWidth, double canvasHeight, double frameWidth, double frameHeight, ScreenAspect aspect, ScreenScaling scaling) {
  if (!(canvasWidth > 0 && canvasHeight > 0 && frameWidth > 0 && frameHeight > 0)) return ScreenRect{};
  const double ratio = displayAspectRatio(aspect, frameWidth, frameHeight);
  if (scaling == ScreenScaling::Integer) {
    const double multiple = std::floor(std::min(canvasHeight / frameHeight, canvasWidth / (frameHeight * ratio)));
    if (multiple >= 1) return centered(canvasWidth, canvasHeight, frameHeight * multiple * ratio, frameHeight * multiple);
  }
  if (canvasWidth / canvasHeight >= ratio) return centered(canvasWidth, canvasHeight, canvasHeight * ratio, canvasHeight);
  return centered(canvasWidth, canvasHeight, canvasWidth, canvasWidth / ratio);
}

}  // namespace ember::win32::shared
