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
#include <imm.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>

#include "base/d2d-canvas.hpp"
#include "base/graphics.hpp"
#include "base/theme-tokens.hpp"

namespace rujian::ui {

namespace {

constexpr wchar_t kWindowClass[] = L"RujianUiHostWindow";
constexpr double kBaseDpi = 96.0;
/** 光标闪烁定时器的编号。 */
constexpr UINT_PTR kCaretTimer = 1;
/** IDC_ARROW、IDC_IBEAM、IDC_HAND、IDC_SIZEWE（字符串形式的宏在没有 UNICODE 时是窄字符，这里直接用编号）。 */
constexpr int kCursorArrow = 32512;
constexpr int kCursorIBeam = 32513;
constexpr int kCursorHand = 32649;
constexpr int kCursorSizeWE = 32644;

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
  bool imeEnabled{true};
  wchar_t highSurrogate{0};
  /** 应用的消息钩子，空表示没有。 */
  HostWindow::MessageHook hook;

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
    root.setDoubleClick(GetDoubleClickTime(), GetSystemMetrics(SM_CXDOUBLECLK) / 2.0 / scale());
    target.Reset();
    root.layout(clientLogicalSize());
  }

  /** 输入法：焦点控件需要文字输入时开启，候选窗和组合窗贴着文字光标。 */
  void syncIme() {
    const bool wants = root.focusAcceptsText();
    if (wants != imeEnabled) {
      ImmAssociateContextEx(hwnd, nullptr, wants ? IACE_DEFAULT : 0);
      imeEnabled = wants;
    }
    if (!wants) return;
    const auto caret = root.imeCaretRect();
    HIMC context = ImmGetContext(hwnd);
    if (!caret || context == nullptr) {
      if (context != nullptr) ImmReleaseContext(hwnd, context);
      return;
    }
    const POINT origin{static_cast<LONG>(std::lround(caret->x * scale())), static_cast<LONG>(std::lround(caret->y * scale()))};
    COMPOSITIONFORM composition{CFS_POINT, origin, RECT{}};
    ImmSetCompositionWindow(context, &composition);
    CANDIDATEFORM candidate{0, CFS_CANDIDATEPOS, POINT{origin.x, static_cast<LONG>(std::lround(caret->bottom() * scale()))}, RECT{}};
    ImmSetCandidateWindow(context, &candidate);
    ImmReleaseContext(hwnd, context);
  }

  /** 读输入法里的一段文字（GCS_COMPSTR 或 GCS_RESULTSTR）。 */
  static std::u16string imeString(HIMC context, DWORD kind) {
    const LONG bytes = ImmGetCompositionStringW(context, kind, nullptr, 0);
    if (bytes <= 0) return {};
    std::u16string text(static_cast<std::size_t>(bytes) / sizeof(char16_t), u'\0');
    ImmGetCompositionStringW(context, kind, text.data(), static_cast<DWORD>(bytes));
    return text;
  }

