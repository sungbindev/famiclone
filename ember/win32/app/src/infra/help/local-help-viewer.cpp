/*
 * ------------------------------------------------------------------------
 * 名称：local-help-viewer.cpp
 * 说明：用本机服务加默认浏览器查看帮助文档的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "infra/help/local-help-viewer.hpp"

#include "shared/help/help-request.hpp"

namespace ember::win32::infra {

namespace {

constexpr const char* kLegacyTitle = "帮助文档";
constexpr const char* kLegacyText = "系统的默认浏览器是 Internet Explorer，它太旧，帮助文档可能显示不正常。建议先安装较新的浏览器（如 Edge、Chrome、Firefox）并设为默认。\n\n仍然用它打开吗？";

}  // namespace

LocalHelpViewer::LocalHelpViewer(const contracts::HelpSite& site, contracts::ExternalLinks& links, contracts::Confirmation& confirmation, std::function<bool()> legacyBrowser)
    : site_(site), links_(links), confirmation_(confirmation), legacyBrowser_(std::move(legacyBrowser)) {}

platform::HttpResponse LocalHelpViewer::respond(const std::string& target) const {
  platform::HttpResponse response;
  const auto path = shared::normalizeHelpPath(target);
  auto content = path ? site_.read(*path) : std::nullopt;
  response.headers = shared::helpSecurityHeaders();
  if (!content) {
    response.status = 404;
    response.contentType = "text/plain; charset=utf-8";
    return response;
  }
  response.contentType = shared::helpContentType(*path);
  response.body = std::move(*content);
  return response;
}

std::string LocalHelpViewer::open(const std::optional<std::string>& page) {
  std::string path;
  if (page) {
    const auto known = shared::helpPagePath(*page);
    if (!known) return "不认识的页面：" + *page;
    path = *known;
  }
  if (!browserChecked_) {
    browserChecked_ = true;
    if (legacyBrowser_ && legacyBrowser_() && !confirmation_.ask(kLegacyTitle, kLegacyText)) return {};
  }
  if (!server_) {
    auto server = std::make_unique<platform::LocalHttpServer>([this](const std::string& target) { return respond(target); });
    if (!server->start()) return "无法在本机启动帮助文档的服务";
    server_ = std::move(server);
  }
  return links_.open("http://127.0.0.1:" + std::to_string(server_->port()) + "/" + path);
}

}  // namespace ember::win32::infra
