/*
 * ------------------------------------------------------------------------
 * 名称：directory-opener.cpp
 * 说明：用系统文件管理器打开目录的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "platform/shell/directory-opener.hpp"

#include <windows.h>
#include <shellapi.h>

#include <system_error>

namespace ember::win32::platform {

std::string openDirectoryInFileManager(const std::filesystem::path& directory) {
  std::error_code error;
  std::filesystem::create_directories(directory, error);
  if (error) return "无法创建目录（错误码 " + std::to_string(error.value()) + "）";
  const auto result = reinterpret_cast<INT_PTR>(ShellExecuteW(nullptr, L"open", directory.c_str(), nullptr, nullptr, SW_SHOWNORMAL));
  // ShellExecute 的返回值大于 32 表示成功
  return result > 32 ? std::string{} : "系统没有打开目录（错误码 " + std::to_string(result) + "）";
}

}  // namespace ember::win32::platform
