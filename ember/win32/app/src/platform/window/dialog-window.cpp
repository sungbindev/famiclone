/*
 * ------------------------------------------------------------------------
 * 名称：dialog-window.cpp
 * 说明：对话框窗口的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-20
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "platform/window/dialog-window.hpp"

#include <windows.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>

#include "shared/input/scan-code-table.hpp"
#include "shared/shortcut/shortcut.hpp"

namespace ember::win32::platform {

namespace {

/** 内容高度按正文时，正文最高占工作区高度的这个比例。 */
constexpr double kMaximumBodyShare = 0.7;

/** 当前按着的修饰键（Shift=1、Ctrl=2、Alt=4 的位和）。 */
unsigned currentModifiers() {
  unsigned modifiers = 0;
  if ((GetKeyState(VK_SHIFT) & 0x8000) != 0) modifiers |= shared::kShortcutShift;
  if ((GetKeyState(VK_CONTROL) & 0x8000) != 0) modifiers |= shared::kShortcutCtrl;
  if ((GetKeyState(VK_MENU) & 0x8000) != 0) modifiers |= shared::kShortcutAlt;
  return modifiers;
}

rujian::ui::HostWindow::Options hostOptions(const DialogWindowOptions& options) {
  rujian::ui::HostWindow::Options host;
  host.title = options.title;
  host.clientSize = rujian::ui::Size{options.width > 0 ? options.width : 200.0, options.height > 0 ? options.height : 200.0};
  host.theme = options.theme;
  host.owner = options.owner;
  return host;
}

}  // namespace

DialogWindow::DialogWindow(DialogWindowOptions options, std::unique_ptr<rujian::ui::DialogFrame> frame) : options_(std::move(options)), window_(hostOptions(options_)) {
  frame_ = frame.get();
  frame_->setSystemButtons(true);
  frame_->setMaximizable(options_.maximizable);
  frame_->setMinimizable(options_.minimizable);
  frame_->setOnClose([this](rujian::ui::DialogCloseReason) {
    if (options_.onCloseRequested) options_.onCloseRequested();
  });
  frame_->setOnMinimize([this] { minimize(); });
  frame_->setOnToggleMaximize([this] {
    HWND hwnd = static_cast<HWND>(window_.nativeHandle());
    ShowWindow(hwnd, IsZoomed(hwnd) != FALSE ? SW_RESTORE : SW_MAXIMIZE);
  });
  window_.root().setContent(std::move(frame));

  FramelessCallbacks callbacks;
  callbacks.hitTest = [this](double x, double y) { return frame_->hitTestWindow(rujian::ui::Point{x, y}) == rujian::ui::DialogHit::Caption ? FrameHit::Caption : FrameHit::Client; };
  callbacks.maximizedChanged = [this](bool maximized) { frame_->setMaximized(maximized); };
  callbacks.activeChanged = [this](bool active) {
    frame_->setWindowActive(active);
    if (!active) window_.root().dismissLayers();
  };
  callbacks.nonClientPressed = [this] { window_.root().dismissLayers(); };
  FramelessOptions frameOptions{options_.minimumWidth, options_.minimumHeight, [this] { return window_.root().scale(); }, options_.customFrame};
  if (options_.fitMinimumWidth || options_.fitMinimumHeight) frameOptions.minimumProvider = [this] { return minimumSize(); };
  frameless_ = std::make_unique<FramelessFrame>(window_.nativeHandle(), std::move(frameOptions), std::move(callbacks));
  window_.setMessageHook([this](unsigned message, std::uintptr_t wParam, std::intptr_t lParam, std::intptr_t& result) {
    if (message == WM_CLOSE) {
      // 不让系统销毁窗口（销毁会让消息循环退出），只请求关闭
      if (options_.onCloseRequested) options_.onCloseRequested();
      result = 0;
      return true;
    }
    // 系统菜单、任务栏等处发来的最小化也走同一条路：对话框自己不单独最小化
    if (message == WM_SYSCOMMAND && (wParam & 0xFFF0) == SC_MINIMIZE) {
      minimize();
      result = 0;
      return true;
    }
    if (options_.onPhysicalKey || options_.onShortcutKey || !swallowedKeys_.empty()) {
      // 键的标识：扫描码加扩展位（lParam 的 16 到 24 位），按下、字符与抬起消息里都一样
      const int keyId = static_cast<int>((lParam >> 16) & 0x1FF);
      switch (message) {
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN: {
          // 按住不放的重复按下不再上报，但学习期间同样吞掉
          const bool repeat = (lParam & 0x40000000) != 0;
          if (!repeat) {
            const auto scan = static_cast<std::uint16_t>(keyId & 0xFF);
            const bool extended = (keyId & 0x100) != 0;
            const auto code = shared::scanCodeToKeyCode(scan, extended, false);
            const bool physicalTaken = code && options_.onPhysicalKey && options_.onPhysicalKey(std::string(*code));
            // 重新按下的键只看这一次有没有被拦截，之前漏掉抬起消息留下的标记不再算
            if (physicalTaken || (options_.onShortcutKey && options_.onShortcutKey(static_cast<int>(wParam), currentModifiers()))) swallowedKeys_.insert(keyId);
            else swallowedKeys_.erase(keyId);
          }
          if (swallowedKeys_.count(keyId) != 0) {
            result = 0;
            return true;
          }
          break;
        }
        case WM_CHAR:
        case WM_SYSCHAR:
        case WM_DEADCHAR:
        case WM_SYSDEADCHAR:
          if (swallowedKeys_.count(keyId) != 0) {
            result = 0;
            return true;
          }
          break;
        case WM_KEYUP:
        case WM_SYSKEYUP:
          if (swallowedKeys_.erase(keyId) != 0) {
            result = 0;
            return true;
          }
          break;
        default:
          break;
      }
    }
    return frameless_->handle(message, wParam, lParam, result);
  });
  frameless_->attach();
  // 非模态对话框自己最小化，要有自己的任务栏按钮才找得回来
  if (options_.modeless) {
    HWND hwnd = static_cast<HWND>(window_.nativeHandle());
    SetWindowLongPtrW(hwnd, GWL_EXSTYLE, GetWindowLongPtrW(hwnd, GWL_EXSTYLE) | WS_EX_APPWINDOW);
  }
  applyBorderColor();
  place();
}

