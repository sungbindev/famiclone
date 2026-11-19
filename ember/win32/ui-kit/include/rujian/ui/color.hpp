/*
 * ------------------------------------------------------------------------
 * 名称：color.hpp
 * 说明：颜色：32 位的 ARGB 值，以及混合、变更透明度等基本运算。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：颜色分量都是 8 位，不预乘透明度；“currentColor”令牌用 kCurrentColor 这个标记表示，使用方换成当前文字颜色。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>

namespace rujian::ui {

/** 颜色，内存布局为 0xAARRGGBB。 */
struct Color {
  /** 32 位 ARGB 值。 */
  std::uint32_t argb;

  /** 透明度分量，0 到 255。 */
  [[nodiscard]] constexpr std::uint8_t a() const { return static_cast<std::uint8_t>(argb >> 24); }
  /** 红色分量。 */
  [[nodiscard]] constexpr std::uint8_t r() const { return static_cast<std::uint8_t>(argb >> 16); }
  /** 绿色分量。 */
  [[nodiscard]] constexpr std::uint8_t g() const { return static_cast<std::uint8_t>(argb >> 8); }
  /** 蓝色分量。 */
  [[nodiscard]] constexpr std::uint8_t b() const { return static_cast<std::uint8_t>(argb); }

  /** 用分量创建颜色。 */
  [[nodiscard]] static constexpr Color rgba(std::uint8_t red, std::uint8_t green, std::uint8_t blue, std::uint8_t alpha = 255) {
    return Color{(static_cast<std::uint32_t>(alpha) << 24) | (static_cast<std::uint32_t>(red) << 16) | (static_cast<std::uint32_t>(green) << 8) | blue};
  }

  /** 把透明度乘以一个 0 到 1 的系数，返回新颜色。 */
  [[nodiscard]] constexpr Color withOpacity(double factor) const {
    const double scaled = a() * (factor < 0.0 ? 0.0 : (factor > 1.0 ? 1.0 : factor));
    return rgba(r(), g(), b(), static_cast<std::uint8_t>(scaled + 0.5));
  }

  [[nodiscard]] constexpr bool operator==(const Color&) const = default;
};

/** 表示“跟随当前文字颜色”的标记值（令牌取值为 currentColor 时使用）。 */
inline constexpr Color kCurrentColor{0x01000000u};

/** 完全透明。 */
inline constexpr Color kTransparent{0x00000000u};

/**
 * 把前景色叠在背景色上，得到不透明或半透明的结果（普通的 source-over 合成）。
 * @param foreground 前景色。
 * @param background 背景色。
 * @return 合成后的颜色。
 */
[[nodiscard]] Color compose(Color foreground, Color background);

}  // namespace rujian::ui
