/*
 * ------------------------------------------------------------------------
 * 名称：local-http-server.cpp
 * 说明：本机只读 HTTP 服务的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "platform/network/local-http-server.hpp"

#include <winsock2.h>
#include <ws2tcpip.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <system_error>

namespace ember::win32::platform {

namespace {

/** 请求头的最大长度。 */
constexpr std::size_t kMaxHeaderBytes = 16 * 1024;
/** 读请求的超时（毫秒）。 */
constexpr int kReceiveTimeoutMilliseconds = 5000;
/** 发响应的超时（毫秒）。 */
constexpr int kSendTimeoutMilliseconds = 10000;

SOCKET asSocket(std::uintptr_t value) { return static_cast<SOCKET>(value); }

const char* reasonOf(int status) {
  switch (status) {
    case 200: return "OK";
    case 400: return "Bad Request";
    case 403: return "Forbidden";
    case 404: return "Not Found";
    case 405: return "Method Not Allowed";
    case 431: return "Request Header Fields Too Large";
    default: return "Internal Server Error";
  }
}

std::string lowerCase(std::string text) {
  for (char& c : text) {
    if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
  }
  return text;
}

/** 请求头里某个头的值（头名不分大小写）；没有时为空。 */
std::string headerValue(const std::string& headers, const std::string& name) {
  const std::string wanted = lowerCase(name) + ":";
  std::size_t lineStart = headers.find("\r\n");
  while (lineStart != std::string::npos) {
    lineStart += 2;
    const std::size_t lineEnd = headers.find("\r\n", lineStart);
    const std::string line = headers.substr(lineStart, lineEnd == std::string::npos ? std::string::npos : lineEnd - lineStart);
    if (lowerCase(line.substr(0, wanted.size())) == wanted) {
      const std::size_t valueStart = line.find_first_not_of(" \t", wanted.size());
      return valueStart == std::string::npos ? std::string() : line.substr(valueStart, line.find_last_not_of(" \t") - valueStart + 1);
    }
    lineStart = lineEnd;
  }
  return {};
}

/** 把整段数据发完；失败返回 false。 */
bool sendAll(SOCKET connection, const char* data, std::size_t size) {
  while (size > 0) {
    const int sent = send(connection, data, static_cast<int>(std::min<std::size_t>(size, 1 << 20)), 0);
    if (sent <= 0) return false;
    data += sent;
    size -= static_cast<std::size_t>(sent);
  }
  return true;
}

/** 发一个响应；withBody 为假时（HEAD）只发头，Content-Length 仍是正文的长度。 */
void sendResponse(SOCKET connection, const HttpResponse& response, bool withBody) {
  std::string head = "HTTP/1.1 " + std::to_string(response.status) + " " + reasonOf(response.status) + "\r\n";
  head += "Content-Length: " + std::to_string(response.body.size()) + "\r\n";
  if (!response.contentType.empty()) head += "Content-Type: " + response.contentType + "\r\n";
  for (const auto& [name, value] : response.headers) head += name + ": " + value + "\r\n";
  head += "Cache-Control: no-store\r\nConnection: close\r\n\r\n";
  if (!sendAll(connection, head.data(), head.size())) return;
  if (withBody && !response.body.empty()) sendAll(connection, reinterpret_cast<const char*>(response.body.data()), response.body.size());
}

HttpResponse plain(int status) {
  HttpResponse response;
  response.status = status;
  response.contentType = "text/plain; charset=utf-8";
  return response;
}

}  // namespace

LocalHttpServer::LocalHttpServer(Handler handler) : handler_(std::move(handler)) {}

LocalHttpServer::~LocalHttpServer() { stop(); }

