/*
 * ------------------------------------------------------------------------
 * 名称：help-test.cpp
 * 说明：帮助文档的测试：请求规则（路径整理、拒绝不合格的地址、内容类型、页面名）、本机 HTTP 服务（只读、只认本机的 Host、拒绝不合格的请求）、帮助文档查看（启动服务、打开的网址、旧浏览器询问、不认识的页面）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：本机服务的测试用真实的回环连接；不打开任何浏览器。
 * ------------------------------------------------------------------------
 */

#include <winsock2.h>
#include <ws2tcpip.h>

#include <gtest/gtest.h>

#include <stdexcept>

#include "infra/help/archive-help-site.hpp"
#include "infra/help/local-help-viewer.hpp"
#include "infra/resources/zip-archive.hpp"
#include "platform/network/local-http-server.hpp"
#include "shared/help/help-request.hpp"
#include "support/fakes.hpp"
#include "support/zip-builder.hpp"

namespace ember::win32 {
namespace {

/** 向本机端口发一段原始请求，读到连接关闭为止，返回收到的全部内容。 */
std::string roundTrip(int port, const std::string& request) {
  WSADATA data{};
  WSAStartup(MAKEWORD(2, 2), &data);
  SOCKET connection = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_port = htons(static_cast<u_short>(port));
  inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);
  std::string response;
  if (connect(connection, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0) {
    send(connection, request.data(), static_cast<int>(request.size()), 0);
    char buffer[4096];
    for (;;) {
      const int received = recv(connection, buffer, sizeof(buffer), 0);
      if (received <= 0) break;
      response.append(buffer, static_cast<std::size_t>(received));
    }
  }
  closesocket(connection);
  WSACleanup();
  return response;
}

std::string get(int port, const std::string& target, const std::string& host = "") {
  return roundTrip(port, "GET " + target + " HTTP/1.1\r\nHost: " + (host.empty() ? "127.0.0.1:" + std::to_string(port) : host) + "\r\n\r\n");
}

TEST(HelpRequest, NormalizesPathsAndMapsDirectoriesToIndex) {
  EXPECT_EQ(shared::normalizeHelpPath("/"), "index.html");
  EXPECT_EQ(shared::normalizeHelpPath("/index.html"), "index.html");
  EXPECT_EQ(shared::normalizeHelpPath("/manual/changelog.html?x=1#top"), "manual/changelog.html");
  EXPECT_EQ(shared::normalizeHelpPath("/manual/"), "manual/index.html");
  EXPECT_EQ(shared::normalizeHelpPath("//a///b.js"), "a/b.js");
  EXPECT_EQ(shared::normalizeHelpPath("/assets/%E5%B8%AE%E5%8A%A9.js"), "assets/帮助.js");
}

TEST(HelpRequest, RejectsUnsafeAddresses) {
  for (const char* target : {"", "index.html", "/../secret", "/a/../b", "/a/./b", "/a/%2e%2e/b", "/a%2fb", "/a%2Fb", "/a%5cb", "/a\\b", "/c:/x", "/a%00b", "/%zz", "/%2", "/%E5%B8", "/%FF.html"}) {
    EXPECT_FALSE(shared::normalizeHelpPath(target).has_value()) << target;
  }
}

TEST(HelpRequest, ContentTypesAndPages) {
  EXPECT_EQ(shared::helpContentType("a/b.html"), "text/html; charset=utf-8");
  EXPECT_EQ(shared::helpContentType("a.JS"), "text/javascript; charset=utf-8");
  EXPECT_EQ(shared::helpContentType("fonts/a.woff2"), "font/woff2");
  EXPECT_EQ(shared::helpContentType("a.unknown"), "application/octet-stream");
  EXPECT_EQ(shared::helpContentType("dir.v1/file"), "application/octet-stream");
  EXPECT_EQ(shared::helpPagePath("changelog"), "manual/changelog.html");
  EXPECT_FALSE(shared::helpPagePath("other").has_value());
  EXPECT_EQ(shared::helpSecurityHeaders().size(), 2U);
}

TEST(LocalHttpServer, ServesGetAndHeadAndRefusesOtherMethodsAndForeignHosts) {
  platform::LocalHttpServer server([](const std::string& target) {
    platform::HttpResponse response;
    response.contentType = "text/plain";
    const std::string body = "hello " + target;
    response.body.assign(body.begin(), body.end());
    return response;
  });
  ASSERT_TRUE(server.start());
  ASSERT_GT(server.port(), 0);
  const std::string ok = get(server.port(), "/a?b=1");
  EXPECT_NE(ok.find("HTTP/1.1 200 OK"), std::string::npos);
  EXPECT_NE(ok.find("Content-Length: 12"), std::string::npos);
  EXPECT_NE(ok.find("Connection: close"), std::string::npos);
  EXPECT_NE(ok.find("hello /a?b=1"), std::string::npos);
  EXPECT_NE(get(server.port(), "/", "localhost:" + std::to_string(server.port())).find("200 OK"), std::string::npos);

  const std::string head = roundTrip(server.port(), "HEAD /a HTTP/1.1\r\nHost: 127.0.0.1:" + std::to_string(server.port()) + "\r\n\r\n");
  EXPECT_NE(head.find("200 OK"), std::string::npos);
  EXPECT_EQ(head.find("hello"), std::string::npos);

  EXPECT_NE(roundTrip(server.port(), "POST /a HTTP/1.1\r\nHost: 127.0.0.1:" + std::to_string(server.port()) + "\r\n\r\n").find("405"), std::string::npos);
  EXPECT_NE(get(server.port(), "/a", "evil.example.com").find("403"), std::string::npos);
  EXPECT_NE(get(server.port(), "/a", "127.0.0.1:1").find("403"), std::string::npos);
  EXPECT_NE(roundTrip(server.port(), "GET a HTTP/1.1\r\nHost: 127.0.0.1:" + std::to_string(server.port()) + "\r\n\r\n").find("400"), std::string::npos);
}

TEST(LocalHttpServer, ListensOnLoopbackOnlyAndStopsCleanly) {
  platform::LocalHttpServer server([](const std::string&) { return platform::HttpResponse{}; });
  EXPECT_EQ(server.port(), 0);
  ASSERT_TRUE(server.start());
  EXPECT_TRUE(server.start());
  const int port = server.port();
  EXPECT_NE(get(port, "/").find("200 OK"), std::string::npos);
  server.stop();
  EXPECT_EQ(server.port(), 0);
  EXPECT_TRUE(get(port, "/").empty());
}

TEST(LocalHttpServer, HandlerExceptionBecomes500) {
  platform::LocalHttpServer server([](const std::string&) -> platform::HttpResponse { throw std::runtime_error("x"); });
  ASSERT_TRUE(server.start());
  EXPECT_NE(get(server.port(), "/").find("500"), std::string::npos);
}

struct ViewerFixture {
  test::FakeExternalLinks links;
  test::FakeConfirmation confirmation;
  bool legacy{false};
  std::vector<unsigned char> bytes{test::makeZip({{"help/index.html", "<h1>首页</h1>"}, {"help/manual/changelog.html", "更新记录"}, {"plugins/a.emap", "P"}})};
  infra::ZipArchive archive{bytes.data(), bytes.size()};
  infra::ArchiveHelpSite site{archive};
  std::unique_ptr<infra::LocalHelpViewer> viewer;

