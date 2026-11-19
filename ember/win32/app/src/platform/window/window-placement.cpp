/*
 * ------------------------------------------------------------------------
 * 名称：window-placement.cpp
 * 说明：窗口初始位置与大小的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "platform/window/window-placement.hpp"

#include <windows.h>

namespace ember::win32::platform {

void placeWindow(void* window, int width, int height, const std::optional<shared::WindowPlacement>& saved) {
  HWND hwnd = static_cast<HWND>(window);
  RECT target{0, 0, width, height};
  bool visible = false;
  if (saved) {
    target = RECT{saved->x, saved->y, saved->x + width, saved->y + height};
    // 保存的位置要落在某个显示器上；显示器拔掉或分辨率变了以后可能已经不在屏幕里
    visible = MonitorFromRect(&target, MONITOR_DEFAULTTONULL) != nullptr;
  }
  if (!visible) {
    RECT area{};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &area, 0);
    const int x = area.left + ((area.right - area.left) - width) / 2;
    const int y = area.top + ((area.bottom - area.top) - height) / 2;
    target = RECT{x, y, x + width, y + height};
  }
  SetWindowPos(hwnd, nullptr, target.left, target.top, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
}

}  // namespace ember::win32::platform
