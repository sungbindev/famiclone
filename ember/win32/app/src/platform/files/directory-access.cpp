/*
 * ------------------------------------------------------------------------
 * 名称：directory-access.cpp
 * 说明：目录的访问检查的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "platform/files/directory-access.hpp"

#include <windows.h>

#include <fstream>
#include <string>
#include <system_error>

namespace ember::win32::platform {

bool canCreateFilesIn(const std::filesystem::path& directory) {
  if (directory.empty()) return false;
  std::error_code error;
  if (!std::filesystem::is_directory(directory, error)) return false;
  const std::filesystem::path probe = directory / (L"ember-write-test-" + std::to_wstring(GetCurrentProcessId()) + L".tmp");
  {
    std::ofstream file(probe, std::ios::binary | std::ios::trunc);
    if (!file) return false;
  }
  std::filesystem::remove(probe, error);
  return true;
}

}  // namespace ember::win32::platform
