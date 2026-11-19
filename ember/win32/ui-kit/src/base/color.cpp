/*
 * ------------------------------------------------------------------------
 * 名称：color.cpp
 * 说明：颜色运算的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：合成使用不预乘的 source-over 公式。
 * ------------------------------------------------------------------------
 */

#include "rujian/ui/color.hpp"

namespace rujian::ui {

Color compose(Color foreground, Color background) {
  const double fa = foreground.a() / 255.0;
  const double ba = background.a() / 255.0;
  const double outAlpha = fa + ba * (1.0 - fa);
  if (outAlpha <= 0.0) return kTransparent;
  const auto channel = [&](std::uint8_t f, std::uint8_t b) {
    const double value = (f * fa + b * ba * (1.0 - fa)) / outAlpha;
    return static_cast<std::uint8_t>(value + 0.5);
  };
  return Color::rgba(channel(foreground.r(), background.r()), channel(foreground.g(), background.g()), channel(foreground.b(), background.b()), static_cast<std::uint8_t>(outAlpha * 255.0 + 0.5));
}

}  // namespace rujian::ui
