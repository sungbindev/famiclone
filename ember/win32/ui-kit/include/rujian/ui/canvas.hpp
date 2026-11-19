/*
 * ------------------------------------------------------------------------
 * 名称：canvas.hpp
 * 说明：画布接口：控件只通过它绘制，坐标与长度都是逻辑像素，由实现按窗口缩放比例换算并对齐到设备像素。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：边框按 CSS 的规则处理：边框画在盒子内侧，宽度取设备像素的整数倍且至少 1 个设备像素；
 *       盒子的边缘对齐到设备像素，保证和网页版在各种缩放比例下一样清晰。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <string_view>

#include "rujian/ui/color.hpp"
#include "rujian/ui/geometry.hpp"
#include "rujian/ui/icon-id.hpp"
#include "rujian/ui/text.hpp"
#include "rujian/ui/token-types.hpp"

namespace rujian::ui {

/** 盒子的外观（对应 CSS 的 background、border、border-radius）。 */
struct BoxStyle {
  /** 背景色；透明表示不填充。 */
  Color fill{kTransparent};
  /** 圆角半径。 */
  double radius{0};
  /** 边框宽度；0 表示没有边框。 */
  double borderWidth{0};
  /** 边框颜色。 */
  Color borderColor{kTransparent};
};

/** 文字在盒子里的垂直位置。 */
enum class VerticalAlign { Top, Center };

/** 画布。 */
class Canvas {
 public:
  virtual ~Canvas() = default;

  /** 窗口的缩放比例（设备像素 / 逻辑像素）。 */
  [[nodiscard]] virtual double scale() const = 0;

  /** 用颜色铺满整个画布。 */
  virtual void clear(Color color) = 0;

  /** 开始一个矩形裁剪区；radius 大于 0 时是圆角裁剪。每次调用必须有对应的 popClip。 */
  virtual void pushClip(const Rect& rect, double radius = 0) = 0;

  /** 结束最近一次的裁剪区。 */
  virtual void popClip() = 0;

  /** 填充矩形，边缘对齐到设备像素。 */
  virtual void fillRect(const Rect& rect, Color color) = 0;

  /** 画盒子：背景、圆角与内侧边框。无圆角时边缘对齐到设备像素；有圆角时不对齐（网页版的圆角背景在分数缩放下也不对齐，边缘是抗锯齿的渐变）。 */
  virtual void drawBox(const Rect& rect, const BoxStyle& style) = 0;

  /**
   * 沿圆角矩形的中线描边，位置与线宽都不做像素对齐（按设备像素的小数覆盖率抗锯齿）。
   * 用于焦点环：网页版的 outline 就是沿“边框框向外偏移后的位置”描一条线，线宽 1px 实测只有 2/3 个逻辑像素。
   * @param centerLine 中线所在的矩形。
   * @param radius 中线处的圆角半径。
   * @param width 线宽（逻辑像素）。
   * @param color 颜色。
   */
  virtual void strokeRoundedRect(const Rect& centerLine, double radius, double width, Color color) = 0;

  /** 画盒子的阴影（box-shadow）；rect 与 radius 是盒子本身的外框。 */
  virtual void drawShadow(const Rect& rect, double radius, const ShadowList& shadows) = 0;

  /** 画水平或垂直的线，线宽取设备像素的整数倍且至少 1 个设备像素；线的位置是它占据的矩形。 */
  virtual void drawLine(const Rect& rect, Color color) = 0;

  /** 画文字；颜色为 kCurrentColor 时无效，使用方必须先换成具体颜色。 */
  virtual void drawText(std::string_view text, const Rect& box, const TextStyle& style, VerticalAlign vertical) = 0;

  /** 度量文字；maxWidth 为 0 表示不限宽度。 */
  [[nodiscard]] virtual TextMetrics measureText(std::string_view text, const TextStyle& style, double maxWidth) = 0;

  /**
   * 画图标：把 24×24 画布中间 20×20 的区域缩放到 rect，描边宽度不随图标尺寸变化。
   * @param id 图标编号。
   * @param rect 图标的显示区域（一般是正方形）。
   * @param color 图标颜色。
   * @param strokeWidth 描边宽度（逻辑像素）。
   */
  virtual void drawIcon(IconId id, const Rect& rect, Color color, double strokeWidth) = 0;
};

}  // namespace rujian::ui
