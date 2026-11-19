/*
 * ------------------------------------------------------------------------
 * 名称：local-http-server.hpp
 * 说明：只在本机回环地址（127.0.0.1）监听的只读 HTTP 服务：随机端口，只处理 GET 与 HEAD，每个连接处理完一个请求就关闭，请求的内容交给回调决定。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：请求一律不可信：请求头超过 16 KB、读取超时（5 秒）、方法不是 GET 或 HEAD、Host 头不是本机地址与本服务的端口（防止 DNS 重绑定）的请求都直接拒绝；
 *       回调在服务的工作线程里调用，可能同时有多个；析构时停止监听并等全部工作线程结束。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace ember::win32::platform {

/** 一次响应。 */
struct HttpResponse {
  int status{200};
  /** Content-Type 的值，没有正文时可以为空。 */
  std::string contentType;
  /** 要附加的其他响应头。 */
  std::vector<std::pair<std::string, std::string>> headers;
  std::vector<std::uint8_t> body;
};

/** 本机只读 HTTP 服务。 */
class LocalHttpServer {
 public:
  /**
   * 请求处理：在工作线程里调用。
   * @param target 请求行里的目标（路径加查询，未解码）。
   */
  using Handler = std::function<HttpResponse(const std::string& target)>;

  explicit LocalHttpServer(Handler handler);
  ~LocalHttpServer();
  LocalHttpServer(const LocalHttpServer&) = delete;
  LocalHttpServer& operator=(const LocalHttpServer&) = delete;

  /** 开始监听；已经在监听时直接成功。 */
  [[nodiscard]] bool start();
  /** 停止监听并等全部工作线程结束。 */
  void stop();
  /** 监听的端口；没有监听时为 0。 */
  [[nodiscard]] int port() const { return port_; }

 private:
  void acceptLoop();
  void serve(std::uintptr_t connection);

  Handler handler_;
  /** 监听套接字；还没有监听时是 INVALID_SOCKET（全 1）。 */
  std::uintptr_t listener_{~std::uintptr_t{0}};
  int port_{0};
  std::thread acceptor_;
  std::atomic<bool> stopping_{false};
  /** 正在工作的工作线程个数：stop 等它降到 0。 */
  std::atomic<int> active_{0};
};

}  // namespace ember::win32::platform
