/*
 * ------------------------------------------------------------------------
 * 名称：environment.cpp
 * 说明：读取环境变量的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * ------------------------------------------------------------------------
 */

#include "platform/process/environment.hpp"

#include <windows.h>

#include <vector>

#include "platform/text/wide-text.hpp"

namespace ember::win32::platform {

std::optional<std::string> environmentVariable(std::string_view name) {
  const std::wstring wideName = widen(name);
  if (wideName.empty()) return std::nullopt;
  const DWORD needed = GetEnvironmentVariableW(wideName.c_str(), nullptr, 0);
  if (needed == 0) return std::nullopt;
  std::vector<wchar_t> buffer(needed);
  const DWORD length = GetEnvironmentVariableW(wideName.c_str(), buffer.data(), needed);
  if (length == 0 || length >= needed) return std::nullopt;
  std::string value = narrow(std::wstring_view(buffer.data(), length));
  if (value.empty()) return std::nullopt;
  return value;
}

}  // namespace ember::win32::platform
