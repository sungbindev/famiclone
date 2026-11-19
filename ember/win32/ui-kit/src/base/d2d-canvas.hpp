/*
 * ------------------------------------------------------------------------
 * 名称：d2d-canvas.hpp
 * 说明：用 Direct2D 1.0 实现的画布，可画到窗口或内存位图上。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：渲染目标的 DPI 必须设为 96 × 缩放比例，这样 Direct2D 的“设备无关像素”就等于逻辑像素。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <d2d1.h>
#include <wrl/client.h>

#include <vector>

#include "rujian/ui/canvas.hpp"

namespace rujian::ui {

class Graphics;

/** Direct2D 画布。 */
class D2DCanvas final : public Canvas {
 public:
  /**
   * @param target 渲染目标，调用方负责 BeginDraw 与 EndDraw，并保证画布存活期间它有效。
   * @param scale 缩放比例。
   * @param graphics 图形资源。
   */
  D2DCanvas(ID2D1RenderTarget* target, double scale, Graphics& graphics);
  ~D2DCanvas() override;

  [[nodiscard]] double scale() const override { return scale_; }
  void clear(Color color) override;
  void pushClip(const Rect& rect, double radius) override;
  void popClip() override;
  void fillRect(const Rect& rect, Color color) override;
  void fillRectSmooth(const Rect& rect, Color color) override;
  void strokeLine(Point from, Point to, double width, Color color) override;
  void fillPolygon(const std::vector<Point>& points, Color color) override;
  void fillRotatedCorner(double left, double top, double width, double height, double stroke, double angleDegrees, double shiftY, Color color) override;
  void fillInsetShadow(const Rect& paddingBox, double radius, const Insets& border, Point offset, Color color) override;
  void drawBox(const Rect& rect, const BoxStyle& style) override;
  void strokeOutline(const Rect& borderBox, double radius, double offset, double width, Color color) override;
  void drawShadow(const Rect& rect, double radius, const ShadowList& shadows) override;
  void drawLine(const Rect& rect, Color color) override;
  void drawText(std::string_view text, const Rect& box, const TextStyle& style, VerticalAlign vertical) override;
  void drawText16(std::u16string_view text, const Rect& box, const TextStyle& style, VerticalAlign vertical) override;
  [[nodiscard]] TextMetrics measureText(std::string_view text, const TextStyle& style, double maxWidth) override;
  void drawIcon(IconId id, const Rect& rect, Color color, double strokeWidth) override;

 private:
  /** 把逻辑坐标对齐到最近的设备像素。 */
  [[nodiscard]] double snap(double value) const;
  [[nodiscard]] Rect snapRect(const Rect& rect) const;
  /** 边框宽度：设备像素的整数倍，至少 1 个。 */
  [[nodiscard]] double snapBorder(double width) const;
  void setBrush(Color color);

  ID2D1RenderTarget* target_;
  double scale_;
  Graphics& graphics_;
  Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush_;
  /** 裁剪栈：圆角裁剪用图层，保存图层以保证它在使用期间存活。 */
  std::vector<Microsoft::WRL::ComPtr<ID2D1Layer>> clips_;
};

}  // namespace rujian::ui
