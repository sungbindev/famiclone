/*
 * ------------------------------------------------------------------------
 * 名称：help-site.hpp
 * 说明：帮助文档站点的抽象：按站点里的相对路径读出文件内容。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：站点内容的来源（目录、随程序带的压缩包）由基础设施层实现；读取可能在本机服务的多个工作线程里同时进行，实现必须线程安全。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace ember::win32::contracts {

/** 帮助文档站点。 */
class HelpSite {
 public:
  virtual ~HelpSite() = default;

  /**
   * 读出站点里的一个文件。
   * @param path 相对路径，用 / 分隔，没有开头的 /（已经过请求规则整理，不含上级目录段）。
   * @returns 文件内容；不存在时没有值。
   */
  [[nodiscard]] virtual std::optional<std::vector<std::uint8_t>> read(const std::string& path) const = 0;
};

}  // namespace ember::win32::contracts
