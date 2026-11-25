/*
 * ------------------------------------------------------------------------
 * 名称：window-placement.cpp
 * 说明：窗口初始位置与大小的实现：居中到主显示器的工作区。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "platform/window/window-placement.hpp"

#include <windows.h>

namespace ember::win32::platform {

void placeWindow(void* window, int width, int height) {
  HWND hwnd = static_cast<HWND>(window);
  RECT area{};
  SystemParametersInfoW(SPI_GETWORKAREA, 0, &area, 0);
  const int x = area.left + ((area.right - area.left) - width) / 2;
  const int y = area.top + ((area.bottom - area.top) - height) / 2;
  SetWindowPos(hwnd, nullptr, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
}

}  // namespace ember::win32::platform
