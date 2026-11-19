/*
 * ------------------------------------------------------------------------
 * 名称：app-logo.cpp
 * 说明：应用标志控件的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-20
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "ui/shell/app-logo.hpp"

#include <algorithm>
#include <cmath>

#include "rujian/ui/canvas.hpp"

namespace ember::win32::ui {

AppLogo::AppLogo(BitmapProvider provider) : provider_(std::move(provider)) {}

void AppLogo::paint(rujian::ui::Canvas& canvas) {
  if (!provider_) return;
  const rujian::ui::Rect box = bounds();
  const double scale = canvas.scale();
  // 对齐到设备像素后的边长：位图与区域一样大时一个像素对一个像素
  const int width = static_cast<int>(std::lround(box.right() * scale) - std::lround(box.x * scale));
  const int height = static_cast<int>(std::lround(box.bottom() * scale) - std::lround(box.y * scale));
  const int size = std::min(width, height);
  if (size <= 0) return;
  if (size != cachedSize_) {
    cachedSize_ = size;
    cached_ = provider_(size);
  }
  if (cached_.size() != static_cast<std::size_t>(size) * static_cast<std::size_t>(size)) return;
  canvas.drawBitmap(cached_.data(), size, size, box);
}

}  // namespace ember::win32::ui