  ViewerFixture() { viewer = std::make_unique<infra::LocalHelpViewer>(site, links, confirmation, [this] { return legacy; }); }
};

TEST(LocalHelpViewer, StartsTheServerOnceAndOpensTheHomeOrAFixedPage) {
  ViewerFixture f;
  EXPECT_EQ(f.viewer->port(), 0);
  EXPECT_EQ(f.viewer->open(std::nullopt), "");
  const int port = f.viewer->port();
  ASSERT_GT(port, 0);
  EXPECT_EQ(f.viewer->open(std::string("changelog")), "");
  EXPECT_EQ(f.viewer->port(), port);
  ASSERT_EQ(f.links.opened.size(), 2U);
  EXPECT_EQ(f.links.opened[0], "http://127.0.0.1:" + std::to_string(port) + "/");
  EXPECT_EQ(f.links.opened[1], "http://127.0.0.1:" + std::to_string(port) + "/manual/changelog.html");
  // 服务真的在提供站点
  const std::string home = get(port, "/");
  EXPECT_NE(home.find("200 OK"), std::string::npos);
  EXPECT_NE(home.find("text/html; charset=utf-8"), std::string::npos);
  EXPECT_NE(home.find("nosniff"), std::string::npos);
  EXPECT_NE(home.find("Content-Security-Policy"), std::string::npos);
  EXPECT_NE(home.find("首页"), std::string::npos);
  EXPECT_NE(get(port, "/missing.html").find("404"), std::string::npos);
  EXPECT_NE(get(port, "/../x").find("404"), std::string::npos);
}

TEST(LocalHelpViewer, UnknownPageIsAnErrorAndOpenFailuresAreReturned) {
  ViewerFixture f;
  EXPECT_NE(f.viewer->open(std::string("secret")), "");
  EXPECT_TRUE(f.links.opened.empty());
  f.links.error = "没有浏览器";
  EXPECT_EQ(f.viewer->open(std::nullopt), "没有浏览器");
}

TEST(LocalHelpViewer, LegacyBrowserIsAskedOncePerRunAndDecliningOpensNothing) {
  ViewerFixture f;
  f.legacy = true;
  f.confirmation.answer = false;
  EXPECT_EQ(f.viewer->open(std::nullopt), "");
  EXPECT_EQ(f.confirmation.asked.size(), 1U);
  EXPECT_TRUE(f.links.opened.empty());
  // 只问一次：之后直接打开
  EXPECT_EQ(f.viewer->open(std::nullopt), "");
  EXPECT_EQ(f.confirmation.asked.size(), 1U);
  EXPECT_EQ(f.links.opened.size(), 1U);
}

TEST(LocalHelpViewer, AcceptingTheLegacyWarningOpensAtOnce) {
  ViewerFixture f;
  f.legacy = true;
  f.confirmation.answer = true;
  EXPECT_EQ(f.viewer->open(std::nullopt), "");
  EXPECT_EQ(f.links.opened.size(), 1U);
}

}  // namespace
}  // namespace ember::win32
