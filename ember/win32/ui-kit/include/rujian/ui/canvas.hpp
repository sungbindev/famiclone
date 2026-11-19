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

#include <cstdint>
#include <string_view>
#include <vector>

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

  /** 填充矩形，边缘不对齐设备像素（按覆盖率抗锯齿），用于网页版里也不对齐的填充，如文字选区。 */
  virtual void fillRectSmooth(const Rect& rect, Color color) = 0;

  /** 画盒子：背景、圆角与内侧边框。和网页版一样，盒子的边缘对齐到设备像素（有圆角时也对齐）。 */
  virtual void drawBox(const Rect& rect, const BoxStyle& style) = 0;

  /**
   * 画 CSS 的 outline：沿边框框向外偏移 offset 的位置描一圈 width 宽的线。
   * 和网页版一样，线宽取设备像素的整数倍且至少 1 个设备像素，偏移向零取整到设备像素；
   * 位置不再对齐像素，圆角随偏移与线宽一起外扩（offset 为负时向内）。
   * @param borderBox 元素的边框框。
   * @param radius 元素的圆角半径。
   * @param offset outline-offset（逻辑像素）。
   * @param width outline-width（逻辑像素）。
   * @param color 颜色。
   */
  virtual void strokeOutline(const Rect& borderBox, double radius, double offset, double width, Color color) = 0;

  /** 沿中线描一条直线，平头线帽，不对齐像素。 */
  virtual void strokeLine(Point from, Point to, double width, Color color) = 0;

  /** 填充多边形（按顶点顺序连接并闭合），不对齐像素；用于旋转后的箭头与对勾。 */
  virtual void fillPolygon(const std::vector<Point>& points, Color color) = 0;

  /**
   * 画“右边与下边各有一段边框的方块”顺时针旋转后的样子（网页版里用 CSS 边框加 transform: rotate 画箭头与对勾的写法）。
   * 方块自己的位置与尺寸、边框宽度都按网页版的规则取整到设备像素，调用方只给逻辑像素的 CSS 数值。
   * @param left 方块左边（未取整）。
   * @param top 方块上边（未取整，不含 translate）。
   * @param width 方块宽（含边框）。
   * @param height 方块高（含边框）。
   * @param stroke 右边与下边的边框宽度。
   * @param angleDegrees 顺时针旋转的角度。
   * @param shiftY 旋转前的竖直平移（向下为正）。
   * @param color 颜色。
   */
  virtual void fillRotatedCorner(double left, double top, double width, double height, double stroke, double angleDegrees, double shiftY, Color color) = 0;

  /**
   * 画没有模糊的内阴影（inset box-shadow）：填充“内边距框”减去“同样的框平移 offset”后剩下的部分。
   * 内边距框的圆角按 CSS 规则算：水平、垂直分别是外圆角半径减去相邻两边的边框宽度，不小于 0。
   * 坐标不对齐像素，框应先由调用方对齐到设备像素（网页版是这样算的）。
   * @param paddingBox 阴影所在的内边距框。
   * @param radius 元素的外圆角半径，0 表示直角。
   * @param border 元素四边的边框宽度。
   * @param offset 阴影的偏移（即 box-shadow 的 x、y）。
   * @param color 颜色。
   */
  virtual void fillInsetShadow(const Rect& paddingBox, double radius, const Insets& border, Point offset, Color color) = 0;

  /** 画盒子的阴影（box-shadow）；rect 与 radius 是盒子本身的外框。 */
  virtual void drawShadow(const Rect& rect, double radius, const ShadowList& shadows) = 0;

  /** 画水平或垂直的线，线宽取设备像素的整数倍且至少 1 个设备像素；线的位置是它占据的矩形。 */
  virtual void drawLine(const Rect& rect, Color color) = 0;

  /** 画文字；颜色为 kCurrentColor 时无效，使用方必须先换成具体颜色。 */
  virtual void drawText(std::string_view text, const Rect& box, const TextStyle& style, VerticalAlign vertical) = 0;

  /** 同 drawText，文字是 UTF-16。 */
  virtual void drawText16(std::u16string_view text, const Rect& box, const TextStyle& style, VerticalAlign vertical) = 0;

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

  /**
   * 画位图，边缘对齐到设备像素；位图尺寸与对齐后的区域一样大时一个像素对应一个设备像素，不模糊。
   * @param pixels 像素，每个 32 位按 0xAARRGGBB 排列，颜色已预乘透明度，行优先、没有行间空隙。
   * @param width 位图宽（像素）。
   * @param height 位图高（像素）。
   * @param rect 显示区域。
   */
  virtual void drawBitmap(const std::uint32_t* pixels, int width, int height, const Rect& rect) = 0;
};

}  // namespace rujian::ui
