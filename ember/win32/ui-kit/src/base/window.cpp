/*
 * ------------------------------------------------------------------------
 * 名称：window.cpp
 * 说明：窗口宿主的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：Windows 7 没有 GetDpiForWindow，按“动态查找该函数，找不到就用系统 DPI”处理。
 *       声明为 floating 的弹层显示成独立的分层小窗口（不抢焦点、点击透明像素穿透），鼠标消息转回同一个 Root。
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
/** IDC_ARROW、IDC_IBEAM、IDC_HAND、IDC_SIZEWE、IDC_SIZEALL、IDC_NO（字符串形式的宏在没有 UNICODE 时是窄字符，这里直接用编号）。 */
constexpr int kCursorArrow = 32512;
constexpr int kCursorIBeam = 32513;
constexpr int kCursorHand = 32649;
constexpr int kCursorSizeWE = 32644;
constexpr int kCursorMove = 32646;
constexpr int kCursorNotAllowed = 32648;

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

/** DwmSetWindowAttribute 的“隐形”属性编号（Windows 8 起有）。 */
constexpr DWORD kDwmCloak = 13;

/** 让 DWM 不合成窗口（窗口仍然可见、正常收消息与绘制）；系统不支持时返回 false。 */
bool setCloaked(HWND hwnd, bool cloaked) {
  using SetAttributeFn = HRESULT(WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
  static const auto setAttribute = reinterpret_cast<SetAttributeFn>(reinterpret_cast<void*>(GetProcAddress(LoadLibraryW(L"dwmapi.dll"), "DwmSetWindowAttribute")));
  if (setAttribute == nullptr) return false;
  const BOOL value = cloaked ? TRUE : FALSE;
  return SUCCEEDED(setAttribute(hwnd, kDwmCloak, &value, sizeof(value)));
}

/** 等 DWM 合成完当前的内容（dwmapi 不进入导入表，找不到就什么也不做）。 */
void flushComposition() {
  using FlushFn = HRESULT(WINAPI*)();
  static const auto flush = reinterpret_cast<FlushFn>(reinterpret_cast<void*>(GetProcAddress(LoadLibraryW(L"dwmapi.dll"), "DwmFlush")));
  if (flush != nullptr) flush();
}

/** 弹层小窗口的窗口类名。 */
constexpr wchar_t kLayerClass[] = L"RujianUiLayerWindow";

/** 一组阴影伸到盒子外的最大距离（逻辑像素）；模糊的可见范围约为模糊半径的 1.25 倍。 */
double shadowReach(const ShadowList& shadows) {
  double reach = 0.0;
  for (int i = 0; i < shadows.count; ++i) {
    const Shadow& shadow = shadows.items[static_cast<std::size_t>(i)];
    if (shadow.inset) continue;
    reach = std::max(reach, shadow.spread + 1.25 * shadow.blur + std::max(std::abs(shadow.offsetX), std::abs(shadow.offsetY)));
  }
  return reach;
}

/** 控件要求的光标样式对应的系统光标。 */
HCURSOR cursorFor(CursorKind kind) {
  int id = kCursorArrow;
  if (kind == CursorKind::IBeam) id = kCursorIBeam;
  else if (kind == CursorKind::Hand) id = kCursorHand;
  else if (kind == CursorKind::SizeWE) id = kCursorSizeWE;
  else if (kind == CursorKind::Move) id = kCursorMove;
  else if (kind == CursorKind::NotAllowed) id = kCursorNotAllowed;
  return LoadCursorW(nullptr, MAKEINTRESOURCEW(id));
}

}  // namespace

struct HostWindow::Impl {
  HWND hwnd{nullptr};
  Root root;
  Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget> target;
  /** 上一帧的完整画面（与窗口等大）：缩放时先用它把新大小的窗口画满，再重新布局绘制。 */
  Microsoft::WRL::ComPtr<ID2D1BitmapRenderTarget> back;
  D2D1_SIZE_U backPixels{};
  bool trackingLeave{false};
  bool imeEnabled{true};
  wchar_t highSurrogate{0};
  /** 应用的消息钩子，空表示没有。 */
  HostWindow::MessageHook hook;

