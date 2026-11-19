/*
 * ------------------------------------------------------------------------
 * 名称：update-transport.hpp
 * 说明：升级用的网络传输的抽象：取一个网址的内容、把一个网址下载到文件。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：在工作线程里调用；真实实现用 WinHTTP，测试里换成假的。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace ember::win32::contracts {

/** 取到内存的结果。 */
struct TransportGetResult {
  bool ok{false};
  /** 失败的原因（中文）。 */
  std::string error;
  std::vector<std::uint8_t> body;
};

/** 升级用的网络传输。 */
class UpdateTransport {
 public:
  virtual ~UpdateTransport() = default;

  /** 取一个网址的内容。 */
  [[nodiscard]] virtual TransportGetResult get(const std::string& url, std::size_t maxBytes) = 0;
  /**
   * 把一个网址下载到文件。
   * @param cancelled 返回 true 时中止。
   * @returns 成功时为空文字，失败时是原因。
   */
  [[nodiscard]] virtual std::string download(const std::string& url, const std::filesystem::path& file, std::uint64_t maxBytes, const std::function<bool()>& cancelled) = 0;
};

}  // namespace ember::win32::contracts
