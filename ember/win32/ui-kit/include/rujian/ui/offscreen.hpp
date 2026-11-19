/*
 * ------------------------------------------------------------------------
 * 名称：offscreen.hpp
 * 说明：离屏绘制：把一棵控件树画到内存位图上，可取像素或存成 PNG，用于自动测试与和网页版截图对照。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：位图的像素大小 = 逻辑大小 × 缩放比例（四舍五入）；像素格式为预乘透明度的 BGRA，每行紧密排列。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>

#include "rujian/ui/geometry.hpp"
#include "rujian/ui/root.hpp"

namespace rujian::ui {

/** 离屏绘制面。 */
class OffscreenSurface {
 public:
  /**
   * @param size 逻辑大小。
   * @param scale 缩放比例，必须大于 0。
   * @throws std::runtime_error 创建位图或渲染目标失败时。
   */
  OffscreenSurface(Size size, double scale);
  ~OffscreenSurface();
  OffscreenSurface(const OffscreenSurface&) = delete;
  OffscreenSurface& operator=(const OffscreenSurface&) = delete;

  /**
   * 先用背景色铺满，再绘制整棵控件树。调用前要已对 Root 做过布局。
   * @param root 根容器；它的缩放比例必须与面一致。
   * @param background 背景色。
   * @return 绘制成功返回 true。
   */
  bool render(Root& root, Color background);

  /** 像素宽度。 */
  [[nodiscard]] int pixelWidth() const;
  /** 像素高度。 */
  [[nodiscard]] int pixelHeight() const;
  /** 读出全部像素（预乘透明度的 BGRA）。 */
  [[nodiscard]] std::vector<std::uint8_t> pixels() const;
  /** 存成 PNG 文件，成功返回 true。 */
  bool savePng(const std::filesystem::path& path) const;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace rujian::ui
