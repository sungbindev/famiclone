/*
 * ------------------------------------------------------------------------
 * 名称：https-client.cpp
 * 说明：用 WinHTTP 做的同步下载的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "platform/network/https-client.hpp"

#include <windows.h>
#include <winhttp.h>

#include <fstream>
#include <memory>

#include "platform/text/wide-text.hpp"

namespace ember::win32::platform {

namespace {

/** 各阶段的超时（毫秒）。 */
constexpr int kResolveTimeout = 10000;
constexpr int kConnectTimeout = 15000;
constexpr int kSendTimeout = 30000;
constexpr int kReceiveTimeout = 30000;
constexpr const wchar_t* kUserAgent = L"Ember-Updater";
/** Windows 7 默认不启用 TLS 1.2：显式启用 TLS 1.0、1.1、1.2（系统不认识的位会被忽略，设置失败时用系统默认）。 */
constexpr DWORD kSecureProtocols = 0x00000080 | 0x00000200 | 0x00000800;

/** WinHTTP 句柄的自动关闭。 */
struct HandleCloser {
  void operator()(void* handle) const {
    if (handle != nullptr) WinHttpCloseHandle(handle);
  }
};
using Handle = std::unique_ptr<void, HandleCloser>;

/** 错误码转成中文说明。 */
std::string describeError(DWORD code) {
  std::string text;
  switch (code) {
    case ERROR_WINHTTP_TIMEOUT: text = "连接超时"; break;
    case ERROR_WINHTTP_NAME_NOT_RESOLVED: text = "找不到服务器，请检查网络"; break;
    case ERROR_WINHTTP_CANNOT_CONNECT: text = "无法连接服务器，请检查网络"; break;
    case ERROR_WINHTTP_CONNECTION_ERROR: text = "与服务器的连接出错"; break;
    case ERROR_WINHTTP_SECURE_FAILURE: text = "安全连接失败"; break;
    default: text = "网络出错"; break;
  }
  text += "（错误码 " + std::to_string(code) + "）";
  // 干净的 Windows 7 没有 TLS 1.2：安全连接失败或连接被重置时给出提示
  if (code == ERROR_WINHTTP_SECURE_FAILURE || code == ERROR_WINHTTP_CONNECTION_ERROR) text += "。Windows 7 需要安装 KB3140245 更新才能使用 TLS 1.2";
  return text;
}

/** 系统的代理开关是否打开（注册表 HKCU 的 Internet Settings\ProxyEnable）。
 * WinHttpGetIEProxyConfigForCurrentUser 不看这个开关，代理关掉后地址仍会返回，
 * 这里先查开关，关着时忽略残留的代理地址直连，与浏览器的行为一致。 */
bool proxyEnabled() {
  DWORD enabled = 0;
  DWORD size = sizeof(enabled);
  const LSTATUS status = RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings", L"ProxyEnable", RRF_RT_REG_DWORD, nullptr, &enabled, &size);
  return status == ERROR_SUCCESS && enabled != 0;
}

/** 一次请求：连接、发出、收到响应头；成功时 request 可以读正文。 */
struct Request {
  Handle session;
  Handle connection;
  Handle request;
  DWORD status{0};
  std::string error;
};

Request open(const std::string& url) {
  Request result;
  const std::wstring wide = widen(url);
  URL_COMPONENTS parts{};
  parts.dwStructSize = sizeof(parts);
  parts.dwSchemeLength = static_cast<DWORD>(-1);
  parts.dwHostNameLength = static_cast<DWORD>(-1);
  parts.dwUrlPathLength = static_cast<DWORD>(-1);
  parts.dwExtraInfoLength = static_cast<DWORD>(-1);
  if (wide.empty() || WinHttpCrackUrl(wide.c_str(), static_cast<DWORD>(wide.size()), 0, &parts) == FALSE) {
    result.error = "网址不合格";
    return result;
  }
  const std::wstring host(parts.lpszHostName, parts.dwHostNameLength);
  const std::wstring path = std::wstring(parts.lpszUrlPath, parts.dwUrlPathLength) + std::wstring(parts.lpszExtraInfo, parts.dwExtraInfoLength);
  const bool secure = parts.nScheme == INTERNET_SCHEME_HTTPS;

  // 代理：IE 里设置的代理开着就用它，否则直连（开关关着时代理地址可能还有残留，不能走）；
  // 本机回环地址（只用于本机的测试与调试）一律直连
  WINHTTP_CURRENT_USER_IE_PROXY_CONFIG proxyConfig{};
  std::wstring proxy;
  if (host != L"127.0.0.1" && proxyEnabled() && WinHttpGetIEProxyConfigForCurrentUser(&proxyConfig) != FALSE) {
    if (proxyConfig.lpszProxy != nullptr) proxy = proxyConfig.lpszProxy;
    if (proxyConfig.lpszAutoConfigUrl != nullptr) GlobalFree(proxyConfig.lpszAutoConfigUrl);
    if (proxyConfig.lpszProxy != nullptr) GlobalFree(proxyConfig.lpszProxy);
    if (proxyConfig.lpszProxyBypass != nullptr) GlobalFree(proxyConfig.lpszProxyBypass);
  }
  if (!proxy.empty()) result.session.reset(WinHttpOpen(kUserAgent, WINHTTP_ACCESS_TYPE_NAMED_PROXY, proxy.c_str(), WINHTTP_NO_PROXY_BYPASS, 0));
  else result.session.reset(WinHttpOpen(kUserAgent, WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0));
  if (!result.session) {
    result.error = describeError(GetLastError());
    return result;
  }
  WinHttpSetTimeouts(result.session.get(), kResolveTimeout, kConnectTimeout, kSendTimeout, kReceiveTimeout);
  DWORD protocols = kSecureProtocols;
  WinHttpSetOption(result.session.get(), WINHTTP_OPTION_SECURE_PROTOCOLS, &protocols, sizeof(protocols));

  result.connection.reset(WinHttpConnect(result.session.get(), host.c_str(), parts.nPort, 0));
  if (!result.connection) {
    result.error = describeError(GetLastError());
    return result;
  }
  result.request.reset(WinHttpOpenRequest(result.connection.get(), L"GET", path.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, secure ? WINHTTP_FLAG_SECURE : 0));
  if (!result.request) {
    result.error = describeError(GetLastError());
    return result;
  }
  if (WinHttpSendRequest(result.request.get(), WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) == FALSE || WinHttpReceiveResponse(result.request.get(), nullptr) == FALSE) {
    result.error = describeError(GetLastError());
    return result;
  }
  DWORD size = sizeof(result.status);
  if (WinHttpQueryHeaders(result.request.get(), WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &result.status, &size, WINHTTP_NO_HEADER_INDEX) == FALSE) {
    result.error = describeError(GetLastError());
    return result;
  }
  return result;
}

