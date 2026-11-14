/*
 * ------------------------------------------------------------------------
 * 名称：debug-test-client.hpp
 * 说明：测试用的调试服务客户端：连接本机端口，完成 WebSocket 握手，发送带掩码的文本帧，接收服务端的帧（文本、二进制、关闭、ping），并提供按 JSON-RPC 调用的辅助函数。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：只用于测试，不做完整的 WebSocket 客户端；服务端发来的帧不带掩码，这里自己解析。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "platform/network/tcp-socket.hpp"
#include "platform/network/websocket.hpp"

namespace ember::test {

/** 服务端发来的一个帧。 */
struct ServerFrame {
  platform::websocket::Opcode opcode{platform::websocket::Opcode::Text};
  std::vector<std::uint8_t> payload;

  [[nodiscard]] std::string text() const { return std::string(payload.begin(), payload.end()); }
  /** 关闭帧里的关闭码。 */
  [[nodiscard]] std::uint16_t closeCode() const { return payload.size() >= 2 ? static_cast<std::uint16_t>((payload[0] << 8) | payload[1]) : 0; }
};

/** 握手的结果。 */
struct HandshakeResult {
  int status{0};
  std::string response;
};

/** 测试用的调试服务客户端。 */
class DebugTestClient {
 public:
  /** 连接并握手；握手失败时 handshake().status 不是 101，之后不能再收发。 */
  static std::unique_ptr<DebugTestClient> connect(std::uint16_t port, const std::string& origin = "", const std::string& hostHeader = "") {
    std::string error;
    auto socket = platform::TcpSocket::connectLoopback(port, error);
    if (!socket) return nullptr;
    auto client = std::unique_ptr<DebugTestClient>(new DebugTestClient(std::move(*socket)));
    client->performHandshake(port, origin, hostHeader);
    return client;
  }

  [[nodiscard]] const HandshakeResult& handshake() const { return handshake_; }

  /** 发一个带掩码的文本帧。 */
  bool sendText(const std::string& text) {
    return sendFrame(platform::websocket::Opcode::Text, std::span(reinterpret_cast<const std::uint8_t*>(text.data()), text.size()));
  }

  bool sendFrame(platform::websocket::Opcode opcode, std::span<const std::uint8_t> payload) {
    const auto frame = platform::websocket::encodeFrame(opcode, payload, std::array<std::uint8_t, 4>{0x12, 0x34, 0x56, 0x78});
    return socket_.writeAll(frame.data(), frame.size());
  }

  /** 发送原始字节（构造不合格的帧用）。 */
  bool sendRaw(const std::vector<std::uint8_t>& bytes) { return socket_.writeAll(bytes.data(), bytes.size()); }

  /** 等一个服务端的帧；超时或连接已关闭返回空。 */
  std::optional<ServerFrame> receive(int timeoutMs = 3000) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    for (;;) {
      if (auto frame = parseFrame()) return frame;
      const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now()).count();
      if (remaining <= 0) return std::nullopt;
      std::uint8_t buffer[4096];
      const auto read = socket_.read(buffer, sizeof(buffer), static_cast<int>(std::min<long long>(remaining, 200)));
      if (read.kind == platform::TcpSocket::ReadResult::Kind::Data) {
        received_.insert(received_.end(), buffer, buffer + read.size);
      } else if (read.kind != platform::TcpSocket::ReadResult::Kind::Timeout) {
        return std::nullopt;
      }
    }
  }

  /** 等下一个文本帧并解析成 JSON；跳过 ping。 */
  std::optional<nlohmann::json> receiveJson(int timeoutMs = 3000) {
    for (;;) {
      auto frame = receive(timeoutMs);
      if (!frame) return std::nullopt;
      if (frame->opcode == platform::websocket::Opcode::Ping) continue;
      if (frame->opcode != platform::websocket::Opcode::Text) return std::nullopt;
      return nlohmann::json::parse(frame->text());
    }
  }

  /** 发一个请求并等它的响应；中间收到的通知（没有 id）放进 notifications。 */
  nlohmann::json call(const std::string& method, const nlohmann::json& params = nlohmann::json::object(), int id = 1) {
    const nlohmann::json request{{"jsonrpc", "2.0"}, {"id", id}, {"method", method}, {"params", params}};
    sendText(request.dump());
    for (;;) {
      const auto message = receiveJson();
      if (!message) return nlohmann::json{{"timeout", true}};
      if (message->contains("id") && (*message)["id"] == id) return *message;
      notifications.push_back(*message);
    }
  }

  /** 握手：用正确的令牌发 debug.hello。 */
  nlohmann::json hello(const std::string& token, const std::string& client = "test") { return call("debug.hello", {{"client", client}, {"token", token}}, 100); }

  /** 收到过的通知。 */
  std::vector<nlohmann::json> notifications;

 private:
  explicit DebugTestClient(platform::TcpSocket socket) : socket_(std::move(socket)) {}

  void performHandshake(std::uint16_t port, const std::string& origin, const std::string& hostHeader) {
    const std::string host = hostHeader.empty() ? "127.0.0.1:" + std::to_string(port) : hostHeader;
    std::string request = "GET / HTTP/1.1\r\nHost: " + host + "\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nSec-WebSocket-Version: 13\r\n";
    if (!origin.empty()) request += "Origin: " + origin + "\r\n";
    request += "\r\n";
    static_cast<void>(socket_.writeAll(reinterpret_cast<const std::uint8_t*>(request.data()), request.size()));
    std::string response;
    while (response.find("\r\n\r\n") == std::string::npos) {
      std::uint8_t buffer[1024];
      const auto read = socket_.read(buffer, sizeof(buffer), 3000);
      if (read.kind != platform::TcpSocket::ReadResult::Kind::Data) break;
      response.append(reinterpret_cast<const char*>(buffer), read.size);
    }
    const auto end = response.find("\r\n\r\n");
    handshake_.response = response;
    handshake_.status = response.size() > 12 ? std::atoi(response.c_str() + 9) : 0;
    if (end != std::string::npos) received_.assign(response.begin() + static_cast<std::ptrdiff_t>(end + 4), response.end());
    // 响应体（错误响应带正文）不是 WebSocket 帧，清掉。
    if (handshake_.status != 101) received_.clear();
  }

  std::optional<ServerFrame> parseFrame() {
    if (received_.size() < 2) return std::nullopt;
    std::uint64_t length = received_[1] & 0x7F;
    std::size_t header = 2;
    if (length == 126) {
      if (received_.size() < 4) return std::nullopt;
      length = (static_cast<std::uint64_t>(received_[2]) << 8) | received_[3];
      header = 4;
    } else if (length == 127) {
      if (received_.size() < 10) return std::nullopt;
      length = 0;
      for (std::size_t i = 0; i < 8; ++i) length = (length << 8) | received_[2 + i];
      header = 10;
    }
    if (received_.size() < header + length) return std::nullopt;
    ServerFrame frame;
    frame.opcode = static_cast<platform::websocket::Opcode>(received_[0] & 0x0F);
    frame.payload.assign(received_.begin() + static_cast<std::ptrdiff_t>(header), received_.begin() + static_cast<std::ptrdiff_t>(header + length));
    received_.erase(received_.begin(), received_.begin() + static_cast<std::ptrdiff_t>(header + length));
    return frame;
  }

  platform::TcpSocket socket_;
  HandshakeResult handshake_;
  std::vector<std::uint8_t> received_;
};

}  // namespace ember::test
