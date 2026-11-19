/*
 * ------------------------------------------------------------------------
 * 名称：window-shadow.cpp
 * 说明：窗口自绘外阴影的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：图像放在 32 位的 DIB 里，用 UpdateLayeredWindow 带逐像素透明度显示；窗口类只注册一次。
 * ------------------------------------------------------------------------
 */

#include "platform/window/window-shadow.hpp"

#include <windows.h>

#include <cstring>

#include "shared/window/window-shadow-image.hpp"

namespace ember::win32::platform {

namespace {

constexpr wchar_t kShadowClass[] = L"EmberWindowShadow";

ATOM registerShadowClass() {
  static const ATOM atom = [] {
    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = DefWindowProcW;
    windowClass.hInstance = GetModuleHandleW(nullptr);
    windowClass.lpszClassName = kShadowClass;
    return RegisterClassExW(&windowClass);
  }();
  return atom;
}

}  // namespace

struct WindowShadow::Impl {
  HWND main{nullptr};
  HWND shadow{nullptr};
  HDC memory{nullptr};
  HBITMAP bitmap{nullptr};
  HGDIOBJ previous{nullptr};
  int bitmapWidth{0};
  int bitmapHeight{0};
  int renderedWindowWidth{0};
  int renderedWindowHeight{0};
  double renderedScale{0};
  bool renderedActive{false};
  bool rendered{false};
  bool active{true};
  bool suppressed{false};
  double scale{1.0};

  ~Impl() {
    if (shadow != nullptr) DestroyWindow(shadow);
    releaseBitmap();
    if (memory != nullptr) DeleteDC(memory);
  }

  void releaseBitmap() {
    if (memory != nullptr && previous != nullptr) SelectObject(memory, previous);
    previous = nullptr;
    if (bitmap != nullptr) DeleteObject(bitmap);
    bitmap = nullptr;
    rendered = false;
  }

  bool ensureWindow() {
    if (shadow != nullptr) return true;
    if (registerShadowClass() == 0) return false;
    shadow = CreateWindowExW(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, kShadowClass, L"", WS_POPUP, 0, 0, 0, 0, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    if (shadow == nullptr) return false;
    HDC screen = GetDC(nullptr);
    memory = CreateCompatibleDC(screen);
    ReleaseDC(nullptr, screen);
    return memory != nullptr;
  }

  /** 把阴影图像画进 DIB。 */
  bool render(int windowWidth, int windowHeight) {
    const shared::WindowShadowImage image = shared::renderWindowShadow(windowWidth, windowHeight, scale, active);
    if (image.pixels.empty()) return false;
    if (bitmap == nullptr || bitmapWidth != image.width || bitmapHeight != image.height) {
      releaseBitmap();
      BITMAPINFO info{};
      info.bmiHeader.biSize = sizeof(info.bmiHeader);
      info.bmiHeader.biWidth = image.width;
      info.bmiHeader.biHeight = -image.height;
      info.bmiHeader.biPlanes = 1;
      info.bmiHeader.biBitCount = 32;
      info.bmiHeader.biCompression = BI_RGB;
      void* bits = nullptr;
      bitmap = CreateDIBSection(memory, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
      if (bitmap == nullptr || bits == nullptr) return false;
      previous = SelectObject(memory, bitmap);
      bitmapWidth = image.width;
      bitmapHeight = image.height;
      dibBits = bits;
    }
    std::memcpy(dibBits, image.pixels.data(), image.pixels.size() * sizeof(std::uint32_t));
    margin = image.margin;
    renderedWindowWidth = windowWidth;
    renderedWindowHeight = windowHeight;
    renderedScale = scale;
    renderedActive = active;
    rendered = true;
    return true;
  }

  void hide() {
    if (shadow != nullptr && IsWindowVisible(shadow) != FALSE) ShowWindow(shadow, SW_HIDE);
  }

  void sync() {
    const bool wanted = !suppressed && IsWindowVisible(main) != FALSE && IsIconic(main) == FALSE && IsZoomed(main) == FALSE;
    if (!wanted) {
      hide();
      return;
    }
    RECT rect{};
    if (GetWindowRect(main, &rect) == FALSE || !ensureWindow()) return;
    const int windowWidth = rect.right - rect.left;
    const int windowHeight = rect.bottom - rect.top;
    if (windowWidth <= 0 || windowHeight <= 0) return;
    if (!rendered || renderedWindowWidth != windowWidth || renderedWindowHeight != windowHeight || renderedScale != scale || renderedActive != active) {
      if (!render(windowWidth, windowHeight)) return;
    }
    POINT position{rect.left - margin, rect.top - margin};
    SIZE size{bitmapWidth, bitmapHeight};
    POINT source{0, 0};
    BLENDFUNCTION blend{AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
    UpdateLayeredWindow(shadow, nullptr, &position, &size, memory, &source, 0, &blend, ULW_ALPHA);
    // 放到主窗口的正后面；主窗口置顶时这样放，阴影也在置顶的那一层
    SetWindowPos(shadow, main, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
  }

  void* dibBits{nullptr};
  int margin{0};
};

WindowShadow::WindowShadow(void* window) : impl_(new Impl) { impl_->main = static_cast<HWND>(window); }

WindowShadow::~WindowShadow() { delete impl_; }

void WindowShadow::sync() { impl_->sync(); }

void WindowShadow::setActive(bool active) {
  if (impl_->active == active) return;
  impl_->active = active;
  impl_->sync();
}

void WindowShadow::setSuppressed(bool suppressed) {
  if (impl_->suppressed == suppressed) return;
  impl_->suppressed = suppressed;
  impl_->sync();
}

void WindowShadow::setScale(double scale) {
  if (impl_->scale == scale) return;
  impl_->scale = scale;
  impl_->sync();
}

}  // namespace ember::win32::platform
