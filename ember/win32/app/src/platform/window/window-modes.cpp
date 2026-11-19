/*
 * ------------------------------------------------------------------------
 * 名称：window-modes.cpp
 * 说明：主窗口模式的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "platform/window/window-modes.hpp"

#include <windows.h>
#include <shellapi.h>

#include "platform/text/wide-text.hpp"
#include "platform/window/frameless-frame.hpp"

namespace ember::win32::platform {

static_assert(sizeof(WINDOWPLACEMENT) <= 48, "保存窗口位置的字节数不够");

WindowModes::WindowModes(void* window, FramelessFrame& frame, std::function<void()> onChanged, std::function<void(std::vector<std::string>)> onDropped)
    : window_(window), frame_(frame), onChanged_(std::move(onChanged)), onDropped_(std::move(onDropped)) {
  DragAcceptFiles(static_cast<HWND>(window_), TRUE);
}

WindowModes::~WindowModes() {
  // 窗口先于本对象销毁时句柄已经失效，系统会忽略
  DragAcceptFiles(static_cast<HWND>(window_), FALSE);
}

bool WindowModes::alwaysOnTop() const { return (GetWindowLongPtrW(static_cast<HWND>(window_), GWL_EXSTYLE) & WS_EX_TOPMOST) != 0; }

void WindowModes::toggleAlwaysOnTop() {
  HWND hwnd = static_cast<HWND>(window_);
  SetWindowPos(hwnd, alwaysOnTop() ? HWND_NOTOPMOST : HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
  if (onChanged_) onChanged_();
}

void WindowModes::toggleFullScreen() {
  HWND hwnd = static_cast<HWND>(window_);
  auto* placement = reinterpret_cast<WINDOWPLACEMENT*>(savedPlacement_);
  if (!fullScreen_) {
    placement->length = sizeof(WINDOWPLACEMENT);
    GetWindowPlacement(hwnd, placement);
    savedStyle_ = static_cast<std::uintptr_t>(GetWindowLongPtrW(hwnd, GWL_STYLE));
    MONITORINFO monitor{sizeof(monitor)};
    if (GetMonitorInfoW(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &monitor) == FALSE) return;
    fullScreen_ = true;
    frame_.setFullScreen(true);
    // 去掉标题与缩放边框的样式（也去掉最大化标记，退出时由保存的位置还原），再铺满显示器
    SetWindowLongPtrW(hwnd, GWL_STYLE, static_cast<LONG_PTR>(savedStyle_ & ~static_cast<std::uintptr_t>(WS_CAPTION | WS_THICKFRAME | WS_MAXIMIZE)));
    SetWindowPos(hwnd, nullptr, monitor.rcMonitor.left, monitor.rcMonitor.top, monitor.rcMonitor.right - monitor.rcMonitor.left, monitor.rcMonitor.bottom - monitor.rcMonitor.top,
                 SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOACTIVATE);
  } else {
    fullScreen_ = false;
    frame_.setFullScreen(false);
    SetWindowLongPtrW(hwnd, GWL_STYLE, static_cast<LONG_PTR>(savedStyle_));
    SetWindowPlacement(hwnd, placement);
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
  }
  if (onChanged_) onChanged_();
}

bool WindowModes::handle(unsigned message, std::uintptr_t wParam, std::intptr_t /*lParam*/, std::intptr_t& result) {
  if (message != WM_DROPFILES) return false;
  HDROP drop = reinterpret_cast<HDROP>(wParam);
  const UINT count = DragQueryFileW(drop, 0xFFFFFFFF, nullptr, 0);
  std::vector<std::string> paths;
  for (UINT index = 0; index < count; ++index) {
    const UINT length = DragQueryFileW(drop, index, nullptr, 0);
    std::wstring path(length, L'\0');
    if (length > 0 && DragQueryFileW(drop, index, path.data(), length + 1) > 0) paths.push_back(narrow(path));
  }
  DragFinish(drop);
  result = 0;
  if (!paths.empty() && onDropped_) onDropped_(std::move(paths));
  return true;
}

}  // namespace ember::win32::platform
