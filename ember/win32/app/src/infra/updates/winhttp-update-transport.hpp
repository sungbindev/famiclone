/*
 * ------------------------------------------------------------------------
 * 名称：winhttp-update-transport.hpp
 * 说明：用 WinHTTP 做的升级网络传输：取清单、下载新程序。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#pragma once

#include "contracts/system/update-transport.hpp"

namespace ember::win32::infra {

/** 用 WinHTTP 做的升级网络传输。 */
class WinHttpUpdateTransport final : public contracts::UpdateTransport {
 public:
  [[nodiscard]] contracts::TransportGetResult get(const std::string& url, std::size_t maxBytes) override;
  [[nodiscard]] std::string download(const std::string& url, const std::filesystem::path& file, std::uint64_t maxBytes, const std::function<bool()>& cancelled) override;
};

}  // namespace ember::win32::infra