  /**
   * 一个弹层的独立小窗口：逐像素透明度的分层窗口（UpdateLayeredWindow 呈现），归属于宿主窗口，
   * 不抢键盘焦点、不进任务栏；内容画进内存位图后整幅呈现，圆角与阴影外的透明像素点击穿透。
   * 鼠标消息换算成宿主窗口坐标后转给同一个 Root，命中、焦点、弹层外按下的分发逻辑不变。
   */
  struct LayerWindow {
    Impl* owner{nullptr};
    Widget* widget{nullptr};
    HWND hwnd{nullptr};
    HDC memory{nullptr};
    HBITMAP bitmap{nullptr};
    HGDIOBJ originalBitmap{nullptr};
    SIZE pixels{};
    Microsoft::WRL::ComPtr<ID2D1DCRenderTarget> target;
    bool trackingLeave{false};

    ~LayerWindow() {
      if (hwnd != nullptr && IsWindow(hwnd) != FALSE) {
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        DestroyWindow(hwnd);
      }
      target.Reset();
      if (bitmap != nullptr) {
        if (memory != nullptr && originalBitmap != nullptr) SelectObject(memory, originalBitmap);
        DeleteObject(bitmap);
      }
      if (memory != nullptr) DeleteDC(memory);
    }

    /** 弹层控件还在显示中（控件可能已销毁，窗口到下一次同步才移除）。 */
    [[nodiscard]] bool alive() const {
      const std::vector<Widget*> floating = owner->root.floatingLayers();
      return std::ranges::find(floating, widget) != floating.end();
    }

    /** 把小窗口客户区里的事件坐标换成宿主窗口的逻辑坐标。 */
    [[nodiscard]] Point toRoot(LPARAM lParam) const {
      POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
      MapWindowPoints(hwnd, owner->hwnd, &point, 1);
      return Point{point.x / owner->scale(), point.y / owner->scale()};
    }
  };

