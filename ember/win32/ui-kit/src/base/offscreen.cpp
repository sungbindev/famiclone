/*
 * ------------------------------------------------------------------------
 * 名称：offscreen.cpp
 * 说明：离屏绘制的实现：WIC 位图加 Direct2D 的 WIC 渲染目标。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：渲染目标的文字抗锯齿固定为灰度，保证不同机器上截图一致。
 * ------------------------------------------------------------------------
 */

#include "rujian/ui/offscreen.hpp"

#include <cmath>
#include <stdexcept>

#include "base/d2d-canvas.hpp"
#include "base/graphics.hpp"

namespace rujian::ui {

struct OffscreenSurface::Impl {
  Microsoft::WRL::ComPtr<IWICBitmap> bitmap;
  Microsoft::WRL::ComPtr<ID2D1RenderTarget> target;
  double scale{1.0};
  UINT width{0};
  UINT height{0};
};

OffscreenSurface::OffscreenSurface(Size size, double scale) : impl_(std::make_unique<Impl>()) {
  Graphics& graphics = Graphics::shared();
  impl_->scale = scale;
  impl_->width = static_cast<UINT>(std::lround(size.width * scale));
  impl_->height = static_cast<UINT>(std::lround(size.height * scale));
  if (impl_->width == 0 || impl_->height == 0) throw std::runtime_error("离屏位图的大小不能为 0");
  if (FAILED(graphics.wic()->CreateBitmap(impl_->width, impl_->height, GUID_WICPixelFormat32bppPBGRA, WICBitmapCacheOnLoad, &impl_->bitmap))) throw std::runtime_error("创建离屏位图失败");
  const D2D1_RENDER_TARGET_PROPERTIES properties =
      D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT, D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED), static_cast<FLOAT>(96.0 * scale), static_cast<FLOAT>(96.0 * scale));
  if (FAILED(graphics.d2d()->CreateWicBitmapRenderTarget(impl_->bitmap.Get(), properties, &impl_->target))) throw std::runtime_error("创建离屏渲染目标失败");
  impl_->target->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
}

OffscreenSurface::~OffscreenSurface() = default;

bool OffscreenSurface::render(Root& root, Color background) {
  Graphics& graphics = Graphics::shared();
  impl_->target->BeginDraw();
  {
    D2DCanvas canvas(impl_->target.Get(), impl_->scale, graphics);
    canvas.clear(background);
    root.paint(canvas);
  }
  return SUCCEEDED(impl_->target->EndDraw());
}

int OffscreenSurface::pixelWidth() const { return static_cast<int>(impl_->width); }

int OffscreenSurface::pixelHeight() const { return static_cast<int>(impl_->height); }

std::vector<std::uint8_t> OffscreenSurface::pixels() const {
  std::vector<std::uint8_t> result;
  const WICRect rect{0, 0, static_cast<INT>(impl_->width), static_cast<INT>(impl_->height)};
  const UINT stride = impl_->width * 4;
  result.resize(static_cast<std::size_t>(stride) * impl_->height);
  if (FAILED(impl_->bitmap->CopyPixels(&rect, stride, static_cast<UINT>(result.size()), result.data()))) result.clear();
  return result;
}

bool OffscreenSurface::savePng(const std::filesystem::path& path) const {
  Graphics& graphics = Graphics::shared();
  Microsoft::WRL::ComPtr<IWICStream> stream;
  Microsoft::WRL::ComPtr<IWICBitmapEncoder> encoder;
  Microsoft::WRL::ComPtr<IWICBitmapFrameEncode> frame;
  Microsoft::WRL::ComPtr<IPropertyBag2> properties;
  Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
  if (FAILED(graphics.wic()->CreateStream(&stream)) || FAILED(stream->InitializeFromFilename(path.c_str(), GENERIC_WRITE))) return false;
  if (FAILED(graphics.wic()->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder)) || FAILED(encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache))) return false;
  if (FAILED(encoder->CreateNewFrame(&frame, &properties)) || FAILED(frame->Initialize(properties.Get()))) return false;
  if (FAILED(frame->SetSize(impl_->width, impl_->height))) return false;
  WICPixelFormatGUID format = GUID_WICPixelFormat32bppBGRA;
  if (FAILED(frame->SetPixelFormat(&format))) return false;
  if (FAILED(graphics.wic()->CreateFormatConverter(&converter)) ||
      FAILED(converter->Initialize(impl_->bitmap.Get(), GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom))) return false;
  if (FAILED(frame->WriteSource(converter.Get(), nullptr)) || FAILED(frame->Commit()) || FAILED(encoder->Commit())) return false;
  return true;
}

}  // namespace rujian::ui
