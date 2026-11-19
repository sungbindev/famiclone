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
#include "base/unicode.hpp"

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
constexpr int kMaxShadowLayers = 48;

/** 四个圆角各自的水平、垂直半径（椭圆角）。 */
struct CornerRadii {
  double topLeftX{0}, topLeftY{0}, topRightX{0}, topRightY{0};
  double bottomRightX{0}, bottomRightY{0}, bottomLeftX{0}, bottomLeftY{0};
};

/** 用路径拼一个四角半径各不相同的圆角矩形。 */
Microsoft::WRL::ComPtr<ID2D1PathGeometry> roundedPath(ID2D1Factory* factory, const Rect& r, const CornerRadii& c) {
  Microsoft::WRL::ComPtr<ID2D1PathGeometry> path;
  Microsoft::WRL::ComPtr<ID2D1GeometrySink> sink;
  if (FAILED(factory->CreatePathGeometry(&path)) || FAILED(path->Open(&sink))) return nullptr;
  const auto point = [](double x, double y) { return D2D1::Point2F(static_cast<FLOAT>(x), static_cast<FLOAT>(y)); };
  const auto arc = [&](double x, double y, double rx, double ry) {
    if (rx <= 0 || ry <= 0) {
      sink->AddLine(point(x, y));
      return;
    }
    sink->AddArc(D2D1::ArcSegment(point(x, y), D2D1::SizeF(static_cast<FLOAT>(rx), static_cast<FLOAT>(ry)), 0.0f, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));
  };
  sink->BeginFigure(point(r.x + c.topLeftX, r.y), D2D1_FIGURE_BEGIN_FILLED);
  sink->AddLine(point(r.right() - c.topRightX, r.y));
  arc(r.right(), r.y + c.topRightY, c.topRightX, c.topRightY);
  sink->AddLine(point(r.right(), r.bottom() - c.bottomRightY));
  arc(r.right() - c.bottomRightX, r.bottom(), c.bottomRightX, c.bottomRightY);
  sink->AddLine(point(r.x + c.bottomLeftX, r.bottom()));
  arc(r.x, r.bottom() - c.bottomLeftY, c.bottomLeftX, c.bottomLeftY);
  sink->AddLine(point(r.x, r.y + c.topLeftY));
  arc(r.x + c.topLeftX, r.y, c.topLeftX, c.topLeftY);
  sink->EndFigure(D2D1_FIGURE_END_CLOSED);
  if (FAILED(sink->Close())) return nullptr;
  return path;
}

}  // namespace

D2DCanvas::D2DCanvas(ID2D1RenderTarget* target, double scale, Graphics& graphics) : target_(target), scale_(scale), graphics_(graphics) {
  target_->CreateSolidColorBrush(D2D1::ColorF(0), &brush_);
}

D2DCanvas::~D2DCanvas() = default;

double D2DCanvas::snap(double value) const {
  // 加一个很小的量：边缘恰好落在半个像素上时，浮点误差不能让它往下取整（网页版的布局单位是精确的 1/64 像素）
  return std::round(value * scale_ + 1e-6) / scale_;
}

Rect D2DCanvas::snapRect(const Rect& rect) const {
  const double left = snap(rect.x);
  const double top = snap(rect.y);
  return Rect{left, top, snap(rect.right()) - left, snap(rect.bottom()) - top};
}

double D2DCanvas::snapBorder(double width) const { return snapBorderWidth(width, scale_); }

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

void D2DCanvas::fillRectSmooth(const Rect& rect, Color color) {
  if (color.a() == 0) return;
  setBrush(color);
  target_->FillRectangle(toD2D(rect), brush_.Get());
}

void D2DCanvas::strokeLine(Point from, Point to, double width, Color color) {
  if (color.a() == 0 || width <= 0) return;
  setBrush(color);
  target_->DrawLine(D2D1::Point2F(static_cast<FLOAT>(from.x), static_cast<FLOAT>(from.y)), D2D1::Point2F(static_cast<FLOAT>(to.x), static_cast<FLOAT>(to.y)), brush_.Get(), static_cast<FLOAT>(width));
}

