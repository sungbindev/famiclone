/*
 * ------------------------------------------------------------------------
 * 名称：graphics.hpp
 * 说明：图形资源：进程内共用的 Direct2D、DirectWrite、WIC 工厂，文字排版引擎，图标几何缓存。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：只用 Direct2D 1.0 与 DirectWrite 1.0（Windows 7 SP1 自带的版本）；单线程使用，在界面线程里创建与使用。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <memory>
#include <vector>

#include "base/text-engine.hpp"
#include "rujian/ui/icon-id.hpp"

namespace rujian::ui {

/** 图标里的一条已构建好的路径。 */
struct IconPart {
  /** 路径几何，坐标在 24×24 的画布里。 */
  Microsoft::WRL::ComPtr<ID2D1PathGeometry> geometry;
  /** 是否填充。 */
  bool fill{false};
  /** 是否描边。 */
  bool stroke{true};
  /** 不透明度。 */
  double opacity{1.0};
};

/** 图形资源。 */
class Graphics {
 public:
  /**
   * 取共用的实例，第一次调用时创建。
   * @throws std::runtime_error 系统缺少 Direct2D、DirectWrite 或 WIC 时。
   */
  static Graphics& shared();

  [[nodiscard]] ID2D1Factory* d2d() const { return d2d_.Get(); }
  [[nodiscard]] IDWriteFactory* dwrite() const { return dwrite_.Get(); }
  [[nodiscard]] IWICImagingFactory* wic() const { return wic_.Get(); }
  [[nodiscard]] TextEngine& text() { return *text_; }

  /** 圆头圆角的描边样式，图标用。 */
  [[nodiscard]] ID2D1StrokeStyle* roundStroke() const { return roundStroke_.Get(); }

  /** 取图标的几何；第一次取时解析路径并缓存。 */
  [[nodiscard]] const std::vector<IconPart>& icon(IconId id);

 private:
  Graphics();

  Microsoft::WRL::ComPtr<ID2D1Factory> d2d_;
  Microsoft::WRL::ComPtr<IDWriteFactory> dwrite_;
  Microsoft::WRL::ComPtr<IWICImagingFactory> wic_;
  Microsoft::WRL::ComPtr<ID2D1StrokeStyle> roundStroke_;
  std::unique_ptr<TextEngine> text_;
  std::vector<std::vector<IconPart>> icons_;
  std::vector<bool> iconBuilt_;
};

}  // namespace rujian::ui
