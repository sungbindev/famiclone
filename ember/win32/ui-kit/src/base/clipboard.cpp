/*
 * ------------------------------------------------------------------------
 * 名称：clipboard.cpp
 * 说明：系统剪贴板的文字读写实现：CF_UNICODETEXT。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：别的程序可能正占用剪贴板，打开失败时短暂重试几次。
 * ------------------------------------------------------------------------
 */

#include "base/clipboard.hpp"

#include <windows.h>

#include <cstring>

#include "base/unicode.hpp"

namespace rujian::ui {

namespace {

constexpr int kOpenAttempts = 5;
constexpr DWORD kRetryDelayMilliseconds = 10;

/** 剪贴板的所有者窗口：第一次用时创建的隐藏消息窗口。 */
HWND ownerWindow() {
  static HWND window = CreateWindowExW(0, L"STATIC", L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, GetModuleHandleW(nullptr), nullptr);
  return window;
}

bool openClipboard() {
  for (int attempt = 0; attempt < kOpenAttempts; ++attempt) {
    if (OpenClipboard(ownerWindow()) != FALSE) return true;
    Sleep(kRetryDelayMilliseconds);
  }
  return false;
}

}  // namespace

std::optional<std::string> readClipboardText() {
  if (!openClipboard()) return std::nullopt;
  std::optional<std::string> result;
  if (HANDLE handle = GetClipboardData(CF_UNICODETEXT)) {
    if (const auto* text = static_cast<const wchar_t*>(GlobalLock(handle))) {
      result = toUtf8(std::u16string_view(reinterpret_cast<const char16_t*>(text)));
      GlobalUnlock(handle);
    }
  }
  CloseClipboard();
  return result;
}

bool writeClipboardText(std::string_view utf8) {
  const std::u16string wide = toUtf16(utf8);
  const SIZE_T bytes = (wide.size() + 1) * sizeof(char16_t);
  HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, bytes);
  if (memory == nullptr) return false;
  if (void* target = GlobalLock(memory)) {
    memcpy(target, wide.c_str(), bytes);
    GlobalUnlock(memory);
  } else {
    GlobalFree(memory);
    return false;
  }
  if (!openClipboard()) {
    GlobalFree(memory);
    return false;
  }
  EmptyClipboard();
  const bool stored = SetClipboardData(CF_UNICODETEXT, memory) != nullptr;
  // 放进剪贴板之后内存归系统所有；失败时要自己释放
  if (!stored) GlobalFree(memory);
  CloseClipboard();
  return stored;
}

}  // namespace rujian::ui