void D2DCanvas::fillPolygon(const std::vector<Point>& points, Color color) {
  if (color.a() == 0 || points.size() < 3) return;
  Microsoft::WRL::ComPtr<ID2D1PathGeometry> path;
  Microsoft::WRL::ComPtr<ID2D1GeometrySink> sink;
  if (FAILED(graphics_.d2d()->CreatePathGeometry(&path)) || FAILED(path->Open(&sink))) return;
  const auto toPoint = [](Point point) { return D2D1::Point2F(static_cast<FLOAT>(point.x), static_cast<FLOAT>(point.y)); };
  sink->BeginFigure(toPoint(points.front()), D2D1_FIGURE_BEGIN_FILLED);
  for (std::size_t i = 1; i < points.size(); ++i) sink->AddLine(toPoint(points[i]));
  sink->EndFigure(D2D1_FIGURE_END_CLOSED);
  if (FAILED(sink->Close())) return;
  setBrush(color);
  target_->FillGeometry(path.Get(), brush_.Get());
}

void D2DCanvas::fillRotatedCorner(double left, double top, double width, double height, double stroke, double angleDegrees, double shiftY, Color color) {
  // 网页版的做法：方块自己的位置与尺寸各自按设备像素取整，绕取整后方块的中心旋转；边框宽度也按设备像素取整。
  // 下面的长度与坐标都是设备像素，最后才换回逻辑像素。
  const double L = left * scale_;
  const double T = top * scale_;
  const double lw = std::round(width * scale_);
  const double lh = std::round(height * scale_);
  const double rectLeft = std::round(L);
  const double rectTop = std::round(T);
  const double t = snapBorderWidth(stroke, scale_) * scale_;
  const double pivotX = rectLeft + lw / 2.0;
  const double pivotY = rectTop + lh / 2.0;
  const double angle = angleDegrees * 3.14159265358979323846 / 180.0;
  const double c = std::cos(angle);
  const double s = std::sin(angle);
  const auto point = [&](double x, double y) {
    const double dx = x - pivotX;
    const double dy = y - pivotY;
    return Point{(pivotX + dx * c - dy * s) / scale_, (pivotY + shiftY * scale_ + dx * s + dy * c) / scale_};
  };
  fillPolygon({point(rectLeft + lw - t, rectTop), point(rectLeft + lw, rectTop), point(rectLeft + lw, rectTop + lh), point(rectLeft, rectTop + lh), point(rectLeft, rectTop + lh - t), point(rectLeft + lw - t, rectTop + lh - t)}, color);
}

void D2DCanvas::fillInsetShadow(const Rect& paddingBox, double radius, const Insets& border, Point offset, Color color) {
  if (color.a() == 0 || paddingBox.width <= 0 || paddingBox.height <= 0) return;
  ID2D1Factory* factory = graphics_.d2d();
  // 内圆角的水平、垂直半径 = 外圆角半径 - 相邻两边的边框宽度
  const auto inner = [&](double width) { return std::max(0.0, radius - width); };
  const CornerRadii radii{inner(border.left), inner(border.top), inner(border.right), inner(border.top), inner(border.right), inner(border.bottom), inner(border.left), inner(border.bottom)};
  const Rect shifted{paddingBox.x + offset.x, paddingBox.y + offset.y, paddingBox.width, paddingBox.height};
  const auto outer = roundedPath(factory, paddingBox, radii);
  const auto hole = roundedPath(factory, shifted, radii);
  if (!outer || !hole) return;
  Microsoft::WRL::ComPtr<ID2D1PathGeometry> path;
  Microsoft::WRL::ComPtr<ID2D1GeometrySink> sink;
  if (FAILED(factory->CreatePathGeometry(&path)) || FAILED(path->Open(&sink))) return;
  const HRESULT combined = outer->CombineWithGeometry(hole.Get(), D2D1_COMBINE_MODE_EXCLUDE, nullptr, sink.Get());
  const HRESULT closed = sink->Close();
  if (FAILED(combined) || FAILED(closed)) return;
  setBrush(color);
  target_->FillGeometry(path.Get(), brush_.Get());
}

