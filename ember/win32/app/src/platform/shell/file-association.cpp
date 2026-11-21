/*
 * ------------------------------------------------------------------------
 * 名称：file-association.cpp
 * 说明：登记 .nes 文件打开方式的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "platform/shell/file-association.hpp"

#include <windows.h>
#include <shlobj.h>

#include <vector>

namespace ember::win32::platform {

namespace {

constexpr wchar_t kProgId[] = L"Ember.Cartridge";
constexpr wchar_t kFriendlyName[] = L"NES 卡带";
constexpr wchar_t kExtensionKey[] = L"Software\\Classes\\.nes";
constexpr wchar_t kOpenWithKey[] = L"Software\\Classes\\.nes\\OpenWithProgids";
constexpr wchar_t kProgIdKey[] = L"Software\\Classes\\Ember.Cartridge";
constexpr wchar_t kIconKey[] = L"Software\\Classes\\Ember.Cartridge\\DefaultIcon";
constexpr wchar_t kCommandKey[] = L"Software\\Classes\\Ember.Cartridge\\shell\\open\\command";

/** 读注册表里的一个字符串值（valueName 为空读默认值）；没有时为空。 */
std::wstring readString(HKEY root, const wchar_t* subKey, const wchar_t* valueName) {
  DWORD size = 0;
  if (RegGetValueW(root, subKey, valueName, RRF_RT_REG_SZ, nullptr, nullptr, &size) != ERROR_SUCCESS || size < sizeof(wchar_t)) return {};
  std::vector<wchar_t> buffer(size / sizeof(wchar_t) + 1, L'\0');
  if (RegGetValueW(root, subKey, valueName, RRF_RT_REG_SZ, nullptr, buffer.data(), &size) != ERROR_SUCCESS) return {};
  return std::wstring(buffer.data());
}

/** 当前用户下的一个键里写字符串值（valueName 为空写默认值）；值已经相同时不写。失败时给出错误码。 */
LONG writeString(const wchar_t* subKey, const wchar_t* valueName, const std::wstring& value, bool& changed) {
  if (readString(HKEY_CURRENT_USER, subKey, valueName) == value) return ERROR_SUCCESS;
  HKEY key = nullptr;
  LONG status = RegCreateKeyExW(HKEY_CURRENT_USER, subKey, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr);
  if (status != ERROR_SUCCESS) return status;
  status = RegSetValueExW(key, valueName, 0, REG_SZ, reinterpret_cast<const BYTE*>(value.c_str()), static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t)));
  RegCloseKey(key);
  if (status == ERROR_SUCCESS) changed = true;
  return status;
}

/** 在 .nes 的 OpenWithProgids 里加上本程序的类型（空值，没有时才写）。 */
LONG addOpenWithProgId(bool& changed) {
  HKEY key = nullptr;
  LONG status = RegCreateKeyExW(HKEY_CURRENT_USER, kOpenWithKey, 0, nullptr, 0, KEY_QUERY_VALUE | KEY_SET_VALUE, nullptr, &key, nullptr);
  if (status != ERROR_SUCCESS) return status;
  if (RegQueryValueExW(key, kProgId, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) {
    status = RegSetValueExW(key, kProgId, 0, REG_NONE, nullptr, 0);
    if (status == ERROR_SUCCESS) changed = true;
  }
  RegCloseKey(key);
  return status;
}

}  // namespace

std::string registerCartridgeAssociation(const std::filesystem::path& executable) {
  if (executable.empty()) return "找不到程序自己的路径";
  const std::wstring path = executable.native();
  bool changed = false;
  LONG status = writeString(kProgIdKey, nullptr, kFriendlyName, changed);
  if (status == ERROR_SUCCESS) status = writeString(kIconKey, nullptr, L"\"" + path + L"\",0", changed);
  if (status == ERROR_SUCCESS) status = writeString(kCommandKey, nullptr, L"\"" + path + L"\" \"%1\"", changed);
  if (status == ERROR_SUCCESS) status = addOpenWithProgId(changed);
  // .nes 还没有默认程序时才把本程序设为默认；别的程序或用户已经选定的，不去改
  if (status == ERROR_SUCCESS && readString(HKEY_CLASSES_ROOT, L".nes", nullptr).empty()) status = writeString(kExtensionKey, nullptr, kProgId, changed);
  if (status != ERROR_SUCCESS) return "登记 .nes 文件的打开方式失败（错误码 " + std::to_string(status) + "）";
  if (changed) SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
  return {};
}

}  // namespace ember::win32::platform
