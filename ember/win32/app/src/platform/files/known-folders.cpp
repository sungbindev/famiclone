/*
 * ------------------------------------------------------------------------
 * 名称：known-folders.cpp
 * 说明：系统与程序自己的目录的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "platform/files/known-folders.hpp"

#include <windows.h>
#include <knownfolders.h>
#include <shlobj.h>

namespace ember::win32::platform {

std::filesystem::path roamingAppDataDirectory() {
  PWSTR raw = nullptr;
  if (FAILED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_DEFAULT, nullptr, &raw))) {
    CoTaskMemFree(raw);
    return {};
  }
  std::filesystem::path result(raw);
  CoTaskMemFree(raw);
  return result;
}

std::filesystem::path defaultUserDataDirectory() {
  const auto roaming = roamingAppDataDirectory();
  return roaming.empty() ? std::filesystem::path{} : roaming / L"Ember";
}

std::filesystem::path executableDirectory() {
  std::wstring buffer(MAX_PATH, L'\0');
  for (;;) {
    const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length == 0) return {};
    if (length < buffer.size()) {
      buffer.resize(length);
      break;
    }
    buffer.resize(buffer.size() * 2);
  }
  return std::filesystem::path(buffer).parent_path();
}

}  // namespace ember::win32::platform
