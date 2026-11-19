/*
 * ------------------------------------------------------------------------
 * 名称：d2d-canvas.cpp
 * 说明：Direct2D 画布的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：阴影用多层逐渐变淡的圆角矩形近似（Direct2D 1.0 没有模糊效果），与网页版有细微差别，已记入已知差异。
 * ------------------------------------------------------------------------
 */

#include "base/d2d-canvas.hpp"

#include <algorithm>
#include <cmath>

#include "base/graphics.hpp"

namespace rujian::ui {

namespace {

D2D1_COLOR_F toD2D(Color color) {
  return D2D1::ColorF(color.r() / 255.0f, color.g() / 255.0f, color.b() / 255.0f, color.a() / 255.0f);
}

D2D1_RECT_F toD2D(const Rect& rect) {
  return D2D1::RectF(static_cast<FLOAT>(rect.x), static_cast<FLOAT>(rect.y), static_cast<FLOAT>(rect.right()), static_cast<FLOAT>(rect.bottom()));
}

D2D1_ROUNDED_RECT toRounded(const Rect& rect, double radius) {
  return D2D1::RoundedRect(toD2D(rect), static_cast<FLOAT>(radius), static_cast<FLOAT>(radius));
}

/** 阴影最多叠加的层数。 */
constexpr int kMaxShadowLayers = 24;

}  // namespace

D2DCanvas::D2DCanvas(ID2D1RenderTarget* target, double scale, Graphics& graphics) : target_(target), scale_(scale), graphics_(graphics) {
  target_->CreateSolidColorBrush(D2D1::ColorF(0), &brush_);
}

D2DCanvas::~D2DCanvas() = default;

double D2DCanvas::snap(double value) const { return std::round(value * scale_) / scale_; }

Rect D2DCanvas::snapRect(const Rect& rect) const {
  const double left = snap(rect.x);
  const double top = snap(rect.y);
  return Rect{left, top, snap(rect.right()) - left, snap(rect.bottom()) - top};
}

double D2DCanvas::snapBorder(double width) const {
  if (width <= 0) return 0;
  return std::max(1.0, std::floor(width * scale_)) / scale_;
}

void D2DCanvas::setBrush(Color color) { brush_->SetColor(toD2D(color)); }

void D2DCanvas::clear(Color color) { target_->Clear(toD2D(color)); }

void D2DCanvas::pushClip(const Rect& rect, double radius) {
  const Rect snapped = snapRect(rect);
  if (radius <= 0) {
    target_->PushAxisAlignedClip(toD2D(snapped), D2D1_ANTIALIAS_MODE_ALIASED);
    clips_.emplace_back();
    return;
  }
  Microsoft::WRL::ComPtr<ID2D1RoundedRectangleGeometry> geometry;
  Microsoft::WRL::ComPtr<ID2D1Layer> layer;
  graphics_.d2d()->CreateRoundedRectangleGeometry(toRounded(snapped, radius), &geometry);
  target_->CreateLayer(nullptr, &layer);
  target_->PushLayer(D2D1::LayerParameters(D2D1::InfiniteRect(), geometry.Get()), layer.Get());
  clips_.push_back(std::move(layer));
}

void D2DCanvas::popClip() {
  if (clips_.empty()) return;
  if (clips_.back()) target_->PopLayer();
  else target_->PopAxisAlignedClip();
  clips_.pop_back();
}

void D2DCanvas::fillRect(const Rect& rect, Color color) {
  if (color.a() == 0) return;
  setBrush(color);
  target_->FillRectangle(toD2D(snapRect(rect)), brush_.Get());
}

void D2DCanvas::drawBox(const Rect& rect, const BoxStyle& style) {
  const Rect outer = style.radius > 0 ? rect : snapRect(rect);
  if (outer.width <= 0 || outer.height <= 0) return;
  if (style.fill.a() != 0) {
    setBrush(style.fill);
    if (style.radius > 0) target_->FillRoundedRectangle(toRounded(outer, style.radius), brush_.Get());
    else target_->FillRectangle(toD2D(outer), brush_.Get());
  }
  const double border = snapBorder(style.borderWidth);
  if (border > 0 && style.borderColor.a() != 0) {
    setBrush(style.borderColor);
    // 边框画在盒子内侧：描边的中线在盒子边缘向内半个线宽处
    const Rect middle = outer.inset(Insets::all(border / 2));
    if (style.radius > 0) target_->DrawRoundedRectangle(toRounded(middle, std::max(0.0, style.radius - border / 2)), brush_.Get(), static_cast<FLOAT>(border));
    else target_->DrawRectangle(toD2D(middle), brush_.Get(), static_cast<FLOAT>(border));
  }
}

void D2DCanvas::strokeRoundedRect(const Rect& centerLine, double radius, double width, Color color) {
  if (width <= 0 || color.a() == 0 || centerLine.width <= 0 || centerLine.height <= 0) return;
  setBrush(color);
  target_->DrawRoundedRectangle(toRounded(centerLine, std::max(0.0, radius)), brush_.Get(), static_cast<FLOAT>(width));
}

void D2DCanvas::drawShadow(const Rect& rect, double radius, const ShadowList& shadows) {
  for (int s = shadows.count - 1; s >= 0; --s) {
    const Shadow& shadow = shadows.items[static_cast<std::size_t>(s)];
    if (shadow.inset || shadow.color.a() == 0) continue;
    const Rect base{rect.x + shadow.offsetX, rect.y + shadow.offsetY, rect.width, rect.height};
    const int layers = std::clamp(static_cast<int>(std::lround(shadow.blur)), 1, kMaxShadowLayers);
    // 每层的不透明度相同，叠起来中心最浓、向外逐渐变淡
    const double layerAlpha = 1.0 - std::pow(1.0 - shadow.color.a() / 255.0, 1.0 / layers);
    for (int i = 0; i < layers; ++i) {
      const double grow = shadow.spread + shadow.blur / 2.0 - (i + 0.5) * shadow.blur / layers;
      Color color = Color::rgba(shadow.color.r(), shadow.color.g(), shadow.color.b(), static_cast<std::uint8_t>(layerAlpha * 255.0 + 0.5));
      const Rect layerRect = base.outset(Insets::all(grow));
      if (layerRect.width <= 0 || layerRect.height <= 0) continue;
      setBrush(color);
      target_->FillRoundedRectangle(toRounded(layerRect, std::max(0.0, radius + grow)), brush_.Get());
    }
  }
}

void D2DCanvas::drawLine(const Rect& rect, Color color) {
  if (color.a() == 0) return;
  Rect line = snapRect(rect);
  // 较薄的一边至少 1 个设备像素
  if (rect.width <= rect.height) line.width = std::max(line.width, 1.0 / scale_);
  else line.height = std::max(line.height, 1.0 / scale_);
  setBrush(color);
  target_->FillRectangle(toD2D(line), brush_.Get());
}

void D2DCanvas::drawText(std::string_view text, const Rect& box, const TextStyle& style, VerticalAlign vertical) {
  if (text.empty() || style.color.a() == 0) return;
  const auto layout = graphics_.text().createLayout(text, style, box.width, box.height);
  if (!layout) return;
  DWRITE_TEXT_METRICS metrics{};
  layout->GetMetrics(&metrics);
  const double top = vertical == VerticalAlign::Center ? box.y + (box.height - metrics.height) / 2.0 : box.y;
  setBrush(style.color);
  target_->DrawTextLayout(D2D1::Point2F(static_cast<FLOAT>(snap(box.x)), static_cast<FLOAT>(snap(top))), layout.Get(), brush_.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);
}

TextMetrics D2DCanvas::measureText(std::string_view text, const TextStyle& style, double maxWidth) { return graphics_.text().measure(text, style, maxWidth); }

void D2DCanvas::drawIcon(IconId id, const Rect& rect, Color color, double strokeWidth) {
  if (color.a() == 0 || rect.width <= 0 || rect.height <= 0) return;
  // 图标在 24×24 的画布里，显示中间 20×20
  const double factor = std::min(rect.width, rect.height) / 20.0;
  const double left = snap(rect.x + (rect.width - 20.0 * factor) / 2.0);
  const double top = snap(rect.y + (rect.height - 20.0 * factor) / 2.0);
  D2D1_MATRIX_3X2_F previous{};
  target_->GetTransform(&previous);
  // 先应用调用方原有的变换，再叠加图标的平移与缩放
  const D2D1_MATRIX_3X2_F local = D2D1::Matrix3x2F::Translation(-2.0f, -2.0f) * D2D1::Matrix3x2F::Scale(static_cast<FLOAT>(factor), static_cast<FLOAT>(factor)) *
                                  D2D1::Matrix3x2F::Translation(static_cast<FLOAT>(left), static_cast<FLOAT>(top));
  target_->SetTransform(local * previous);
  for (const IconPart& part : graphics_.icon(id)) {
    setBrush(color.withOpacity(part.opacity));
    if (part.fill) target_->FillGeometry(part.geometry.Get(), brush_.Get());
    if (part.stroke) target_->DrawGeometry(part.geometry.Get(), brush_.Get(), static_cast<FLOAT>(strokeWidth / factor), graphics_.roundStroke());
  }
  target_->SetTransform(previous);
}

}  // namespace rujian::ui
