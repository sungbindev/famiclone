/*
 * ------------------------------------------------------------------------
 * 名称：probe-support.cpp
 * 说明：平台探针的公共支持：不显示的窗口、字符串转换、HRESULT 格式化。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：窗口只用来给 Direct2D 与 Direct3D 提供目标，创建后不显示。
 * ------------------------------------------------------------------------
 */

#include "probe-check.hpp"

#include <cstdio>

namespace ember::win32::probe {

namespace {

/** 探针窗口的类名。 */
constexpr wchar_t WINDOW_CLASS_NAME[] = L"EmberProbeWindow";

}  // namespace

HWND createProbeWindow(unsigned width, unsigned height) {
  const HINSTANCE instance = GetModuleHandleW(nullptr);
  WNDCLASSEXW windowClass{};
  windowClass.cbSize = sizeof(windowClass);
  windowClass.lpfnWndProc = DefWindowProcW;
  windowClass.hInstance = instance;
  windowClass.lpszClassName = WINDOW_CLASS_NAME;
  // 重复注册会失败，窗口类已存在时可以直接使用。
  RegisterClassExW(&windowClass);
  return CreateWindowExW(0, WINDOW_CLASS_NAME, L"Ember probe", WS_OVERLAPPEDWINDOW, 0, 0, static_cast<int>(width), static_cast<int>(height), nullptr, nullptr, instance, nullptr);
}

std::string toUtf8(const std::wstring& text) {
  if (text.empty()) return {};
  const int size = WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
  if (size <= 0) return {};
  std::string result(static_cast<std::size_t>(size), '\0');
  WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), result.data(), size, nullptr, nullptr);
  return result;
}

std::string formatHresult(HRESULT value) {
  char buffer[16]{};
  std::snprintf(buffer, sizeof(buffer), "0x%08lX", static_cast<unsigned long>(value));
  return buffer;
}

}  // namespace ember::win32::probe
