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

#include "platform/window/window-shadow.hpp"

namespace ember::win32::platform {

namespace {

using DwmExtendFrameFn = HRESULT(WINAPI*)(HWND, const MARGINS*);
using DwmSetWindowAttributeFn = HRESULT(WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
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

/** DwmSetWindowAttribute 的属性编号：非客户区绘制策略与边框颜色（后者 Windows 11 才有）。 */
constexpr DWORD kDwmNonClientRenderingPolicy = 2;
constexpr DWORD kDwmBorderColor = 34;
/** 非客户区绘制策略的取值：关闭。 */
constexpr DWORD kDwmNonClientRenderingDisabled = 1;

void setDwmAttribute(HWND hwnd, DWORD attribute, DWORD value) {
  static const auto set = lookup<DwmSetWindowAttributeFn>(dwmModule(), "DwmSetWindowAttribute");
  if (set != nullptr) set(hwnd, attribute, &value, sizeof(value));
}

/** 关掉 DWM 对非客户区的绘制：没有标题栏、按钮与玻璃边框，Windows 7 上也不会在激活、移动时把系统框架画进窗口。 */
void disableNonClientRendering(HWND hwnd) { setDwmAttribute(hwnd, kDwmNonClientRenderingPolicy, kDwmNonClientRenderingDisabled); }

}  // namespace

FramelessFrame::FramelessFrame(void* window, FramelessOptions options, FramelessCallbacks callbacks) : window_(window), options_(std::move(options)), callbacks_(std::move(callbacks)) {
  lastMaximized_ = IsZoomed(static_cast<HWND>(window_)) != FALSE;
}

FramelessFrame::~FramelessFrame() = default;

void FramelessFrame::attach() {
  HWND hwnd = static_cast<HWND>(window_);
  // 去掉标题栏样式（保留缩放边框与系统按钮样式）：带标题栏样式的窗口，系统会把一圈缩放边框的经典样式（浅蓝底、白线）画在窗口内容之上，
  // 子窗口（运行画面）盖住的地方露出这圈边框；没有标题栏样式就没有这圈边框，缩放、贴靠、系统菜单与最小化、最大化命令不受影响
  SetWindowLongPtrW(hwnd, GWL_STYLE, GetWindowLongPtrW(hwnd, GWL_STYLE) & ~static_cast<LONG_PTR>(WS_CAPTION));
  if (options_.customFrame) {
    disableNonClientRendering(hwnd);
    shadow_ = std::make_unique<WindowShadow>(window_);
    shadow_->setScale(options_.scale ? options_.scale() : 1.0);
  } else {
    extendFrame(hwnd);
  }
  // 窗口创建时系统已经算过一次非客户区，让它按现在的处理重算
  SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void FramelessFrame::setFullScreen(bool fullScreen) {
  fullScreen_ = fullScreen;
  if (shadow_) shadow_->setSuppressed(fullScreen);
}

void FramelessFrame::setBorderColor(unsigned colorRef) {
  if (!options_.customFrame) setDwmAttribute(static_cast<HWND>(window_), kDwmBorderColor, colorRef);
}

bool FramelessFrame::compositionEnabled() {
  static const auto isEnabled = lookup<DwmIsCompositionEnabledFn>(dwmModule(), "DwmIsCompositionEnabled");
  BOOL enabled = FALSE;
  return isEnabled != nullptr && SUCCEEDED(isEnabled(&enabled)) && enabled != FALSE;
}

bool FramelessFrame::prefersCustomFrame() {
  // Windows 11 的第一个版本号是 22000，从它起系统才有圆角、边框色，系统的阴影与边框才和界面搭配
  using RtlGetVersionFn = LONG(WINAPI*)(OSVERSIONINFOW*);
  static const auto getVersion = lookup<RtlGetVersionFn>(GetModuleHandleW(L"ntdll.dll"), "RtlGetVersion");
  OSVERSIONINFOW info{};
  info.dwOSVersionInfoSize = sizeof(info);
  if (getVersion == nullptr || getVersion(&info) != 0) return true;
  return info.dwBuildNumber < 22000;
}

void FramelessFrame::setSystemButtonState(FrameHit hover, FrameHit pressed) {
  if (hover == hover_ && pressed == pressed_) return;
  hover_ = hover;
  pressed_ = pressed;
  if (callbacks_.systemButtonState) callbacks_.systemButtonState(hover_, pressed_);
}

void FramelessFrame::beginButtonTracking(FrameHit button) {
  tracked_ = button;
  setSystemButtonState(button, button);
  // 捕获鼠标：之后的移动与抬起都作为客户区消息发回来
  SetCapture(static_cast<HWND>(window_));
}

void FramelessFrame::updateButtonTracking(int clientX, int clientY) {
  HWND hwnd = static_cast<HWND>(window_);
  POINT point{clientX, clientY};
  ClientToScreen(hwnd, &point);
  const WPARAM code = hitTest(point.x, point.y);
  const bool over = isSystemButton(code) && toFrameHit(code) == tracked_;
  setSystemButtonState(over ? tracked_ : FrameHit::Client, tracked_);
}

void FramelessFrame::endButtonTracking(int clientX, int clientY, bool commit) {
  HWND hwnd = static_cast<HWND>(window_);
  const FrameHit button = tracked_;
  tracked_ = FrameHit::Client;
  if (GetCapture() == hwnd) ReleaseCapture();
  setSystemButtonState(FrameHit::Client, FrameHit::Client);
  if (!commit) return;
  POINT point{clientX, clientY};
  ClientToScreen(hwnd, &point);
  const WPARAM code = hitTest(point.x, point.y);
  if (!isSystemButton(code) || toFrameHit(code) != button) return;
  // 松开时还在同一个按钮上才算点击；最大化按钮在最大化时是还原
  WPARAM command = SC_CLOSE;
  if (button == FrameHit::Minimize) command = SC_MINIMIZE;
  else if (button == FrameHit::Maximize) command = IsZoomed(hwnd) != FALSE ? SC_RESTORE : SC_MAXIMIZE;
  PostMessageW(hwnd, WM_SYSCOMMAND, command, 0);
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
        // 整个窗口都是客户区；最大化时窗口比显示器大出一圈缩放边框（大小因系统而异），客户区直接取显示器的工作区，不多不少
        if (IsZoomed(hwnd) != FALSE) {
          auto* params = reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);
          MONITORINFO monitor{sizeof(monitor)};
          if (GetMonitorInfoW(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &monitor) != FALSE) {
            params->rgrc[0] = monitor.rcWork;
          } else {
            const int horizontal = frameThickness(hwnd, true);
            const int vertical = frameThickness(hwnd, false);
            params->rgrc[0].left += horizontal;
            params->rgrc[0].right -= horizontal;
            params->rgrc[0].top += vertical;
            params->rgrc[0].bottom -= vertical;
          }
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
      if (shadow_) shadow_->setActive(wParam != FALSE);
      if (callbacks_.activeChanged) callbacks_.activeChanged(wParam != FALSE);
      // 激活状态变了，整个客户区按新状态重画（标题栏、菜单栏的颜色都跟着变）
      InvalidateRect(hwnd, nullptr, FALSE);
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
      if (shadow_) {
        shadow_->setScale(options_.scale ? options_.scale() : 1.0);
        shadow_->sync();
      }
      return false;
    }
    case WM_NCDESTROY:
      shadow_.reset();
      return false;
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
      if (callbacks_.nonClientPressed) callbacks_.nonClientPressed();
      if (!isSystemButton(wParam)) return false;
      // 系统自己跟踪按钮需要标题栏样式，而标题栏样式已经去掉；按下后由本类捕获鼠标、跟踪悬停与按下，松开时发出系统命令
      beginButtonTracking(toFrameHit(wParam));
      result = 0;
      return true;
    case WM_NCRBUTTONDOWN:
    case WM_NCMBUTTONDOWN:
      if (callbacks_.nonClientPressed) callbacks_.nonClientPressed();
      return false;
    case WM_MOUSEMOVE:
      if (tracked_ == FrameHit::Client) return false;
      updateButtonTracking(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
      result = 0;
      return true;
    case WM_LBUTTONUP:
      if (tracked_ == FrameHit::Client) return false;
      endButtonTracking(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), true);
      result = 0;
      return true;
    case WM_CAPTURECHANGED:
    case WM_CANCELMODE:
      // 跟踪中被别的窗口抢走了鼠标：放弃这次点击
      if (tracked_ != FrameHit::Client && reinterpret_cast<HWND>(lParam) != hwnd) endButtonTracking(0, 0, false);
      return false;
    case WM_SETTINGCHANGE:
      if (lParam != 0 && callbacks_.systemThemeChanged && lstrcmpW(reinterpret_cast<LPCWSTR>(lParam), L"ImmersiveColorSet") == 0) callbacks_.systemThemeChanged();
      return false;
    case WM_DWMCOMPOSITIONCHANGED:
      if (options_.customFrame) disableNonClientRendering(hwnd);
      else extendFrame(hwnd);
      if (callbacks_.compositionChanged) callbacks_.compositionChanged(compositionEnabled());
      return false;
    default:
      return false;
  }
}

}  // namespace ember::win32::platform
