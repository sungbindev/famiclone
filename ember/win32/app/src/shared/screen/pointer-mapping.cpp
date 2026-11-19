/*
 * ------------------------------------------------------------------------
 * 名称：pointer-mapping.cpp
 * 说明：指针位置换算的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "shared/screen/pointer-mapping.hpp"

#include <cmath>

namespace ember::win32::shared {

namespace {

/** 坐标保留的小数位：两位。 */
constexpr double kCoordinateScale = 100;

/** 当前显示方式下画面的矩形；画布或画面的尺寸无效时宽高为 0。 */
ScreenRect screenRectOf(const PointerView& view) {
  const int visibleHeight = kNominalFrameHeight - view.cropLines * 2;
  return computeScreenRect(view.canvasWidth, view.canvasHeight, kNominalFrameWidth, visibleHeight, view.aspect, view.scaling);
}

}  // namespace

double roundCoordinate(double value) { return std::floor(value * kCoordinateScale + 0.5) / kCoordinateScale; }

std::optional<FramePoint> toFramePoint(double x, double y, const PointerView& view) {
  const ScreenRect rect = screenRectOf(view);
  if (rect.width == 0 || rect.height == 0) return std::nullopt;
  const int visibleHeight = kNominalFrameHeight - view.cropLines * 2;
  // 裁掉的行在上面，行号要加上它
  return FramePoint{roundCoordinate(((x - rect.x) / rect.width) * kNominalFrameWidth), roundCoordinate(view.cropLines + ((y - rect.y) / rect.height) * visibleHeight)};
}

std::optional<FrameUnitsPerPixel> frameUnitsPerPixel(const PointerView& view) {
  const ScreenRect rect = screenRectOf(view);
  if (rect.width == 0 || rect.height == 0) return std::nullopt;
  const int visibleHeight = kNominalFrameHeight - view.cropLines * 2;
  return FrameUnitsPerPixel{static_cast<double>(kNominalFrameWidth) / rect.width, static_cast<double>(visibleHeight) / rect.height};
}

}  // namespace ember::win32::shared