void D2DCanvas::drawBox(const Rect& rect, const BoxStyle& style) {
  const Rect outer = snapRect(rect);
  if (outer.width <= 0 || outer.height <= 0) return;
  if (style.fill.a() != 0) {
    setBrush(style.fill);
    if (style.radius > 0) target_->FillRoundedRectangle(toRounded(outer, style.radius), brush_.Get());
    else target_->FillRectangle(toD2D(outer), brush_.Get());
  }
  // 边框宽度取设备像素的整数倍且至少 1 个设备像素
  const double border = snapBorder(style.borderWidth);
  if (border > 0 && style.borderColor.a() != 0) {
    setBrush(style.borderColor);
    // 边框画在盒子内侧：描边的中线在盒子边缘向内半个线宽处
    const Rect middle = outer.inset(Insets::all(border / 2));
    if (style.radius > 0) target_->DrawRoundedRectangle(toRounded(middle, std::max(0.0, style.radius - border / 2)), brush_.Get(), static_cast<FLOAT>(border));
    else target_->DrawRectangle(toD2D(middle), brush_.Get(), static_cast<FLOAT>(border));
  }
}

void D2DCanvas::strokeOutline(const Rect& borderBox, double radius, double offset, double width, Color color) {
  if (width <= 0 || color.a() == 0) return;
  const double lineWidth = snapBorder(width);
  const double shift = std::trunc(offset * scale_) / scale_;
  const double grow = shift + lineWidth / 2.0;
  const Rect middle = snapRect(borderBox).outset(Insets::all(grow));
  if (middle.width <= 0 || middle.height <= 0) return;
  setBrush(color);
  target_->DrawRoundedRectangle(toRounded(middle, std::max(0.0, radius + grow)), brush_.Get(), static_cast<FLOAT>(lineWidth));
}

