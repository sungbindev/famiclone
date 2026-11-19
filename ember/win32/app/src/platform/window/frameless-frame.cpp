/*
 * ------------------------------------------------------------------------
 * 名称：frameless-frame.cpp
 * 说明：自绘标题栏窗口的非客户区行为的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：dwmapi 与按 DPI 取系统尺寸的函数都动态查找，不进入导入表。
 * ------------------------------------------------------------------------
 */

#include "platform/window/frameless-frame.hpp"

#include <windows.h>
#include <windowsx.h>
#include <uxtheme.h>

namespace ember::win32::platform {

namespace {

using DwmExtendFrameFn = HRESULT(WINAPI*)(HWND, const MARGINS*);
using DwmIsCompositionEnabledFn = HRESULT(WINAPI*)(BOOL*);
using GetDpiForWindowFn = UINT(WINAPI*)(HWND);
using GetSystemMetricsForDpiFn = int(WINAPI*)(int, UINT);

HMODULE dwmModule() {
  static const HMODULE module = LoadLibraryW(L"dwmapi.dll");
  return module;
}

template <typename Function>
Function lookup(HMODULE module, const char* name) {
  return module == nullptr ? nullptr : reinterpret_cast<Function>(reinterpret_cast<void*>(GetProcAddress(module, name)));
}

/** 窗口的 DPI：Windows 10 1607 起按窗口所在的显示器，否则用系统 DPI。 */
UINT windowDpi(HWND hwnd) {
  static const auto getDpiForWindow = lookup<GetDpiForWindowFn>(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow");
  if (getDpiForWindow != nullptr) return getDpiForWindow(hwnd);
  HDC dc = GetDC(nullptr);
  const int dpi = GetDeviceCaps(dc, LOGPIXELSX);
  ReleaseDC(nullptr, dc);
  return dpi > 0 ? static_cast<UINT>(dpi) : 96u;
}

/** 系统尺寸：有按 DPI 取的函数时按窗口的 DPI，否则用系统的取值。 */
int metric(HWND hwnd, int index) {
  static const auto forDpi = lookup<GetSystemMetricsForDpiFn>(GetModuleHandleW(L"user32.dll"), "GetSystemMetricsForDpi");
  return forDpi != nullptr ? forDpi(index, windowDpi(hwnd)) : GetSystemMetrics(index);
}

/** 缩放边框的厚度：水平方向用 SM_CXSIZEFRAME，垂直方向用 SM_CYSIZEFRAME，再加上补充边框。 */
int frameThickness(HWND hwnd, bool horizontal) { return metric(hwnd, horizontal ? SM_CXSIZEFRAME : SM_CYSIZEFRAME) + metric(hwnd, SM_CXPADDEDBORDER); }

bool isSystemButton(WPARAM hit) { return hit == HTMINBUTTON || hit == HTMAXBUTTON || hit == HTCLOSE; }

FrameHit toFrameHit(WPARAM hit) {
  switch (hit) {
    case HTMINBUTTON:
      return FrameHit::Minimize;
    case HTMAXBUTTON:
      return FrameHit::Maximize;
    case HTCLOSE:
      return FrameHit::Close;
    default:
      return FrameHit::Client;
  }
}

/** 让 DWM 把窗口框架（阴影）延伸进客户区一个像素。 */
void extendFrame(HWND hwnd) {
  static const auto extend = lookup<DwmExtendFrameFn>(dwmModule(), "DwmExtendFrameIntoClientArea");
  if (extend == nullptr) return;
  const MARGINS margins{1, 1, 1, 1};
  extend(hwnd, &margins);
}

}  // namespace

FramelessFrame::FramelessFrame(void* window, FramelessOptions options, FramelessCallbacks callbacks) : window_(window), options_(std::move(options)), callbacks_(std::move(callbacks)) {
  lastMaximized_ = IsZoomed(static_cast<HWND>(window_)) != FALSE;
}

void FramelessFrame::attach() {
  HWND hwnd = static_cast<HWND>(window_);
  extendFrame(hwnd);
  // 窗口创建时系统已经算过一次非客户区，让它按现在的处理重算
  SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

bool FramelessFrame::compositionEnabled() {
  static const auto isEnabled = lookup<DwmIsCompositionEnabledFn>(dwmModule(), "DwmIsCompositionEnabled");
  BOOL enabled = FALSE;
  return isEnabled != nullptr && SUCCEEDED(isEnabled(&enabled)) && enabled != FALSE;
}

void FramelessFrame::setSystemButtonState(FrameHit hover, FrameHit pressed) {
  if (hover == hover_ && pressed == pressed_) return;
  hover_ = hover;
  pressed_ = pressed;
  if (callbacks_.systemButtonState) callbacks_.systemButtonState(hover_, pressed_);
}

unsigned long FramelessFrame::hitTest(int screenX, int screenY) const {
  HWND hwnd = static_cast<HWND>(window_);
  RECT rect{};
  GetWindowRect(hwnd, &rect);
  if (IsZoomed(hwnd) == FALSE && !fullScreen_) {
    const int horizontal = frameThickness(hwnd, true);
    // 顶边只留系统的缩放边框厚度，不加补充边框，免得压住右上角的关闭按钮
    const int top = metric(hwnd, SM_CYSIZEFRAME);
    const bool left = screenX < rect.left + horizontal;
    const bool right = screenX >= rect.right - horizontal;
    const bool above = screenY < rect.top + top;
    const bool below = screenY >= rect.bottom - horizontal;
    if (above && left) return HTTOPLEFT;
    if (above && right) return HTTOPRIGHT;
    if (below && left) return HTBOTTOMLEFT;
    if (below && right) return HTBOTTOMRIGHT;
    if (left) return HTLEFT;
    if (right) return HTRIGHT;
    if (above) return HTTOP;
    if (below) return HTBOTTOM;
  }
  if (!callbacks_.hitTest || fullScreen_) return HTCLIENT;
  const double scale = options_.scale ? options_.scale() : 1.0;
  switch (callbacks_.hitTest((screenX - rect.left) / scale, (screenY - rect.top) / scale)) {
    case FrameHit::Caption:
      return HTCAPTION;
    case FrameHit::Minimize:
      return HTMINBUTTON;
    case FrameHit::Maximize:
      return HTMAXBUTTON;
    case FrameHit::Close:
      return HTCLOSE;
    case FrameHit::Client:
      break;
  }
  return HTCLIENT;
}

bool FramelessFrame::handle(unsigned message, std::uintptr_t wParam, std::intptr_t lParam, std::intptr_t& result) {
  HWND hwnd = static_cast<HWND>(window_);
  switch (message) {
    case WM_NCCALCSIZE:
      if (wParam == TRUE) {
        // 整个窗口都是客户区；最大化时窗口比显示器大出一圈缩放边框，要把它去掉
        if (IsZoomed(hwnd) != FALSE) {
          auto* params = reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);
          const int horizontal = frameThickness(hwnd, true);
          const int vertical = frameThickness(hwnd, false);
          params->rgrc[0].left += horizontal;
          params->rgrc[0].right -= horizontal;
          params->rgrc[0].top += vertical;
          params->rgrc[0].bottom -= vertical;
        }
        result = 0;
        return true;
      }
      return false;
    case WM_NCHITTEST:
      result = static_cast<std::intptr_t>(hitTest(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)));
      return true;
    case WM_NCACTIVATE:
      // lParam 传 -1：不重绘非客户区（没有非客户区）
      result = static_cast<std::intptr_t>(DefWindowProcW(hwnd, message, wParam, -1));
      if (callbacks_.activeChanged) callbacks_.activeChanged(wParam != FALSE);
      return true;
    case WM_NCPAINT:
      result = 0;
      return true;
    case WM_GETMINMAXINFO: {
      auto* info = reinterpret_cast<MINMAXINFO*>(lParam);
      const double scale = options_.scale ? options_.scale() : 1.0;
      info->ptMinTrackSize.x = static_cast<LONG>(options_.minimumWidth * scale + 0.5);
      info->ptMinTrackSize.y = static_cast<LONG>(options_.minimumHeight * scale + 0.5);
      result = 0;
      return true;
    }
    case WM_SIZE: {
      if (wParam == SIZE_MAXIMIZED || wParam == SIZE_RESTORED) {
        const bool maximized = wParam == SIZE_MAXIMIZED;
        if (maximized != lastMaximized_) {
          lastMaximized_ = maximized;
          if (callbacks_.maximizedChanged) callbacks_.maximizedChanged(maximized);
        }
      }
      return false;
    }
    case WM_WINDOWPOSCHANGED: {
      const auto* position = reinterpret_cast<const WINDOWPOS*>(lParam);
      if ((position->flags & SWP_NOMOVE) == 0 && IsZoomed(hwnd) == FALSE && IsIconic(hwnd) == FALSE && callbacks_.moved) callbacks_.moved(position->x, position->y);
      return false;
    }
    case WM_NCMOUSEMOVE:
      if (!trackingLeave_) {
        TRACKMOUSEEVENT track{sizeof(track), TME_LEAVE | TME_NONCLIENT, hwnd, 0};
        trackingLeave_ = TrackMouseEvent(&track) != FALSE;
      }
      setSystemButtonState(isSystemButton(wParam) ? toFrameHit(wParam) : FrameHit::Client, pressed_);
      return false;
    case WM_NCMOUSELEAVE:
      trackingLeave_ = false;
      setSystemButtonState(FrameHit::Client, pressed_);
      return false;
    case WM_NCLBUTTONDOWN:
      if (!isSystemButton(wParam)) return false;
      setSystemButtonState(toFrameHit(wParam), toFrameHit(wParam));
      // 按下后系统自己跟踪鼠标直到松开（点击、悬停提示与贴靠布局都由系统处理）
      result = static_cast<std::intptr_t>(DefWindowProcW(hwnd, message, wParam, lParam));
      setSystemButtonState(FrameHit::Client, FrameHit::Client);
      return true;
    case WM_SETTINGCHANGE:
      if (lParam != 0 && callbacks_.systemThemeChanged && lstrcmpW(reinterpret_cast<LPCWSTR>(lParam), L"ImmersiveColorSet") == 0) callbacks_.systemThemeChanged();
      return false;
    case WM_DWMCOMPOSITIONCHANGED:
      extendFrame(hwnd);
      if (callbacks_.compositionChanged) callbacks_.compositionChanged(compositionEnabled());
      return false;
    default:
      return false;
  }
}

}  // namespace ember::win32::platform
