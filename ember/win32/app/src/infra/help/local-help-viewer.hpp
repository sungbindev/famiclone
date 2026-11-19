/*
 * ------------------------------------------------------------------------
 * 名称：local-help-viewer.hpp
 * 说明：帮助文档的查看：第一次打开时在本机回环地址的随机端口启动只读的 HTTP 服务（只服务帮助文档站点），再用系统默认浏览器打开站点首页或固定的页面。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：Windows 7 上默认浏览器可能是 Internet Explorer，站点需要较新的浏览器，所以每次运行第一次打开前提示一次，用户可以选择仍然打开；
 *       服务随本对象销毁而停止；站点内容的读取、浏览器的打开与旧浏览器的判断都经抽象注入，便于测试。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <memory>

#include "contracts/shell/confirmation.hpp"
#include "contracts/system/external-links.hpp"
#include "contracts/system/help-site.hpp"
#include "contracts/system/help-viewer.hpp"
#include "platform/network/local-http-server.hpp"

namespace ember::win32::infra {

/** 用本机服务加默认浏览器查看帮助文档。 */
class LocalHelpViewer final : public contracts::HelpViewer {
 public:
  /**
   * @param site 帮助文档站点。
   * @param links 打开网址。
   * @param confirmation 旧浏览器的询问。
   * @param legacyBrowser 系统默认的浏览器是不是不能运行站点的旧浏览器。
   */
  LocalHelpViewer(const contracts::HelpSite& site, contracts::ExternalLinks& links, contracts::Confirmation& confirmation, std::function<bool()> legacyBrowser);

  [[nodiscard]] std::string open(const std::optional<std::string>& page) override;

  /** 本机服务的端口；服务还没有启动时为 0（给测试读）。 */
  [[nodiscard]] int port() const { return server_ ? server_->port() : 0; }

 private:
  [[nodiscard]] platform::HttpResponse respond(const std::string& target) const;

  const contracts::HelpSite& site_;
  contracts::ExternalLinks& links_;
  contracts::Confirmation& confirmation_;
  std::function<bool()> legacyBrowser_;
  bool browserChecked_{false};
  std::unique_ptr<platform::LocalHttpServer> server_;
};

}  // namespace ember::win32::infra
