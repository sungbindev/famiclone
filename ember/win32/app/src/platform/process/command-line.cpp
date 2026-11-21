/*
 * ------------------------------------------------------------------------
 * 名称：command-line.cpp
 * 说明：把 Windows 命令行拆成 UTF-8 参数的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "platform/process/command-line.hpp"

#include <windows.h>
#include <shellapi.h>

#include "platform/text/wide-text.hpp"

namespace ember::win32::platform {

std::vector<std::string> splitCommandLine(const std::wstring& commandLine) {
  std::vector<std::string> result;
  int count = 0;
  LPWSTR* arguments = CommandLineToArgvW(commandLine.c_str(), &count);
  if (arguments == nullptr) return result;
  // 第 0 项是程序自己
  for (int index = 1; index < count; ++index) result.push_back(narrow(arguments[index]));
  LocalFree(arguments);
  return result;
}

}  // namespace ember::win32::platform
