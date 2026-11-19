/*
 * ------------------------------------------------------------------------
 * 名称：window-shadow-image.cpp
 * 说明：窗口外阴影图像的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "shared/window/window-shadow-image.hpp"

#include <algorithm>
#include <cmath>

namespace ember::win32::shared {

namespace {

/** 高斯模糊的标准差、向下的偏移与向四周延伸的距离（逻辑像素）。 */
constexpr double kSigma = 10.0;
constexpr double kOffsetY = 4.0;
constexpr int kMarginLogical = 36;
/** 阴影最深处的不透明度：激活时深，没有激活时浅。 */
constexpr double kActiveOpacity = 0.55;
constexpr double kInactiveOpacity = 0.32;

/** 一条长度为 length 的线段（从 start 起）经高斯模糊后，在位置 position 处的值，范围 0 到 1。 */
double blurred(double position, double start, double length, double sigma) {
  const double scale = 1.0 / (sigma * std::sqrt(2.0));
  return 0.5 * (std::erf((position - start) * scale) - std::erf((position - start - length) * scale));
}

}  // namespace

int shadowMargin(double scale) { return static_cast<int>(std::ceil(kMarginLogical * scale)); }

WindowShadowImage renderWindowShadow(int windowWidth, int windowHeight, double scale, bool active) {
  WindowShadowImage image;
  if (windowWidth <= 0 || windowHeight <= 0 || scale <= 0) return image;
  image.margin = shadowMargin(scale);
  image.width = windowWidth + 2 * image.margin;
  image.height = windowHeight + 2 * image.margin;
  image.pixels.assign(static_cast<std::size_t>(image.width) * static_cast<std::size_t>(image.height), 0u);

  const double sigma = kSigma * scale;
  const double opacity = active ? kActiveOpacity : kInactiveOpacity;
  std::vector<double> horizontal(static_cast<std::size_t>(image.width));
  std::vector<double> vertical(static_cast<std::size_t>(image.height));
  for (int x = 0; x < image.width; ++x) horizontal[static_cast<std::size_t>(x)] = blurred(x + 0.5, image.margin, windowWidth, sigma);
  for (int y = 0; y < image.height; ++y) vertical[static_cast<std::size_t>(y)] = blurred(y + 0.5, image.margin + kOffsetY * scale, windowHeight, sigma);

  for (int y = 0; y < image.height; ++y) {
    const double row = vertical[static_cast<std::size_t>(y)] * opacity;
    std::uint32_t* line = image.pixels.data() + static_cast<std::size_t>(y) * static_cast<std::size_t>(image.width);
    for (int x = 0; x < image.width; ++x) {
      const int alpha = static_cast<int>(std::lround(row * horizontal[static_cast<std::size_t>(x)] * 255.0));
      line[x] = static_cast<std::uint32_t>(std::clamp(alpha, 0, 255)) << 24;
    }
  }
  return image;
}

}  // namespace ember::win32::shared
