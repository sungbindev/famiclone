/*
 * ------------------------------------------------------------------------
 * 名称：winhttp-update-transport.cpp
 * 说明：用 WinHTTP 做的升级网络传输的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "infra/updates/winhttp-update-transport.hpp"

#include "platform/network/https-client.hpp"

namespace ember::win32::infra {

contracts::TransportGetResult WinHttpUpdateTransport::get(const std::string& url, std::size_t maxBytes) {
  platform::HttpGetResult result = platform::httpGet(url, maxBytes);
  return contracts::TransportGetResult{result.ok, std::move(result.error), std::move(result.body)};
}

std::string WinHttpUpdateTransport::download(const std::string& url, const std::filesystem::path& file, std::uint64_t maxBytes, const std::function<bool()>& cancelled) {
  const platform::HttpDownloadResult result = platform::httpDownload(url, file, maxBytes, cancelled);
  return result.ok ? std::string{} : result.error;
}

}  // namespace ember::win32::infra