void D2DCanvas::drawShadow(const Rect& rect, double radius, const ShadowList& shadows) {
  const Rect box = snapRect(rect);
  if (box.width <= 0 || box.height <= 0) return;
  // box-shadow 不画在元素自己的框里面（背景半透明时才看得出来）：用“整个画布减去圆角框”做遮罩
  Microsoft::WRL::ComPtr<ID2D1Layer> layer;
  {
    ID2D1Factory* factory = graphics_.d2d();
    Microsoft::WRL::ComPtr<ID2D1RoundedRectangleGeometry> hole;
    Microsoft::WRL::ComPtr<ID2D1RectangleGeometry> everything;
    Microsoft::WRL::ComPtr<ID2D1PathGeometry> outside;
    Microsoft::WRL::ComPtr<ID2D1GeometrySink> sink;
    constexpr FLOAT kHuge = 100000.0f;
    if (SUCCEEDED(factory->CreateRoundedRectangleGeometry(toRounded(box, radius), &hole)) && SUCCEEDED(factory->CreateRectangleGeometry(D2D1::RectF(-kHuge, -kHuge, kHuge, kHuge), &everything)) &&
        SUCCEEDED(factory->CreatePathGeometry(&outside)) && SUCCEEDED(outside->Open(&sink))) {
      const HRESULT combined = everything->CombineWithGeometry(hole.Get(), D2D1_COMBINE_MODE_EXCLUDE, nullptr, sink.Get());
      if (SUCCEEDED(sink->Close()) && SUCCEEDED(combined) && SUCCEEDED(target_->CreateLayer(nullptr, &layer))) target_->PushLayer(D2D1::LayerParameters(D2D1::InfiniteRect(), outside.Get()), layer.Get());
    }
  }
  for (int s = shadows.count - 1; s >= 0; --s) {
    const Shadow& shadow = shadows.items[static_cast<std::size_t>(s)];
    if (shadow.inset || shadow.color.a() == 0) continue;
    const Rect base{box.x + shadow.offsetX, box.y + shadow.offsetY, box.width, box.height};
    const double opacity = shadow.color.a() / 255.0;
    const double sigma = shadow.blur / 2.0;
    const auto fillLayer = [&](double grow, double alpha) {
      const Rect layerRect = base.outset(Insets::all(grow));
      if (layerRect.width <= 0 || layerRect.height <= 0 || alpha <= 0) return;
      brush_->SetColor(D2D1::ColorF(shadow.color.r() / 255.0f, shadow.color.g() / 255.0f, shadow.color.b() / 255.0f, static_cast<FLOAT>(alpha)));
      target_->FillRoundedRectangle(toRounded(layerRect, std::max(0.0, radius + grow)), brush_.Get());
    };
    if (sigma <= 0) {
      fillLayer(shadow.spread, opacity);
      continue;
    }
    // 高斯模糊的方框边缘：到（含扩展的）边缘外 x 处的覆盖率是 opacity × Φ(-x/σ)；
    // 用若干层由外到内逐层叠加的圆角矩形逼近，每层的不透明度由“累计覆盖率”反推
    const int steps = std::clamp(static_cast<int>(std::ceil(shadow.blur * 1.5)), 8, kMaxShadowLayers);
    const double reach = 2.5 * sigma;
    const double step = 2.0 * reach / steps;
    double previous = 0.0;
    for (int i = 0; i < steps; ++i) {
      const double grow = shadow.spread + reach - i * step;
      const double x = grow - step / 2.0 - shadow.spread;
      const double cumulative = opacity * 0.5 * std::erfc(x / sigma / std::sqrt(2.0));
      const double alpha = 1.0 - (1.0 - cumulative) / (1.0 - previous);
      previous = cumulative;
      fillLayer(grow, alpha);
    }
  }
  if (layer) target_->PopLayer();
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

void D2DCanvas::drawText(std::string_view text, const Rect& box, const TextStyle& style, VerticalAlign vertical) { drawText16(std::u16string_view(toUtf16(text)), box, style, vertical); }

void D2DCanvas::drawText16(std::u16string_view text, const Rect& box, const TextStyle& style, VerticalAlign vertical) {
  if (text.empty() || style.color.a() == 0) return;
  graphics_.text().setScale(scale_);
  const auto layout = graphics_.text().createLayout(text, style, box.width, box.height);
  if (!layout) return;
  DWRITE_TEXT_METRICS metrics{};
  layout->GetMetrics(&metrics);
  const double top = vertical == VerticalAlign::Center ? box.y + (box.height - metrics.height) / 2.0 : box.y;
  setBrush(style.color);
  if (style.letterSpacing != 0 && !style.wrap) {
    // DirectWrite 1.0（Windows 7）没有字间距：按整段排版得到的各簇位置，把每个字符簇单独画出来，簇之间再加字间距
    TextStyle plain = style;
    plain.letterSpacing = 0;
    const double spacing = style.letterSpacing * style.fontSize;
    double x = snap(box.x);
    std::size_t at = 0;
    for (const DWRITE_CLUSTER_METRICS& cluster : TextEngine::clusterMetrics(layout.Get())) {
      if (cluster.isWhitespace == FALSE) {
        if (const auto piece = graphics_.text().createLayout(text.substr(at, cluster.length), plain, 0, 0)) {
          target_->DrawTextLayout(D2D1::Point2F(static_cast<FLOAT>(x), static_cast<FLOAT>(snap(top))), piece.Get(), brush_.Get(), D2D1_DRAW_TEXT_OPTIONS_NONE);
        }
      }
      at += cluster.length;
      x += cluster.width + spacing;
    }
    return;
  }
  target_->DrawTextLayout(D2D1::Point2F(static_cast<FLOAT>(snap(box.x)), static_cast<FLOAT>(snap(top))), layout.Get(), brush_.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);
}

TextMetrics D2DCanvas::measureText(std::string_view text, const TextStyle& style, double maxWidth) {
  graphics_.text().setScale(scale_);
  return graphics_.text().measure(text, style, maxWidth);
}

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

void D2DCanvas::drawBitmap(const std::uint32_t* pixels, int width, int height, const Rect& rect) {
  if (pixels == nullptr || width <= 0 || height <= 0 || rect.width <= 0 || rect.height <= 0) return;
  // 位图的 DPI 与渲染目标一致，对齐后尺寸相同时一个像素对应一个设备像素
  const auto dpi = static_cast<FLOAT>(96.0 * scale_);
  const D2D1_BITMAP_PROPERTIES properties = D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), dpi, dpi);
  Microsoft::WRL::ComPtr<ID2D1Bitmap> bitmap;
  if (FAILED(target_->CreateBitmap(D2D1::SizeU(static_cast<UINT>(width), static_cast<UINT>(height)), pixels, static_cast<UINT>(width) * 4, properties, &bitmap))) return;
  target_->DrawBitmap(bitmap.Get(), toD2D(snapRect(rect)), 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
}

}  // namespace rujian::ui
