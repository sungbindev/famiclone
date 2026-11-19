/*
 * ------------------------------------------------------------------------
 * 名称：window.cpp
 * 说明：窗口宿主的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：Windows 7 没有 GetDpiForWindow，按“动态查找该函数，找不到就用系统 DPI”处理。
 * ------------------------------------------------------------------------
 */

#include "rujian/ui/window.hpp"

#include <windows.h>
#include <windowsx.h>

#include <cmath>
#include <stdexcept>

#include "base/d2d-canvas.hpp"
#include "base/graphics.hpp"
#include "base/theme-tokens.hpp"

namespace rujian::ui {

namespace {

constexpr wchar_t kWindowClass[] = L"RujianUiHostWindow";
constexpr double kBaseDpi = 96.0;

/** 取窗口的 DPI：优先用 GetDpiForWindow（Windows 10 1607 起），否则用系统 DPI。 */
UINT windowDpi(HWND hwnd) {
  using GetDpiForWindowFn = UINT(WINAPI*)(HWND);
  static const auto getDpiForWindow = reinterpret_cast<GetDpiForWindowFn>(reinterpret_cast<void*>(GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow")));
  if (getDpiForWindow != nullptr) return getDpiForWindow(hwnd);
  HDC dc = GetDC(nullptr);
  const int dpi = GetDeviceCaps(dc, LOGPIXELSX);
  ReleaseDC(nullptr, dc);
  return dpi > 0 ? static_cast<UINT>(dpi) : 96u;
}

unsigned currentModifiers() {
  unsigned modifiers = 0;
  if ((GetKeyState(VK_SHIFT) & 0x8000) != 0) modifiers |= kModifierShift;
  if ((GetKeyState(VK_CONTROL) & 0x8000) != 0) modifiers |= kModifierCtrl;
  if ((GetKeyState(VK_MENU) & 0x8000) != 0) modifiers |= kModifierAlt;
  return modifiers;
}

}  // namespace

struct HostWindow::Impl {
  HWND hwnd{nullptr};
  Root root;
  Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget> target;
  bool trackingLeave{false};
  wchar_t highSurrogate{0};

  explicit Impl(ThemeName theme) : root(theme, 1.0) {}

  [[nodiscard]] double scale() const { return root.scale(); }

  [[nodiscard]] Point toLogical(LPARAM lParam) const { return Point{GET_X_LPARAM(lParam) / scale(), GET_Y_LPARAM(lParam) / scale()}; }

  [[nodiscard]] Size clientLogicalSize() const {
    RECT rect{};
    GetClientRect(hwnd, &rect);
    return Size{(rect.right - rect.left) / scale(), (rect.bottom - rect.top) / scale()};
  }

  /** 按当前 DPI 更新缩放比例并重新布局。 */
  void applyDpi() {
    root.setScale(windowDpi(hwnd) / kBaseDpi);
    target.Reset();
    root.layout(clientLogicalSize());
  }

  void ensureTarget() {
    if (target) return;
    RECT rect{};
    GetClientRect(hwnd, &rect);
    const auto dpi = static_cast<FLOAT>(kBaseDpi * scale());
    const D2D1_RENDER_TARGET_PROPERTIES properties = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT, D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_IGNORE), dpi, dpi);
    const D2D1_HWND_RENDER_TARGET_PROPERTIES hwndProperties = D2D1::HwndRenderTargetProperties(hwnd, D2D1::SizeU(static_cast<UINT>(rect.right - rect.left), static_cast<UINT>(rect.bottom - rect.top)));
    Graphics::shared().d2d()->CreateHwndRenderTarget(properties, hwndProperties, &target);
  }

  void paint() {
    PAINTSTRUCT ps;
    BeginPaint(hwnd, &ps);
    ensureTarget();
    if (target) {
      target->BeginDraw();
      {
        D2DCanvas canvas(target.Get(), scale(), Graphics::shared());
        canvas.clear(themeTokens(root.theme()).content_bg);
        root.paint(canvas);
      }
      if (target->EndDraw() == D2DERR_RECREATE_TARGET) target.Reset();
    }
    EndPaint(hwnd, &ps);
  }

