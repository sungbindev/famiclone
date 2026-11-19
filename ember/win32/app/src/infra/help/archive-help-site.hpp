/*
 * ------------------------------------------------------------------------
 * 名称：archive-help-site.hpp
 * 说明：来自资源包的帮助文档站点：站点文件在资源包的 help/ 目录下，按相对路径读取。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：相对路径已经过请求规则整理（不含上级目录段）；只读 help/ 下的条目，资源包里的其他条目（插件、许可全文）读不到。
 * ------------------------------------------------------------------------
 */

#pragma once

#include "contracts/resources/resource-archive.hpp"
#include "contracts/system/help-site.hpp"

namespace ember::win32::infra {

/** 来自资源包的帮助文档站点。 */
class ArchiveHelpSite final : public contracts::HelpSite {
 public:
  /** @param archive 资源包，必须比本对象活得久。 */
  explicit ArchiveHelpSite(const contracts::ResourceArchive& archive) : archive_(archive) {}

  [[nodiscard]] std::optional<std::vector<std::uint8_t>> read(const std::string& path) const override { return archive_.read("help/" + path); }

 private:
  const contracts::ResourceArchive& archive_;
};

}  // namespace ember::win32::infra
