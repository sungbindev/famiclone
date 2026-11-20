/*
 * ------------------------------------------------------------------------
 * 名称：system-glyph.hpp
 * 说明：窗口系统按钮（最小化、最大化、还原、关闭）的图形与关闭按钮的颜色，标题栏与对话框框架共用。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-20
 * 备注：图形是 10×10 的线条，线宽 1，居中画在按钮区域里；线条按整数设备像素对齐，不用字体图标（Windows 7 没有那套字体）。
 * ------------------------------------------------------------------------
 */

#pragma once

#include "rujian/ui/canvas.hpp"
#include "rujian/ui/color.hpp"
#include "rujian/ui/geometry.hpp"

namespace rujian::ui {

/** 系统按钮的图形。 */
enum class SystemGlyph { Minimize, Maximize, Restore, Close };

/** 系统按钮里图形的边长与线宽。 */
inline constexpr double kSystemGlyphSize = 10.0;
inline constexpr double kSystemGlyphStroke = 1.0;

/** 关闭按钮悬停与按下的底色（系统的红色）与此时的图形色。 */
inline constexpr Color kSystemCloseHoverBackground = Color::rgba(0xC4, 0x2B, 0x1C);
inline constexpr Color kSystemClosePressedBackground = Color::rgba(0xB3, 0x28, 0x1A);
inline constexpr Color kSystemCloseGlyph = Color::rgba(0xFF, 0xFF, 0xFF);

/** 在按钮区域 box 的正中画一个系统按钮图形。 */
inline void paintSystemGlyph(Canvas& canvas, SystemGlyph glyph, const Rect& box, Color color) {
  const double left = box.x + (box.width - kSystemGlyphSize) / 2.0;
  const double top = box.y + (box.height - kSystemGlyphSize) / 2.0;
  const double edge = kSystemGlyphStroke;
  switch (glyph) {
    case SystemGlyph::Minimize:
      canvas.drawLine(Rect{left, top + kSystemGlyphSize / 2.0, kSystemGlyphSize, edge}, color);
      break;
    case SystemGlyph::Maximize:
      canvas.drawLine(Rect{left, top, kSystemGlyphSize, edge}, color);
      canvas.drawLine(Rect{left, top + kSystemGlyphSize - edge, kSystemGlyphSize, edge}, color);
      canvas.drawLine(Rect{left, top, edge, kSystemGlyphSize}, color);
      canvas.drawLine(Rect{left + kSystemGlyphSize - edge, top, edge, kSystemGlyphSize}, color);
      break;
    case SystemGlyph::Restore: {
      // 前面一个 8×8 的方框，后面一个只露出上边与右边的方框
      const double side = kSystemGlyphSize - 2.0;
      canvas.drawLine(Rect{left, top + 2.0, side, edge}, color);
      canvas.drawLine(Rect{left, top + 2.0 + side - edge, side, edge}, color);
      canvas.drawLine(Rect{left, top + 2.0, edge, side}, color);
      canvas.drawLine(Rect{left + side - edge, top + 2.0, edge, side}, color);
      canvas.drawLine(Rect{left + 2.0, top, kSystemGlyphSize - 2.0, edge}, color);
      canvas.drawLine(Rect{left + 2.0, top, edge, 2.0}, color);
      canvas.drawLine(Rect{left + kSystemGlyphSize - edge, top, edge, side}, color);
      canvas.drawLine(Rect{left + side, top + side - edge, 2.0, edge}, color);
      break;
    }
    case SystemGlyph::Close:
      canvas.strokeLine(Point{left, top}, Point{left + kSystemGlyphSize, top + kSystemGlyphSize}, kSystemGlyphStroke, color);
      canvas.strokeLine(Point{left + kSystemGlyphSize, top}, Point{left, top + kSystemGlyphSize}, kSystemGlyphStroke, color);
      break;
  }
}

}  // namespace rujian::ui