  LRESULT handle(UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
      case WM_ERASEBKGND:
        return 1;
      case WM_PAINT:
        paint();
        return 0;
      case WM_SIZE:
        if (wParam != SIZE_MINIMIZED) {
          if (target) target->Resize(D2D1::SizeU(LOWORD(lParam), HIWORD(lParam)));
          root.layout(clientLogicalSize());
        }
        return 0;
      case WM_DPICHANGED: {
        const RECT* suggested = reinterpret_cast<const RECT*>(lParam);
        SetWindowPos(hwnd, nullptr, suggested->left, suggested->top, suggested->right - suggested->left, suggested->bottom - suggested->top, SWP_NOZORDER | SWP_NOACTIVATE);
        applyDpi();
        return 0;
      }
      case WM_MOUSEMOVE:
        if (!trackingLeave) {
          TRACKMOUSEEVENT track{sizeof(track), TME_LEAVE, hwnd, 0};
          TrackMouseEvent(&track);
          trackingLeave = true;
        }
        root.mouseMove(toLogical(lParam), currentModifiers());
        return 0;
      case WM_MOUSELEAVE:
        trackingLeave = false;
        root.mouseLeave();
        return 0;
      case WM_LBUTTONDOWN:
      case WM_RBUTTONDOWN:
      case WM_MBUTTONDOWN:
        SetCapture(hwnd);
        root.mouseDown(toLogical(lParam), message == WM_LBUTTONDOWN ? MouseButton::Left : (message == WM_RBUTTONDOWN ? MouseButton::Right : MouseButton::Middle), currentModifiers());
        return 0;
      case WM_LBUTTONUP:
      case WM_RBUTTONUP:
      case WM_MBUTTONUP:
        root.mouseUp(toLogical(lParam), message == WM_LBUTTONUP ? MouseButton::Left : (message == WM_RBUTTONUP ? MouseButton::Right : MouseButton::Middle), currentModifiers());
        ReleaseCapture();
        return 0;
      case WM_MOUSEWHEEL: {
        POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        ScreenToClient(hwnd, &point);
        root.mouseWheel(Point{point.x / scale(), point.y / scale()}, GET_WHEEL_DELTA_WPARAM(wParam) / static_cast<double>(WHEEL_DELTA), currentModifiers());
        return 0;
      }
      case WM_KEYDOWN:
        root.keyDown(static_cast<int>(wParam), currentModifiers(), (lParam & 0x40000000) != 0);
        break;
      case WM_KEYUP:
        root.keyUp(static_cast<int>(wParam), currentModifiers());
        break;
      case WM_CHAR: {
        const auto unit = static_cast<wchar_t>(wParam);
        if (unit >= 0xD800 && unit <= 0xDBFF) {
          highSurrogate = unit;
        } else if (unit >= 0xDC00 && unit <= 0xDFFF && highSurrogate != 0) {
          root.character(0x10000 + ((static_cast<char32_t>(highSurrogate) - 0xD800) << 10) + (static_cast<char32_t>(unit) - 0xDC00));
          highSurrogate = 0;
        } else {
          highSurrogate = 0;
          root.character(unit);
        }
        return 0;
      }
      case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
      default:
        break;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
  }

  static LRESULT CALLBACK procedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_NCCREATE) {
      const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lParam);
      auto* self = static_cast<Impl*>(create->lpCreateParams);
      self->hwnd = hwnd;
      SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    auto* self = reinterpret_cast<Impl*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    return self != nullptr ? self->handle(message, wParam, lParam) : DefWindowProcW(hwnd, message, wParam, lParam);
  }
};

HostWindow::HostWindow(const Options& options) : impl_(std::make_unique<Impl>(options.theme)) {
  Graphics::shared();
  static const ATOM registered = [] {
    WNDCLASSEXW windowClass{sizeof(windowClass)};
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = &Impl::procedure;
    windowClass.hInstance = GetModuleHandleW(nullptr);
    windowClass.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));  // 32512 即 IDC_ARROW
    windowClass.lpszClassName = kWindowClass;
    return RegisterClassExW(&windowClass);
  }();
  if (registered == 0) throw std::runtime_error("注册窗口类失败");

  // 先按系统 DPI 估算窗口大小，创建后再按窗口所在显示器的 DPI 校正
  HDC dc = GetDC(nullptr);
  const double systemScale = GetDeviceCaps(dc, LOGPIXELSX) / kBaseDpi;
  ReleaseDC(nullptr, dc);
  RECT frame{0, 0, static_cast<LONG>(std::lround(options.clientSize.width * systemScale)), static_cast<LONG>(std::lround(options.clientSize.height * systemScale))};
  AdjustWindowRectEx(&frame, WS_OVERLAPPEDWINDOW, FALSE, 0);
  impl_->hwnd = CreateWindowExW(0, kWindowClass, options.title.c_str(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, frame.right - frame.left, frame.bottom - frame.top, nullptr, nullptr,
                                GetModuleHandleW(nullptr), impl_.get());
  if (impl_->hwnd == nullptr) throw std::runtime_error("创建窗口失败");
  impl_->root.setInvalidateHandler([hwnd = impl_->hwnd] { InvalidateRect(hwnd, nullptr, FALSE); });
  impl_->applyDpi();
}

HostWindow::~HostWindow() {
  if (impl_->hwnd != nullptr && IsWindow(impl_->hwnd) != FALSE) {
    SetWindowLongPtrW(impl_->hwnd, GWLP_USERDATA, 0);
    DestroyWindow(impl_->hwnd);
  }
}

Root& HostWindow::root() { return impl_->root; }

void HostWindow::show() {
  ShowWindow(impl_->hwnd, SW_SHOW);
  UpdateWindow(impl_->hwnd);
}

int HostWindow::runMessageLoop() {
  MSG message;
  while (GetMessageW(&message, nullptr, 0, 0) > 0) {
    TranslateMessage(&message);
    DispatchMessageW(&message);
  }
  return static_cast<int>(message.wParam);
}

}  // namespace rujian::ui
