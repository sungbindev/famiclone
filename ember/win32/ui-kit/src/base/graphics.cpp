/*
 * ------------------------------------------------------------------------
 * 名称：graphics.cpp
 * 说明：图形资源的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：图标路径由 svg-path 解析后转成 Direct2D 的路径几何；弧段直接用 D2D 的弧线段。
 * ------------------------------------------------------------------------
 */

#include "base/graphics.hpp"

#include <objbase.h>

#include <stdexcept>

#include "base/icon-data.hpp"
#include "base/svg-path.hpp"

namespace rujian::ui {

namespace {

D2D1_POINT_2F toD2D(Point p) { return D2D1::Point2F(static_cast<FLOAT>(p.x), static_cast<FLOAT>(p.y)); }

/** 把解析好的路径段写进几何。 */
bool buildGeometry(ID2D1Factory* factory, const std::vector<PathSegment>& segments, bool filled, Microsoft::WRL::ComPtr<ID2D1PathGeometry>& geometry) {
  if (FAILED(factory->CreatePathGeometry(&geometry))) return false;
  Microsoft::WRL::ComPtr<ID2D1GeometrySink> sink;
  if (FAILED(geometry->Open(&sink))) return false;
  const D2D1_FIGURE_BEGIN begin = filled ? D2D1_FIGURE_BEGIN_FILLED : D2D1_FIGURE_BEGIN_HOLLOW;
  bool open = false;
  for (const PathSegment& segment : segments) {
    switch (segment.kind) {
      case PathSegment::Kind::Move:
        if (open) sink->EndFigure(D2D1_FIGURE_END_OPEN);
        sink->BeginFigure(toD2D(segment.end), begin);
        open = true;
        break;
      case PathSegment::Kind::Line:
        sink->AddLine(toD2D(segment.end));
        break;
      case PathSegment::Kind::Cubic:
        sink->AddBezier(D2D1::BezierSegment(toD2D(segment.control1), toD2D(segment.control2), toD2D(segment.end)));
        break;
      case PathSegment::Kind::Quad:
        sink->AddQuadraticBezier(D2D1::QuadraticBezierSegment(toD2D(segment.control1), toD2D(segment.end)));
        break;
      case PathSegment::Kind::Arc:
        sink->AddArc(D2D1::ArcSegment(toD2D(segment.end), D2D1::SizeF(static_cast<FLOAT>(segment.radiusX), static_cast<FLOAT>(segment.radiusY)), static_cast<FLOAT>(segment.rotation),
                                      segment.sweep ? D2D1_SWEEP_DIRECTION_CLOCKWISE : D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE, segment.largeArc ? D2D1_ARC_SIZE_LARGE : D2D1_ARC_SIZE_SMALL));
        break;
      case PathSegment::Kind::Close:
        if (open) sink->EndFigure(D2D1_FIGURE_END_CLOSED);
        open = false;
        break;
    }
  }
  if (open) sink->EndFigure(D2D1_FIGURE_END_OPEN);
  return SUCCEEDED(sink->Close());
}

}  // namespace

Graphics& Graphics::shared() {
  static Graphics instance;
  return instance;
}

Graphics::Graphics() {
  // WIC 需要 COM；调用线程已用别的模式初始化时不算失败
  const HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
  if (FAILED(com) && com != RPC_E_CHANGED_MODE) throw std::runtime_error("COM 初始化失败");
  if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, d2d_.GetAddressOf()))) throw std::runtime_error("创建 Direct2D 工厂失败");
  if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(dwrite_.GetAddressOf())))) throw std::runtime_error("创建 DirectWrite 工厂失败");
  if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wic_)))) throw std::runtime_error("创建 WIC 工厂失败");
  const D2D1_STROKE_STYLE_PROPERTIES properties = D2D1::StrokeStyleProperties(D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND, D2D1_LINE_JOIN_ROUND);
  if (FAILED(d2d_->CreateStrokeStyle(properties, nullptr, 0, &roundStroke_))) throw std::runtime_error("创建描边样式失败");
  text_ = std::make_unique<TextEngine>(dwrite_.Get());
  icons_.resize(static_cast<std::size_t>(IconId::Count));
  iconBuilt_.assign(static_cast<std::size_t>(IconId::Count), false);
}

const std::vector<IconPart>& Graphics::icon(IconId id) {
  const auto index = static_cast<std::size_t>(id);
  if (iconBuilt_[index]) return icons_[index];
  iconBuilt_[index] = true;
  const IconData& data = iconData(id);
  for (std::size_t i = 0; i < data.count; ++i) {
    const IconPath& path = data.paths[i];
    const bool fill = path.fill == "currentColor";
    const bool stroke = path.stroke != "none";
    if (!fill && !stroke) continue;
    const auto segments = parseSvgPath(path.d);
    if (!segments) continue;
    IconPart part;
    if (!buildGeometry(d2d_.Get(), *segments, fill, part.geometry)) continue;
    part.fill = fill;
    part.stroke = stroke;
    part.opacity = path.opacity;
    icons_[index].push_back(std::move(part));
  }
  return icons_[index];
}

}  // namespace rujian::ui
