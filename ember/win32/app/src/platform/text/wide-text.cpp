/*
 * ------------------------------------------------------------------------
 * 名称：wide-text.cpp
 * 说明：UTF-8 与 UTF-16 文字转换的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "platform/text/wide-text.hpp"

#include <windows.h>

namespace ember::win32::platform {

std::wstring widen(std::string_view utf8) {
  if (utf8.empty()) return {};
  const int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
  if (size <= 0) return {};
  std::wstring result(static_cast<std::size_t>(size), L'\0');
  MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), static_cast<int>(utf8.size()), result.data(), size);
  return result;
}

std::string narrow(std::wstring_view utf16) {
  if (utf16.empty()) return {};
  const int size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, utf16.data(), static_cast<int>(utf16.size()), nullptr, 0, nullptr, nullptr);
  if (size <= 0) return {};
  std::string result(static_cast<std::size_t>(size), '\0');
  WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, utf16.data(), static_cast<int>(utf16.size()), result.data(), size, nullptr, nullptr);
  return result;
}

std::string pathToUtf8(const std::filesystem::path& path) { return narrow(path.native()); }

std::filesystem::path pathFromUtf8(std::string_view utf8) { return std::filesystem::path(widen(utf8)); }

}  // namespace ember::win32::platform
