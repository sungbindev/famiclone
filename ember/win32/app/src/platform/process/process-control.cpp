/*
 * ------------------------------------------------------------------------
 * 名称：process-control.cpp
 * 说明：进程操作的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "platform/process/process-control.hpp"

#include <windows.h>

#include "platform/text/wide-text.hpp"

namespace ember::win32::platform {

namespace {

/** 按 Windows 命令行规则给一个参数加引号（CommandLineToArgvW 能还原）。 */
std::wstring quoteArgument(const std::wstring& argument) {
  if (!argument.empty() && argument.find_first_of(L" \t\n\v\"") == std::wstring::npos) return argument;
  std::wstring result = L"\"";
  std::size_t backslashes = 0;
  for (const wchar_t c : argument) {
    if (c == L'\\') {
      ++backslashes;
      continue;
    }
    if (c == L'"') {
      result.append(backslashes * 2 + 1, L'\\');
      result += c;
    } else {
      result.append(backslashes, L'\\');
      result += c;
    }
    backslashes = 0;
  }
  result.append(backslashes * 2, L'\\');
  result += L'"';
  return result;
}

}  // namespace

std::uint32_t currentProcessId() { return GetCurrentProcessId(); }

bool waitForProcessExit(std::uint32_t processId, unsigned timeoutMilliseconds) {
  HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, processId);
  // 打不开：进程已经不在（或没有权限，这里按已退出处理，后面的移动会在文件被占用时报错）
  if (process == nullptr) return true;
  const DWORD waited = WaitForSingleObject(process, timeoutMilliseconds);
  CloseHandle(process);
  return waited == WAIT_OBJECT_0;
}

bool startProcess(const std::filesystem::path& executable, const std::vector<std::string>& arguments) {
  std::wstring commandLine = quoteArgument(executable.wstring());
  for (const std::string& argument : arguments) commandLine += L' ' + quoteArgument(widen(argument));
  STARTUPINFOW startup{sizeof(startup)};
  PROCESS_INFORMATION information{};
  // 命令行缓冲区必须可写
  if (CreateProcessW(executable.c_str(), commandLine.data(), nullptr, nullptr, FALSE, 0, nullptr, executable.parent_path().c_str(), &startup, &information) == FALSE) return false;
  CloseHandle(information.hThread);
  CloseHandle(information.hProcess);
  return true;
}

}  // namespace ember::win32::platform
