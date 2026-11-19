/*
 * ------------------------------------------------------------------------
 * 名称：app-icon.cpp
 * 说明：应用图标像素的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-20
 * 备注：图标的彩色位图带透明度通道（32 位），用 GetDIBits 取出后再预乘；不用 DrawIconEx 画到位图上，因为它不保证写出透明度通道。
 * ------------------------------------------------------------------------
 */

#include "platform/window/app-icon.hpp"

#include <windows.h>

namespace ember::win32::platform {

namespace {

/** 可执行文件里应用图标的资源编号（ember-win.rc.in 里的 `1 ICON`）。 */
constexpr int kAppIconResource = 1;

/** 把图标的彩色位图读成自上而下的 32 位 BGRA；失败时返回空。 */
std::vector<std::uint32_t> readColorBitmap(HBITMAP color, int size) {
  BITMAPINFO info{};
  info.bmiHeader.biSize = sizeof(info.bmiHeader);
  info.bmiHeader.biWidth = size;
  info.bmiHeader.biHeight = -size;
  info.bmiHeader.biPlanes = 1;
  info.bmiHeader.biBitCount = 32;
  info.bmiHeader.biCompression = BI_RGB;
  std::vector<std::uint32_t> pixels(static_cast<std::size_t>(size) * static_cast<std::size_t>(size));
  HDC screen = GetDC(nullptr);
  const int rows = GetDIBits(screen, color, 0, static_cast<UINT>(size), pixels.data(), &info, DIB_RGB_COLORS);
  ReleaseDC(nullptr, screen);
  if (rows != size) pixels.clear();
  return pixels;
}

/** 非预乘的 BGRA 转成预乘。 */
void premultiply(std::vector<std::uint32_t>& pixels) {
  for (std::uint32_t& pixel : pixels) {
    const std::uint32_t alpha = pixel >> 24;
    const std::uint32_t red = (((pixel >> 16) & 0xFF) * alpha + 127) / 255;
    const std::uint32_t green = (((pixel >> 8) & 0xFF) * alpha + 127) / 255;
    const std::uint32_t blue = ((pixel & 0xFF) * alpha + 127) / 255;
    pixel = (alpha << 24) | (red << 16) | (green << 8) | blue;
  }
}

}  // namespace

std::vector<std::uint32_t> renderAppIcon(int pixelSize) {
  if (pixelSize <= 0) return {};
  HICON icon = static_cast<HICON>(LoadImageW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(kAppIconResource), IMAGE_ICON, pixelSize, pixelSize, LR_DEFAULTCOLOR));
  if (icon == nullptr) return {};
  std::vector<std::uint32_t> pixels;
  ICONINFO info{};
  if (GetIconInfo(icon, &info) != FALSE) {
    BITMAP bitmap{};
    if (info.hbmColor != nullptr && GetObjectW(info.hbmColor, sizeof(bitmap), &bitmap) != 0 && bitmap.bmWidth == pixelSize && bitmap.bmHeight == pixelSize && bitmap.bmBitsPixel == 32) {
      pixels = readColorBitmap(info.hbmColor, pixelSize);
      premultiply(pixels);
    }
    if (info.hbmColor != nullptr) DeleteObject(info.hbmColor);
    if (info.hbmMask != nullptr) DeleteObject(info.hbmMask);
  }
  DestroyIcon(icon);
  return pixels;
}

}  // namespace ember::win32::platform