DialogWindow::~DialogWindow() {
  HWND owner = static_cast<HWND>(options_.owner);
  if (ownerLocked_ && owner != nullptr && IsWindow(owner) != FALSE) {
    // 先恢复归属窗口可用再销毁窗口，系统才会把焦点还给它
    const bool inForeground = GetForegroundWindow() == static_cast<HWND>(window_.nativeHandle());
    EnableWindow(owner, TRUE);
    // 用 ShellExecute 打开网页再回来后，销毁前台窗口时系统可能把前台交给别的程序；趁本程序还在前台，先交给归属窗口
    if (inForeground) SetForegroundWindow(owner);
  }
  frameless_.reset();
}

std::pair<double, double> DialogWindow::minimumSize() {
  double width = options_.minimumWidth;
  double height = options_.minimumHeight;
  if (options_.fitMinimumWidth || options_.fitMinimumHeight) {
    const rujian::ui::Size fit = frame_->minimumWindowSize();
    if (options_.fitMinimumWidth) width = std::max(width, fit.width);
    if (options_.fitMinimumHeight) height = std::max(height, fit.height);
  }
  return {width, height};
}

void DialogWindow::place() {
  HWND hwnd = static_cast<HWND>(window_.nativeHandle());
  HWND owner = static_cast<HWND>(options_.owner);
  RECT ownerRect{};
  if (owner == nullptr || GetWindowRect(owner, &ownerRect) == FALSE) {
    const int width = GetSystemMetrics(SM_CXSCREEN);
    const int height = GetSystemMetrics(SM_CYSCREEN);
    ownerRect = RECT{0, 0, width, height};
  }
  // 先移到归属窗口的显示器上，窗口的缩放比例才是那台显示器的
  SetWindowPos(hwnd, nullptr, ownerRect.left, ownerRect.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
  const double scale = window_.root().scale();
  MONITORINFO monitor{sizeof(monitor)};
  RECT work = ownerRect;
  if (GetMonitorInfoW(MonitorFromRect(&ownerRect, MONITOR_DEFAULTTONEAREST), &monitor) != FALSE) work = monitor.rcWork;
  const auto [minimumWidth, minimumHeight] = minimumSize();
  // 最小宽度按内容算的方向，打开时就用最小值（向上取整到设备像素，与系统的最小尺寸查询一致）；最小高度按内容算时，登记了初始高度就取它与最小高度中大的
  const int width = options_.fitMinimumWidth ? static_cast<int>(std::ceil(minimumWidth * scale - 1e-6)) : static_cast<int>(std::lround(std::max(options_.width, minimumWidth) * scale));
  const int fitHeight = static_cast<int>(std::ceil(minimumHeight * scale - 1e-6));
  const int pixelHeight = options_.fitMinimumHeight
                              ? std::max(fitHeight, static_cast<int>(std::lround(options_.height * scale)))
                              : static_cast<int>(std::lround(std::max(options_.height > 0 ? options_.height : frame_->naturalHeight(std::max(options_.width, minimumWidth), (work.bottom - work.top) / scale * kMaximumBodyShare), minimumHeight) * scale));
  const int ownerCenterX = (ownerRect.left + ownerRect.right) / 2;
  const int ownerCenterY = (ownerRect.top + ownerRect.bottom) / 2;
  const int left = std::clamp(ownerCenterX - width / 2, static_cast<int>(work.left), std::max(static_cast<int>(work.left), static_cast<int>(work.right) - width));
  const int top = std::clamp(ownerCenterY - pixelHeight / 2, static_cast<int>(work.top), std::max(static_cast<int>(work.top), static_cast<int>(work.bottom) - pixelHeight));
  SetWindowPos(hwnd, nullptr, left, top, width, pixelHeight, SWP_NOZORDER | SWP_NOACTIVATE);
}

void DialogWindow::minimize() {
  HWND target = static_cast<HWND>(options_.owner);
  // 非模态对话框只最小化自己，主窗口照常使用
  if (options_.modeless) target = static_cast<HWND>(window_.nativeHandle());
  if (target != nullptr && IsWindow(target) != FALSE) ShowWindow(target, SW_MINIMIZE);
}

void DialogWindow::show() {
  if (options_.owner != nullptr && !options_.modeless) {
    EnableWindow(static_cast<HWND>(options_.owner), FALSE);
    ownerLocked_ = true;
  }
  window_.show();
  frame_->focusInitial();
}

void DialogWindow::activate() {
  HWND hwnd = static_cast<HWND>(window_.nativeHandle());
  if (IsIconic(hwnd) != FALSE) ShowWindow(hwnd, SW_RESTORE);
  SetForegroundWindow(hwnd);
}

void DialogWindow::setTheme(rujian::ui::ThemeName theme) {
  window_.root().setTheme(theme);
  applyBorderColor();
}

void DialogWindow::applyBorderColor() {
  const rujian::ui::Color border = rujian::ui::windowBorderColor(window_.root().theme());
  frameless_->setBorderColor(static_cast<unsigned>(border.r()) | (static_cast<unsigned>(border.g()) << 8) | (static_cast<unsigned>(border.b()) << 16));
}

void* DialogWindow::handle() const { return window_.nativeHandle(); }

rujian::ui::Point DialogWindow::clientPointFromScreen(int x, int y) {
  POINT point{x, y};
  ScreenToClient(static_cast<HWND>(window_.nativeHandle()), &point);
  const double scale = window_.root().scale();
  return rujian::ui::Point{point.x / scale, point.y / scale};
}

bool DialogWindow::resizeWidthBy(double delta) {
  HWND hwnd = static_cast<HWND>(window_.nativeHandle());
  RECT rect{};
  if (IsZoomed(hwnd) != FALSE || IsIconic(hwnd) != FALSE || GetWindowRect(hwnd, &rect) == FALSE) return false;
  const double scale = window_.root().scale();
  RECT work = rect;
  MONITORINFO monitor{sizeof(monitor)};
  if (GetMonitorInfoW(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &monitor) != FALSE) work = monitor.rcWork;
  const int minimumWidth = static_cast<int>(std::ceil(minimumSize().first * scale - 1e-6));
  const int workWidth = static_cast<int>(work.right - work.left);
  const int width = std::clamp(static_cast<int>(rect.right - rect.left) + static_cast<int>(std::lround(delta * scale)), std::min(minimumWidth, workWidth), workWidth);
  const int left = std::clamp(static_cast<int>(rect.left), static_cast<int>(work.left), std::max(static_cast<int>(work.left), static_cast<int>(work.right) - width));
  SetWindowPos(hwnd, nullptr, left, rect.top, width, rect.bottom - rect.top, SWP_NOZORDER | SWP_NOACTIVATE);
  return true;
}

}  // namespace ember::win32::platform
