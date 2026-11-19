/*
 * ------------------------------------------------------------------------
 * 名称：geometry.hpp
 * 说明：几何类型：点、尺寸、矩形、四边内距，坐标与长度都是逻辑像素（与 CSS 像素相同），绘制时再乘以窗口的缩放比例。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：矩形用左上角加宽高表示；inset 与 outset 对应 CSS 的内边距与外扩。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <algorithm>
#include <cmath>

namespace rujian::ui {

/**
 * CSS 边框宽度的取整规则：取设备像素的整数倍（向下）且至少 1 个设备像素；宽度为 0 时还是 0。
 * 网页版的布局也用取整后的宽度（比如 1.25 倍下 1px 的边框占 0.8 个逻辑像素），有边框的控件要用它算尺寸。
 * @param width CSS 里的宽度（逻辑像素）。
 * @param scale 窗口的缩放比例。
 * @return 取整后的宽度（逻辑像素）。
 */
[[nodiscard]] inline double snapBorderWidth(double width, double scale) {
  if (width <= 0) return 0;
  return std::max(1.0, std::floor(width * scale)) / scale;
}

/** 点。 */
struct Point {
  double x{0};
  double y{0};
};

/** 尺寸。 */
struct Size {
  double width{0};
  double height{0};

  [[nodiscard]] constexpr bool operator==(const Size&) const = default;
};

/** 四边的距离，用于内边距、外边距与边框宽度。 */
struct Insets {
  double top{0};
  double right{0};
  double bottom{0};
  double left{0};

  /** 四边相同。 */
  [[nodiscard]] static constexpr Insets all(double value) { return Insets{value, value, value, value}; }
  /** 上下与左右分别相同（对应 CSS 的两值简写 padding: 垂直 水平）。 */
  [[nodiscard]] static constexpr Insets symmetric(double vertical, double horizontal) { return Insets{vertical, horizontal, vertical, horizontal}; }
  /** 左右之和。 */
  [[nodiscard]] constexpr double horizontal() const { return left + right; }
  /** 上下之和。 */
  [[nodiscard]] constexpr double vertical() const { return top + bottom; }
};

/** 矩形。 */
struct Rect {
  double x{0};
  double y{0};
  double width{0};
  double height{0};

  /** 右边缘。 */
  [[nodiscard]] constexpr double right() const { return x + width; }
  /** 下边缘。 */
  [[nodiscard]] constexpr double bottom() const { return y + height; }
  /** 点是否在矩形内（含左上边缘，不含右下边缘）。 */
  [[nodiscard]] constexpr bool contains(Point point) const { return point.x >= x && point.x < right() && point.y >= y && point.y < bottom(); }
  /** 向内收缩；某个方向收缩后宽或高小于 0 时按 0 处理。 */
  [[nodiscard]] constexpr Rect inset(const Insets& insets) const {
    const double w = width - insets.horizontal();
    const double h = height - insets.vertical();
    return Rect{x + insets.left, y + insets.top, w < 0 ? 0 : w, h < 0 ? 0 : h};
  }
  /** 向外扩张。 */
  [[nodiscard]] constexpr Rect outset(const Insets& insets) const { return Rect{x - insets.left, y - insets.top, width + insets.horizontal(), height + insets.vertical()}; }
  [[nodiscard]] constexpr bool operator==(const Rect&) const = default;
};

}  // namespace rujian::ui