  /** 处理 WM_IME_COMPOSITION：先把确认的文字交出去，再更新还在组合的文字。 */
  void handleImeComposition(LPARAM flags) {
    HIMC context = ImmGetContext(hwnd);
    if (context == nullptr) return;
    if ((flags & GCS_RESULTSTR) != 0) root.imeCommit(imeString(context, GCS_RESULTSTR));
    if ((flags & GCS_COMPSTR) != 0 || flags == 0) {
      ImeComposition composition;
      composition.text = imeString(context, GCS_COMPSTR);
      composition.cursor = composition.text.empty() ? 0 : static_cast<std::size_t>(ImmGetCompositionStringW(context, GCS_CURSORPOS, nullptr, 0) & 0xFFFF);
      if (!composition.text.empty() && (flags & GCS_COMPATTR) != 0) {
        const LONG count = ImmGetCompositionStringW(context, GCS_COMPATTR, nullptr, 0);
        std::string attributes(static_cast<std::size_t>(std::max<LONG>(count, 0)), '\0');
        if (count > 0) ImmGetCompositionStringW(context, GCS_COMPATTR, attributes.data(), static_cast<DWORD>(count));
        // “正在转换”的分句：ATTR_TARGET_CONVERTED(1) 与 ATTR_TARGET_NOTCONVERTED(3)
        std::size_t first = attributes.size();
        std::size_t last = 0;
        for (std::size_t i = 0; i < attributes.size(); ++i) {
          if (attributes[i] == 1 || attributes[i] == 3) {
            first = std::min(first, i);
            last = i + 1;
          }
        }
        if (first < last) {
          composition.targetStart = first;
          composition.targetEnd = last;
        }
      }
      root.imeComposition(composition);
    }
    ImmReleaseContext(hwnd, context);
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
    if (hook) {
      std::intptr_t result = 0;
      if (hook(message, wParam, lParam, result)) return static_cast<LRESULT>(result);
    }
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
        root.mouseDown(toLogical(lParam), message == WM_LBUTTONDOWN ? MouseButton::Left : (message == WM_RBUTTONDOWN ? MouseButton::Right : MouseButton::Middle), currentModifiers(), static_cast<unsigned>(GetMessageTime()));
        syncIme();
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
        syncIme();
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
        syncIme();
        return 0;
      }
      case WM_SETCURSOR:
        if (LOWORD(lParam) == HTCLIENT) {
          const CursorKind kind = root.cursor();
          int cursorId = kCursorArrow;
          if (kind == CursorKind::IBeam) cursorId = kCursorIBeam;
          else if (kind == CursorKind::Hand) cursorId = kCursorHand;
          else if (kind == CursorKind::SizeWE) cursorId = kCursorSizeWE;
          SetCursor(LoadCursorW(nullptr, MAKEINTRESOURCEW(cursorId)));
          return TRUE;
        }
        break;
      case WM_TIMER:
        if (wParam == kCaretTimer) {
          root.toggleCaret();
          return 0;
        }
        root.fireTimer(static_cast<unsigned>(wParam));
        return 0;
      case WM_SETFOCUS:
        syncIme();
        return 0;
      case WM_IME_SETCONTEXT:
        // 隐藏系统自己的组合窗，组合串画在输入框里；候选窗仍由系统显示
        return DefWindowProcW(hwnd, message, wParam, lParam & ~static_cast<LPARAM>(ISC_SHOWUICOMPOSITIONWINDOW));
      case WM_IME_STARTCOMPOSITION:
        syncIme();
        return 0;
      case WM_IME_COMPOSITION:
        if (!root.focusAcceptsText()) break;
        handleImeComposition(lParam);
        syncIme();
        return 0;
      case WM_IME_ENDCOMPOSITION:
        root.imeEnd();
        return 0;
      case WM_DESTROY:
        KillTimer(hwnd, kCaretTimer);
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
  // 光标闪烁的间隔取系统设置；系统设为不闪烁（INFINITE）时只显示不闪
  impl_->root.setCaretTimerHandler([hwnd = impl_->hwnd](unsigned interval) {
    const UINT blink = GetCaretBlinkTime();
    if (interval == 0 || blink == 0 || blink == INFINITE) KillTimer(hwnd, kCaretTimer);
    else SetTimer(hwnd, kCaretTimer, blink, nullptr);
  });
  impl_->root.setTimerHandlers([hwnd = impl_->hwnd](unsigned id, unsigned milliseconds) { SetTimer(hwnd, id, milliseconds, nullptr); }, [hwnd = impl_->hwnd](unsigned id) { KillTimer(hwnd, id); });
  impl_->applyDpi();
}

HostWindow::~HostWindow() {
  if (impl_->hwnd != nullptr && IsWindow(impl_->hwnd) != FALSE) {
    SetWindowLongPtrW(impl_->hwnd, GWLP_USERDATA, 0);
    DestroyWindow(impl_->hwnd);
  }
}

Root& HostWindow::root() { return impl_->root; }

void* HostWindow::nativeHandle() const { return impl_->hwnd; }

void HostWindow::setMessageHook(MessageHook hook) { impl_->hook = std::move(hook); }

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
