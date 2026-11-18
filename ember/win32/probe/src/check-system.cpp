/*
 * ------------------------------------------------------------------------
 * 名称：check-system.cpp
 * 说明：系统检查：记录 Windows 版本与进程位数。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：版本用 ntdll 的 RtlGetVersion 读取，不受应用清单里声明的支持系统影响；原生外壳只支持 64 位进程。
 * ------------------------------------------------------------------------
 */

#include "probe-check.hpp"

#include <cstdio>

namespace ember::win32::probe {

namespace {

/** RtlGetVersion 的函数类型。 */
using RtlGetVersionFunction = LONG(WINAPI*)(OSVERSIONINFOW*);

}  // namespace

void checkSystem(CheckResults& results) {
  OSVERSIONINFOW version{};
  version.dwOSVersionInfoSize = sizeof(version);
  const HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
  const auto getVersion = ntdll == nullptr ? nullptr : reinterpret_cast<RtlGetVersionFunction>(reinterpret_cast<void*>(GetProcAddress(ntdll, "RtlGetVersion")));
  if (getVersion == nullptr || getVersion(&version) != 0) {
    results.push_back({"系统版本", CheckLevel::Fail, "读不到系统版本"});
    return;
  }
  char text[160]{};
  std::snprintf(text, sizeof(text), "Windows %lu.%lu 版本号 %lu %s", version.dwMajorVersion, version.dwMinorVersion, version.dwBuildNumber, toUtf8(version.szCSDVersion).c_str());
  const bool supported = version.dwMajorVersion > 6 || (version.dwMajorVersion == 6 && version.dwMinorVersion >= 1);
  results.push_back({"系统版本", supported ? CheckLevel::Pass : CheckLevel::Fail, text});
  results.push_back({"进程位数", sizeof(void*) == 8 ? CheckLevel::Pass : CheckLevel::Fail, sizeof(void*) == 8 ? "64 位" : "不是 64 位进程"});
}

}  // namespace ember::win32::probe