/** 声明的内容长度；没有时为 0。 */
std::uint64_t contentLength(void* request) {
  wchar_t buffer[32] = {};
  DWORD size = sizeof(buffer);
  if (WinHttpQueryHeaders(request, WINHTTP_QUERY_CONTENT_LENGTH, WINHTTP_HEADER_NAME_BY_INDEX, buffer, &size, WINHTTP_NO_HEADER_INDEX) == FALSE) return 0;
  return std::wcstoull(buffer, nullptr, 10);
}

std::string statusError(DWORD status) { return "服务器返回 " + std::to_string(status); }

}  // namespace

bool isAllowedHttpUrl(std::string_view url) { return url.rfind("https://", 0) == 0 || url.rfind("http://127.0.0.1:", 0) == 0 || url.rfind("http://127.0.0.1/", 0) == 0; }

HttpGetResult httpGet(const std::string& url, std::size_t maxBytes) {
  HttpGetResult result;
  if (!isAllowedHttpUrl(url)) {
    result.error = "只允许 https 网址";
    return result;
  }
  Request request = open(url);
  if (!request.error.empty()) {
    result.error = request.error;
    return result;
  }
  result.status = static_cast<int>(request.status);
  if (request.status != 200) {
    result.error = statusError(request.status);
    return result;
  }
  if (contentLength(request.request.get()) > maxBytes) {
    result.error = "内容太大";
    return result;
  }
  for (;;) {
    DWORD available = 0;
    if (WinHttpQueryDataAvailable(request.request.get(), &available) == FALSE) {
      result.error = describeError(GetLastError());
      return result;
    }
    if (available == 0) break;
    if (result.body.size() + available > maxBytes) {
      result.error = "内容太大";
      result.body.clear();
      return result;
    }
    const std::size_t offset = result.body.size();
    result.body.resize(offset + available);
    DWORD read = 0;
    if (WinHttpReadData(request.request.get(), result.body.data() + offset, available, &read) == FALSE) {
      result.error = describeError(GetLastError());
      result.body.clear();
      return result;
    }
    result.body.resize(offset + read);
  }
  result.ok = true;
  return result;
}

HttpDownloadResult httpDownload(const std::string& url, const std::filesystem::path& file, std::uint64_t maxBytes, const std::function<bool()>& cancelled) {
  HttpDownloadResult result;
  if (!isAllowedHttpUrl(url)) {
    result.error = "只允许 https 网址";
    return result;
  }
  Request request = open(url);
  if (!request.error.empty()) {
    result.error = request.error;
    return result;
  }
  if (request.status != 200) {
    result.error = statusError(request.status);
    return result;
  }
  if (contentLength(request.request.get()) > maxBytes) {
    result.error = "文件太大";
    return result;
  }
  std::ofstream stream(file, std::ios::binary | std::ios::trunc);
  if (!stream) {
    result.error = "无法写入下载的文件";
    return result;
  }
  std::vector<char> buffer(64 * 1024);
  std::uint64_t total = 0;
  for (;;) {
    if (cancelled && cancelled()) {
      result.error = "下载被取消";
      return result;
    }
    DWORD read = 0;
    if (WinHttpReadData(request.request.get(), buffer.data(), static_cast<DWORD>(buffer.size()), &read) == FALSE) {
      result.error = describeError(GetLastError());
      return result;
    }
    if (read == 0) break;
    total += read;
    if (total > maxBytes) {
      result.error = "文件太大";
      return result;
    }
    stream.write(buffer.data(), static_cast<std::streamsize>(read));
    if (!stream) {
      result.error = "无法写入下载的文件";
      return result;
    }
  }
  stream.close();
  if (!stream) {
    result.error = "无法写入下载的文件";
    return result;
  }
  result.ok = true;
  return result;
}

}  // namespace ember::win32::platform