bool LocalHttpServer::start() {
  if (port_ != 0) return true;
  WSADATA data{};
  if (WSAStartup(MAKEWORD(2, 2), &data) != 0) return false;
  SOCKET listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (listener == INVALID_SOCKET) {
    WSACleanup();
    return false;
  }
  // 不让别的进程占用同一个端口
  const BOOL exclusive = TRUE;
  setsockopt(listener, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, reinterpret_cast<const char*>(&exclusive), sizeof(exclusive));
  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_port = 0;
  inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);
  int length = sizeof(address);
  if (bind(listener, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0 || listen(listener, SOMAXCONN) != 0 || getsockname(listener, reinterpret_cast<sockaddr*>(&address), &length) != 0) {
    closesocket(listener);
    WSACleanup();
    return false;
  }
  listener_ = static_cast<std::uintptr_t>(listener);
  port_ = ntohs(address.sin_port);
  stopping_ = false;
  acceptor_ = std::thread([this] { acceptLoop(); });
  return true;
}

void LocalHttpServer::stop() {
  if (port_ == 0) return;
  stopping_ = true;
  // 关闭监听套接字让 accept 返回
  const SOCKET listener = asSocket(listener_);
  closesocket(listener);
  if (acceptor_.joinable()) acceptor_.join();
  while (active_ > 0) std::this_thread::sleep_for(std::chrono::milliseconds(5));
  listener_ = ~std::uintptr_t{0};
  port_ = 0;
  WSACleanup();
}

void LocalHttpServer::acceptLoop() {
  const SOCKET listener = asSocket(listener_);
  while (!stopping_) {
    const SOCKET connection = accept(listener, nullptr, nullptr);
    if (connection == INVALID_SOCKET) {
      if (stopping_) return;
      continue;
    }
    ++active_;
    try {
      std::thread([this, connection] {
        serve(static_cast<std::uintptr_t>(connection));
        --active_;
      }).detach();
    } catch (const std::system_error&) {
      closesocket(connection);
      --active_;
    }
  }
}

void LocalHttpServer::serve(std::uintptr_t handle) {
  const SOCKET connection = asSocket(handle);
  const int receiveTimeout = kReceiveTimeoutMilliseconds;
  const int sendTimeout = kSendTimeoutMilliseconds;
  setsockopt(connection, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&receiveTimeout), sizeof(receiveTimeout));
  setsockopt(connection, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&sendTimeout), sizeof(sendTimeout));

  // 读到请求头结束（空行）为止
  std::string request;
  char buffer[4096];
  bool complete = false;
  while (request.size() <= kMaxHeaderBytes) {
    const int received = recv(connection, buffer, sizeof(buffer), 0);
    if (received <= 0) break;
    request.append(buffer, static_cast<std::size_t>(received));
    if (request.find("\r\n\r\n") != std::string::npos) {
      complete = true;
      break;
    }
  }

  if (!complete) {
    sendResponse(connection, plain(request.size() > kMaxHeaderBytes ? 431 : 400), true);
  } else {
    const std::size_t lineEnd = request.find("\r\n");
    const std::string line = request.substr(0, lineEnd);
    const std::size_t firstSpace = line.find(' ');
    const std::size_t secondSpace = firstSpace == std::string::npos ? std::string::npos : line.find(' ', firstSpace + 1);
    const std::string method = firstSpace == std::string::npos ? std::string() : line.substr(0, firstSpace);
    const std::string target = secondSpace == std::string::npos ? std::string() : line.substr(firstSpace + 1, secondSpace - firstSpace - 1);
    const std::string host = lowerCase(headerValue(request.substr(0, request.find("\r\n\r\n")), "Host"));
    const std::string local = "127.0.0.1:" + std::to_string(port_);
    const std::string localName = "localhost:" + std::to_string(port_);
    if (method != "GET" && method != "HEAD") {
      sendResponse(connection, plain(405), true);
    } else if (target.empty() || target[0] != '/') {
      sendResponse(connection, plain(400), true);
    } else if (host != local && host != localName) {
      sendResponse(connection, plain(403), true);
    } else {
      HttpResponse response;
      try {
        response = handler_(target);
      } catch (...) {
        response = plain(500);
      }
      sendResponse(connection, response, method == "GET");
    }
  }
  shutdown(connection, SD_SEND);
  closesocket(connection);
}

}  // namespace ember::win32::platform