  /** 显示中的弹层小窗口；声明在 root 之后，先于 root 销毁。 */
  std::vector<std::unique_ptr<LayerWindow>> layerWindows;

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
    back.Reset();
    for (const auto& layer : layerWindows) layer->target.Reset();
    root.layout(clientLogicalSize());
  }

  // ---- 弹层的独立小窗口 ----

  /** 为弹层准备内存位图与渲染目标（大小变了就重建）。 */
  void ensureLayerSurface(LayerWindow& layer, SIZE size) {
    if (layer.memory != nullptr && layer.target && layer.pixels.cx == size.cx && layer.pixels.cy == size.cy) return;
    layer.target.Reset();
    if (layer.memory == nullptr) layer.memory = CreateCompatibleDC(nullptr);
    if (layer.bitmap != nullptr) {
      if (layer.originalBitmap != nullptr) SelectObject(layer.memory, layer.originalBitmap);
      DeleteObject(layer.bitmap);
      layer.bitmap = nullptr;
      layer.originalBitmap = nullptr;
    }
    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = size.cx;
    info.bmiHeader.biHeight = -size.cy;  // 行序从上到下，与 UpdateLayeredWindow 一致
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    void* bits = nullptr;
    layer.bitmap = CreateDIBSection(layer.memory, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (layer.bitmap != nullptr) layer.originalBitmap = SelectObject(layer.memory, layer.bitmap);
    layer.pixels = size;
    const auto dpi = static_cast<FLOAT>(kBaseDpi * scale());
    const D2D1_RENDER_TARGET_PROPERTIES properties = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT, D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), dpi, dpi);
    Graphics::shared().d2d()->CreateDCRenderTarget(&properties, &layer.target);
    const RECT rect{0, 0, size.cx, size.cy};
    if (layer.target) layer.target->BindDC(layer.memory, &rect);
  }

  /** 把弹层画进小窗口的内存位图：平移量取整到设备像素，控件边缘的像素对齐不变。 */
  void drawLayer(LayerWindow& layer, SIZE size, Point origin) {
    ensureLayerSurface(layer, size);
    if (!layer.target) return;
    layer.target->SetTransform(D2D1::Matrix3x2F::Translation(static_cast<FLOAT>(-origin.x), static_cast<FLOAT>(-origin.y)));
    layer.target->BeginDraw();
    layer.target->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));
    {
      D2DCanvas canvas(layer.target.Get(), scale(), Graphics::shared());
      root.paintLayer(layer.widget, canvas);
    }
    if (FAILED(layer.target->EndDraw())) layer.target.Reset();
  }

  /** 同步弹层小窗口：按显示顺序建窗口、摆位置、重画，关掉的移除；每次宿主窗口绘制后调用。 */
  void syncLayers() {
    const std::vector<Widget*> floating = root.floatingLayers();
    for (auto it = layerWindows.begin(); it != layerWindows.end();) {
      if (std::ranges::find(floating, (*it)->widget) == floating.end()) it = layerWindows.erase(it);
      else ++it;
    }
    if (IsWindowVisible(hwnd) == FALSE) return;
    // 窗口比弹层盒子四周各大出一圈，阴影才不被裁掉
    const auto& colors = themeTokens(root.theme());
    const double margin = std::ceil(std::max(shadowReach(colors.menu_shadow), shadowReach(colors.layer_shadow))) + 2.0;
    const double currentScale = scale();
    for (Widget* widget : floating) {
      LayerWindow* layer = nullptr;
      for (const auto& item : layerWindows) {
        if (item->widget == widget) layer = item.get();
      }
      if (layer == nullptr) {
        static const ATOM registered = [] {
          WNDCLASSEXW windowClass{sizeof(windowClass)};
          windowClass.lpfnWndProc = &Impl::layerProcedure;
          windowClass.hInstance = GetModuleHandleW(nullptr);
          windowClass.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(kCursorArrow));
          windowClass.lpszClassName = kLayerClass;
          return RegisterClassExW(&windowClass);
        }();
        if (registered == 0) continue;
        auto owned = std::make_unique<LayerWindow>();
        owned->owner = this;
        owned->widget = widget;
        owned->hwnd = CreateWindowExW(WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, kLayerClass, L"", WS_POPUP, 0, 0, 1, 1, hwnd, nullptr, GetModuleHandleW(nullptr), owned.get());
        if (owned->hwnd == nullptr) continue;
        layer = owned.get();
        layerWindows.push_back(std::move(owned));
      }
      const Rect box = widget->bounds();
      if (box.width <= 0 || box.height <= 0) {
        ShowWindow(layer->hwnd, SW_HIDE);
        continue;
      }
      const LONG left = static_cast<LONG>(std::floor((box.x - margin) * currentScale));
      const LONG top = static_cast<LONG>(std::floor((box.y - margin) * currentScale));
      SIZE size{static_cast<LONG>(std::ceil((box.right() + margin) * currentScale)) - left, static_cast<LONG>(std::ceil((box.bottom() + margin) * currentScale)) - top};
      POINT position{left, top};
      ClientToScreen(hwnd, &position);
      drawLayer(*layer, size, Point{left / currentScale, top / currentScale});
      BLENDFUNCTION blend{AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
      POINT source{0, 0};
      UpdateLayeredWindow(layer->hwnd, nullptr, &position, &size, layer->memory, &source, 0, &blend, ULW_ALPHA);
      if (IsWindowVisible(layer->hwnd) == FALSE) ShowWindow(layer->hwnd, SW_SHOWNOACTIVATE);
      // 按显示顺序保持前后关系：后显示的弹层（如子菜单）盖在先显示的上面
      SetWindowPos(layer->hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
  }

  /** 弹层小窗口的窗口过程：鼠标消息换算成宿主窗口坐标后转给同一个 Root。 */
  static LRESULT CALLBACK layerProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_NCCREATE) {
      const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lParam);
      auto* self = static_cast<LayerWindow*>(create->lpCreateParams);
      self->hwnd = hwnd;
      SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    auto* self = reinterpret_cast<LayerWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (self == nullptr) return DefWindowProcW(hwnd, message, wParam, lParam);
    Impl* impl = self->owner;
    switch (message) {
      case WM_NCHITTEST: {
        // 盒子外（阴影、圆角外的透明像素）点击穿透，交给下面的窗口
        POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        ScreenToClient(impl->hwnd, &point);
        const Point logical{point.x / impl->scale(), point.y / impl->scale()};
        if (self->alive() && self->widget->bounds().contains(logical)) return HTCLIENT;
        return HTTRANSPARENT;
      }
      case WM_MOUSEACTIVATE:
        return MA_NOACTIVATE;
      case WM_ERASEBKGND:
        return 1;
      case WM_PAINT: {
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);
        EndPaint(hwnd, &ps);
        return 0;
      }
      case WM_MOUSEMOVE:
        if (!self->trackingLeave) {
          TRACKMOUSEEVENT track{sizeof(track), TME_LEAVE, hwnd, 0};
          TrackMouseEvent(&track);
          self->trackingLeave = true;
        }
        impl->root.mouseMove(self->toRoot(lParam), currentModifiers());
        return 0;
      case WM_MOUSELEAVE:
        self->trackingLeave = false;
        impl->root.mouseLeave();
        return 0;
      case WM_LBUTTONDOWN:
      case WM_RBUTTONDOWN:
      case WM_MBUTTONDOWN: {
        const Point logical = self->toRoot(lParam);
        // 应用的钩子也要收到按下（坐标换成宿主窗口的客户区坐标），外壳的点击副作用与点在窗口里时一致
        if (impl->hook) {
          std::intptr_t result = 0;
          const LPARAM translated = MAKELPARAM(std::lround(logical.x * impl->scale()), std::lround(logical.y * impl->scale()));
          if (impl->hook(message, wParam, translated, result)) return static_cast<LRESULT>(result);
        }
        SetCapture(hwnd);
        impl->root.mouseDown(logical, message == WM_LBUTTONDOWN ? MouseButton::Left : (message == WM_RBUTTONDOWN ? MouseButton::Right : MouseButton::Middle), currentModifiers(), static_cast<unsigned>(GetMessageTime()));
        return 0;
      }
      case WM_LBUTTONUP:
      case WM_RBUTTONUP:
      case WM_MBUTTONUP: {
        const Point logical = self->toRoot(lParam);
        if (impl->hook) {
          std::intptr_t result = 0;
          const LPARAM translated = MAKELPARAM(std::lround(logical.x * impl->scale()), std::lround(logical.y * impl->scale()));
          if (impl->hook(message, wParam, translated, result)) {
            if (GetCapture() == hwnd) ReleaseCapture();
            return static_cast<LRESULT>(result);
          }
        }
        impl->root.mouseUp(logical, message == WM_LBUTTONUP ? MouseButton::Left : (message == WM_RBUTTONUP ? MouseButton::Right : MouseButton::Middle), currentModifiers());
        if (GetCapture() == hwnd) ReleaseCapture();
        return 0;
      }
      case WM_SETCURSOR:
        if (LOWORD(lParam) == HTCLIENT) {
          SetCursor(cursorFor(impl->root.cursor()));
          return TRUE;
        }
        break;
      default:
        break;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
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

  /** 渲染目标出错需要重建时，连同上一帧一起丢掉。 */
  void resetTargets() {
    back.Reset();
    target.Reset();
  }

  /** 把整棵控件树画到上一帧的位图里，再整幅呈现到窗口的渲染目标上。 */
  void draw() {
    ensureTarget();
    if (!target) return;
    const D2D1_SIZE_U pixels = target->GetPixelSize();
    if (!back || backPixels.width != pixels.width || backPixels.height != pixels.height) {
      back.Reset();
      const D2D1_SIZE_F size = D2D1::SizeF(static_cast<FLOAT>(pixels.width / scale()), static_cast<FLOAT>(pixels.height / scale()));
      if (SUCCEEDED(target->CreateCompatibleRenderTarget(&size, &pixels, nullptr, D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS_NONE, &back))) backPixels = pixels;
    }
    ID2D1RenderTarget* surface = back ? static_cast<ID2D1RenderTarget*>(back.Get()) : static_cast<ID2D1RenderTarget*>(target.Get());
    surface->BeginDraw();
    {
      D2DCanvas canvas(surface, scale(), Graphics::shared());
      canvas.clear(themeTokens(root.theme()).content_bg);
      root.paint(canvas);
    }
    if (surface->EndDraw() == D2DERR_RECREATE_TARGET) {
      resetTargets();
      return;
    }
    if (back) {
      Microsoft::WRL::ComPtr<ID2D1Bitmap> frame;
      if (SUCCEEDED(back->GetBitmap(&frame))) {
        target->BeginDraw();
        target->DrawBitmap(frame.Get());
        if (target->EndDraw() == D2DERR_RECREATE_TARGET) resetTargets();
      }
    }
    syncLayers();
  }

  /**
   * 窗口刚改了大小、还没重新布局时，先用上一帧把新大小的窗口画满（多出的部分是底色）。
   * 改了大小后系统把整个窗口当作没画过：到下一次呈现之间合成，会闪出边框色；这一步把这段时间缩到最短。
   * 呈现时要连子窗口（运行画面）所在的地方一起画，不然那里要等子窗口自己呈现，同样会闪，所以这一步暂时去掉 WS_CLIPCHILDREN。
   */
  void drawStale() {
    if (!target || !back) return;
    Microsoft::WRL::ComPtr<ID2D1Bitmap> frame;
    if (FAILED(back->GetBitmap(&frame))) return;
    const LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
    SetWindowLongPtrW(hwnd, GWL_STYLE, style & ~static_cast<LONG_PTR>(WS_CLIPCHILDREN));
    target->BeginDraw();
    {
      D2DCanvas canvas(target.Get(), scale(), Graphics::shared());
      canvas.clear(themeTokens(root.theme()).content_bg);
    }
    target->DrawBitmap(frame.Get());
    if (target->EndDraw() == D2DERR_RECREATE_TARGET) resetTargets();
    SetWindowLongPtrW(hwnd, GWL_STYLE, style);
  }

  /** 渲染目标的大小与窗口客户区不一致时，改成一致并先用上一帧画满。 */
  void resizeTarget() {
    RECT rect{};
    if (!target || IsIconic(hwnd) != FALSE || GetClientRect(hwnd, &rect) == FALSE) return;
    const D2D1_SIZE_U size = D2D1::SizeU(static_cast<UINT>(rect.right - rect.left), static_cast<UINT>(rect.bottom - rect.top));
    const D2D1_SIZE_U current = target->GetPixelSize();
    if (size.width == 0 || size.height == 0 || (size.width == current.width && size.height == current.height)) return;
    target->Resize(size);
    if (IsWindowVisible(hwnd) != FALSE) drawStale();
  }

  void paint() {
    PAINTSTRUCT ps;
    BeginPaint(hwnd, &ps);
    draw();
    EndPaint(hwnd, &ps);
  }

  LRESULT handle(UINT message, WPARAM wParam, LPARAM lParam) {
    // 尺寸改变后要最先把新大小画满，在应用的钩子（边框、阴影的同步较慢）之前
    if (message == WM_WINDOWPOSCHANGED && (reinterpret_cast<const WINDOWPOS*>(lParam)->flags & SWP_NOSIZE) == 0) resizeTarget();
    // 窗口移动时弹层的屏幕位置作废，通知它们自己关闭
    if (message == WM_WINDOWPOSCHANGED && (reinterpret_cast<const WINDOWPOS*>(lParam)->flags & SWP_NOMOVE) == 0) root.dismissLayers();
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
          resizeTarget();
          root.layout(clientLogicalSize());
          if (IsWindowVisible(hwnd) != FALSE) {
            draw();
            ValidateRect(hwnd, nullptr);
          }
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
          SetCursor(cursorFor(root.cursor()));
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
  // WS_CLIPCHILDREN：应用在窗口里放原生子窗口（如运行画面）时，窗口自己的绘制不盖到子窗口上
  impl_->hwnd = CreateWindowExW(0, kWindowClass, options.title.c_str(), WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, frame.right - frame.left, frame.bottom - frame.top, static_cast<HWND>(options.owner), nullptr,
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
  // 声明为独立窗口的弹层由本窗口画到自己的小窗口上，可用范围是所在显示器的工作区（窗口坐标，可超出窗口）
  impl_->root.setExternalLayersEnabled(true);
  impl_->root.setLayerViewportProvider([impl = impl_.get()] {
    MONITORINFO monitor{sizeof(monitor)};
    if (GetMonitorInfoW(MonitorFromWindow(impl->hwnd, MONITOR_DEFAULTTONEAREST), &monitor) == FALSE) return Rect{0, 0, impl->root.size().width, impl->root.size().height};
    POINT origin{0, 0};
    ClientToScreen(impl->hwnd, &origin);
    const double layerScale = impl->scale();
    return Rect{(monitor.rcWork.left - origin.x) / layerScale, (monitor.rcWork.top - origin.y) / layerScale, (monitor.rcWork.right - monitor.rcWork.left) / layerScale,
                (monitor.rcWork.bottom - monitor.rcWork.top) / layerScale};
  });
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
  // 第一次绘制要创建渲染目标并第一次排字，较慢；先在窗口还隐藏时画一遍，显示后的绘制就快
  impl_->draw();
  // 窗口刚显示时，DWM 先合成一块白底，内容画好之前会闪一下白；先让 DWM 不合成这个窗口，画好并合成后再亮出来（Windows 7 没有这个属性，只能照常显示）
  const bool cloaked = setCloaked(impl_->hwnd, true);
  ShowWindow(impl_->hwnd, SW_SHOW);
  // 连子窗口（运行画面）一起立即重绘，不等消息循环
  RedrawWindow(impl_->hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
  if (cloaked) {
    flushComposition();
    setCloaked(impl_->hwnd, false);
  }
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
