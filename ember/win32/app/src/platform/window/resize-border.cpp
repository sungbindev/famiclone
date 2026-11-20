/*
 * ------------------------------------------------------------------------
 * 名称：resize-border.cpp
 * 说明：窗口的缩放边框的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-20
 * 备注：窗口类只注册一次。
 * ------------------------------------------------------------------------
 */

#include "platform/window/resize-border.hpp"

#include <windows.h>
#include <windowsx.h>

#include <utility>

namespace ember::win32::platform {

namespace {

constexpr wchar_t kBorderClass[] = L"EmberResizeBorder";

LRESULT CALLBACK borderProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
  auto* border = reinterpret_cast<ResizeBorder*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
  switch (message) {
    case WM_NCHITTEST:
      return border != nullptr ? static_cast<LRESULT>(border->hitTest(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam))) : HTTRANSPARENT;
    case WM_NCLBUTTONDOWN:
      if (border != nullptr) border->press(static_cast<unsigned long>(wParam), GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
      return 0;
    case WM_MOUSEACTIVATE:
      return MA_NOACTIVATE;
    default:
      // 鼠标指针由系统按命中结果换成各方向的缩放指针
      return DefWindowProcW(hwnd, message, wParam, lParam);
  }
}

ATOM registerBorderClass() {
  static const ATOM atom = [] {
    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = borderProc;
    windowClass.hInstance = GetModuleHandleW(nullptr);
    windowClass.lpszClassName = kBorderClass;
    return RegisterClassExW(&windowClass);
  }();
  return atom;
}

}  // namespace

ResizeBorder::ResizeBorder(void* window, std::function<void()> onPressed) : main_(window), onPressed_(std::move(onPressed)) {}

ResizeBorder::~ResizeBorder() {
  if (border_ != nullptr) DestroyWindow(static_cast<HWND>(border_));
}

bool ResizeBorder::ensureWindow() {
  if (border_ != nullptr) return true;
  if (registerBorderClass() == 0) return false;
  HWND created = CreateWindowExW(WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, kBorderClass, L"", WS_POPUP, 0, 0, 0, 0, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
  if (created == nullptr) return false;
  SetWindowLongPtrW(created, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
  // 完全透明的分层窗口不接鼠标，所以留 1/255 的不透明度
  SetLayeredWindowAttributes(created, 0, 1, LWA_ALPHA);
  border_ = created;
  return true;
}

void ResizeBorder::sync(int thickness) {
  thickness_ = thickness;
  HWND main = static_cast<HWND>(main_);
  const bool wanted = !suppressed_ && thickness > 0 && IsWindowVisible(main) != FALSE && IsIconic(main) == FALSE && IsZoomed(main) == FALSE;
  if (!wanted) {
    if (border_ != nullptr && IsWindowVisible(static_cast<HWND>(border_)) != FALSE) ShowWindow(static_cast<HWND>(border_), SW_HIDE);
    return;
  }
  RECT rect{};
  if (GetWindowRect(main, &rect) == FALSE || !ensureWindow()) return;
  // 放到主窗口的正后面；主窗口置顶时这样放，缩放区也在置顶的那一层
  SetWindowPos(static_cast<HWND>(border_), main, rect.left - thickness, rect.top - thickness, rect.right - rect.left + 2 * thickness, rect.bottom - rect.top + 2 * thickness, SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

void ResizeBorder::setSuppressed(bool suppressed) {
  if (suppressed_ == suppressed) return;
  suppressed_ = suppressed;
  sync(thickness_);
}

long ResizeBorder::hitTest(int screenX, int screenY) const {
  HWND main = static_cast<HWND>(main_);
  // 主窗口被禁用（模态对话框打开期间）时缩放不起作用，鼠标交给下面的窗口
  if (suppressed_ || IsWindowEnabled(main) == FALSE) return HTTRANSPARENT;
  RECT rect{};
  GetWindowRect(main, &rect);
  const bool left = screenX < rect.left;
  const bool right = screenX >= rect.right;
  const bool above = screenY < rect.top;
  const bool below = screenY >= rect.bottom;
  if (!left && !right && !above && !below) return HTTRANSPARENT;
  if (screenX < rect.left - thickness_ || screenX >= rect.right + thickness_ || screenY < rect.top - thickness_ || screenY >= rect.bottom + thickness_) return HTTRANSPARENT;
  // 四角的抓取范围沿着边再延伸一段，比只有厚度见方的小角容易点中
  const int corner = thickness_ * 2;
  const bool nearLeft = screenX < rect.left + corner;
  const bool nearRight = screenX >= rect.right - corner;
  const bool nearTop = screenY < rect.top + corner;
  const bool nearBottom = screenY >= rect.bottom - corner;
  if (above || below) {
    if (nearLeft) return above ? HTTOPLEFT : HTBOTTOMLEFT;
    if (nearRight) return above ? HTTOPRIGHT : HTBOTTOMRIGHT;
    return above ? HTTOP : HTBOTTOM;
  }
  if (nearTop) return left ? HTTOPLEFT : HTTOPRIGHT;
  if (nearBottom) return left ? HTBOTTOMLEFT : HTBOTTOMRIGHT;
  return left ? HTLEFT : HTRIGHT;
}

void ResizeBorder::press(unsigned long hit, int screenX, int screenY) {
  if (hit < HTLEFT || hit > HTBOTTOMRIGHT) return;
  if (onPressed_) onPressed_();
  HWND main = static_cast<HWND>(main_);
  SetForegroundWindow(main);
  // 与系统自己处理缩放边框时发的命令一致：SC_SIZE 加方向（左、右、上、左上、右上、下、左下、右下依次是 1 到 8）
  SendMessageW(main, WM_SYSCOMMAND, SC_SIZE + (hit - HTLEFT + 1), MAKELPARAM(static_cast<WORD>(screenX), static_cast<WORD>(screenY)));
}

}  // namespace ember::win32::platform
