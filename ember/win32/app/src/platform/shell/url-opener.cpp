/*
 * ------------------------------------------------------------------------
 * 名称：url-opener.cpp
 * 说明：用系统默认的程序打开网址与判断默认浏览器的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "platform/shell/url-opener.hpp"

#include <windows.h>
#include <shellapi.h>

#include <cwctype>
#include <vector>

#include "platform/text/wide-text.hpp"

namespace ember::win32::platform {

namespace {

/** 读注册表里的一个字符串值；没有时为空。 */
std::wstring readRegistryString(HKEY root, const wchar_t* subKey, const wchar_t* valueName) {
  DWORD size = 0;
  if (RegGetValueW(root, subKey, valueName, RRF_RT_REG_SZ, nullptr, nullptr, &size) != ERROR_SUCCESS || size < sizeof(wchar_t)) return {};
  std::vector<wchar_t> buffer(size / sizeof(wchar_t) + 1, L'\0');
  if (RegGetValueW(root, subKey, valueName, RRF_RT_REG_SZ, nullptr, buffer.data(), &size) != ERROR_SUCCESS) return {};
  return std::wstring(buffer.data());
}

std::wstring lowerCase(std::wstring text) {
  for (wchar_t& c : text) c = static_cast<wchar_t>(std::towlower(c));
  return text;
}

}  // namespace

std::string openUrlInBrowser(std::string_view url) {
  if (url.rfind("http://", 0) != 0 && url.rfind("https://", 0) != 0) return "只能打开 http 或 https 开头的网址";
  const std::wstring wide = widen(url);
  if (wide.empty()) return "网址不合格";
  const auto result = reinterpret_cast<INT_PTR>(ShellExecuteW(nullptr, L"open", wide.c_str(), nullptr, nullptr, SW_SHOWNORMAL));
  // ShellExecute 的返回值大于 32 表示成功
  return result > 32 ? std::string{} : "系统没有打开浏览器（错误码 " + std::to_string(result) + "）";
}

bool defaultBrowserIsInternetExplorer() {
  const std::wstring choice = lowerCase(readRegistryString(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\Shell\\Associations\\UrlAssociations\\http\\UserChoice", L"ProgId"));
  if (!choice.empty()) return choice.find(L"ie.http") != std::wstring::npos;
  // 没有用户选择时（旧系统）看系统登记的 http 打开命令
  return lowerCase(readRegistryString(HKEY_CLASSES_ROOT, L"http\\shell\\open\\command", nullptr)).find(L"iexplore") != std::wstring::npos;
}

}  // namespace ember::win32::platform
