/*
 * ------------------------------------------------------------------------
 * 名称：pointer-mapping.hpp
 * 说明：把指针在画面区里的位置换算成标准帧（256x240）的坐标，以及捕获鼠标时每个设备像素对应标准帧的多少个单位。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：坐标按画面在画布里的实际矩形换算（见 screen-layout.hpp），
 *       所以不受画面比例、缩放方式与窗口大小的影响；指针在画面之外时坐标可以超出 0 到 256、0 到 240。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <optional>

#include "shared/screen/screen-layout.hpp"

namespace ember::win32::shared {

/** 画面当前的显示方式：画布的设备像素尺寸、显示选项与上下各裁掉的行数。 */
struct PointerView {
  double canvasWidth{0};
  double canvasHeight{0};
  ScreenAspect aspect{kDefaultScreenAspect};
  ScreenScaling scaling{kDefaultScreenScaling};
  /** 帧上下各裁掉的行数；显示的是中间的行，坐标仍然是整帧上的行号。 */
  int cropLines{0};
};

/** 标准帧上的一个点。 */
struct FramePoint {
  double x{0};
  double y{0};

  [[nodiscard]] bool operator==(const FramePoint& other) const = default;
};

/** 每个设备像素对应标准帧的多少个单位。 */
struct FrameUnitsPerPixel {
  double x{0};
  double y{0};
};

/** 坐标保留两位小数：两位已经比一个像素的百分之一更细。 */
[[nodiscard]] double roundCoordinate(double value);

/**
 * 把指针位置换算成标准帧的坐标。
 * @param x 指针相对画面区左边的横坐标（设备像素）。
 * @param y 指针相对画面区上边的纵坐标（设备像素）。
 * @param view 画布尺寸与显示选项。
 * @returns 标准帧坐标；画布或画面的尺寸无效（还没有画布）时没有值。
 */
[[nodiscard]] std::optional<FramePoint> toFramePoint(double x, double y, const PointerView& view);

/**
 * 捕获鼠标时把相对位移（设备像素）换算成标准帧的位移用的比值。
 * @returns 横向与纵向的比值；画布或画面的尺寸无效时没有值。
 */
[[nodiscard]] std::optional<FrameUnitsPerPixel> frameUnitsPerPixel(const PointerView& view);

}  // namespace ember::win32::shared
