/*
 * ------------------------------------------------------------------------
 * 名称：system-theme.cpp
 * 说明：读取系统明暗主题的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "platform/theme/system-theme.hpp"

#include <windows.h>

namespace ember::win32::platform {

bool systemUsesLightTheme() {
  DWORD value = 1;
  DWORD size = sizeof(value);
  const LSTATUS status = RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", L"AppsUseLightTheme", RRF_RT_REG_DWORD, nullptr, &value, &size);
  return status != ERROR_SUCCESS || value != 0;
}

}  // namespace ember::win32::platform
