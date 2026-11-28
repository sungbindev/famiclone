/*
 * ------------------------------------------------------------------------
 * 名称：https-client.hpp
 * 说明：用 WinHTTP 做的同步下载：取一个网址的内容到内存（升级清单），或下载到文件（新版本程序）；在工作线程里调用，不要在界面线程调用。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：只允许 https 网址，以及指向本机回环地址 127.0.0.1 的 http（本地演练升级流程用）；Windows 7 默认不启用 TLS 1.2，这里显式启用，系统没装对应更新时连接失败，错误说明里提示；
 *       使用系统（IE）的代理设置，代理开关关着时忽略残留的代理地址直连；有大小上限，超过上限视为失败；下载到临时文件，调用方校验后再改名。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace ember::win32::platform {

/** 取到内存的结果。 */
struct HttpGetResult {
  bool ok{false};
  int status{0};
  /** 失败的原因（中文）。 */
  std::string error;
  std::vector<std::uint8_t> body;
};

/** 下载到文件的结果。 */
struct HttpDownloadResult {
  bool ok{false};
  std::string error;
};

/** 是不是允许访问的网址：https，或指向 127.0.0.1 的 http。 */
[[nodiscard]] bool isAllowedHttpUrl(std::string_view url);

/**
 * 取一个网址的内容到内存。
 * @param url 网址（UTF-8）。
 * @param maxBytes 内容的字节数上限。
 */
[[nodiscard]] HttpGetResult httpGet(const std::string& url, std::size_t maxBytes);

/**
 * 下载一个网址到文件（覆盖已有文件）。
 * @param url 网址（UTF-8）。
 * @param file 目标文件。
 * @param maxBytes 内容的字节数上限。
 * @param cancelled 返回 true 时中止下载（按失败处理）。
 */
[[nodiscard]] HttpDownloadResult httpDownload(const std::string& url, const std::filesystem::path& file, std::uint64_t maxBytes, const std::function<bool()>& cancelled);

}  // namespace ember::win32::platform
