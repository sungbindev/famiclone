/*
 * ------------------------------------------------------------------------
 * 名称：debug-server-test.cpp
 * 说明：调试服务器的测试（真实的本机套接字）：握手与来源检查、令牌与超时、请求与响应、二进制帧、事件订阅与有界队列、多连接、关闭与停止、令牌更换、端点文件、端口占用、协议违规的关闭码。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <nlohmann/json.hpp>

#include "../support/debug-test-client.hpp"
#include "../support/plugin-test-support.hpp"
#include "infra/debug/debug-server.hpp"
#include "shared/errors/core-error.hpp"

namespace {

using ember::infra::debug::DebugServer;
using ember::infra::debug::DebugServerOptions;
using ember::test::DebugTestClient;
namespace ws = ember::platform::websocket;
using nlohmann::json;

constexpr const char* TOKEN = "server-test-token";

/** 一个监听在空闲端口上的服务器，假的请求分发。 */
struct Fixture {
  explicit Fixture(DebugServerOptions options = {}) : errors(std::make_shared<ember::test::RecordingErrorLog>()) {
    options.token = TOKEN;
    options.coreVersion = "9.9.9";
    options.appVersion = "8.8.8";
    server = std::make_unique<DebugServer>(
        std::move(options),
        [this](std::string_view name, const json& params, const ember::platform::CancellationToken& token) -> json {
          if (name == "runtime.status") return json{{"power", "on"}, {"echo", params}};
          if (name == "test.binary") return json{{"format", "raw"}, {"data", json::binary(std::vector<std::uint8_t>(1000, 0xAB))}};
          if (name == "test.slow") {
            slowStarted = true;
            while (!token.isCancelled()) std::this_thread::sleep_for(std::chrono::milliseconds(5));
            token.throwIfCancelled();
          }
          throw ember::shared::CoreError(ember::shared::ERROR_UNKNOWN_REQUEST, "没有登记");
        },
        nullptr, errors);
    std::string error;
    started = server->start(error);
    startError = error;
  }

  std::unique_ptr<DebugTestClient> connect(const std::string& origin = "") const { return DebugTestClient::connect(server->port(), origin); }

  /** 连接并握手成功。 */
  std::unique_ptr<DebugTestClient> login(const std::string& name = "test") const {
    auto client = connect();
    EXPECT_TRUE(client != nullptr && client->handshake().status == 101);
    if (client == nullptr) return nullptr;
    const auto response = client->hello(TOKEN, name);
    EXPECT_TRUE(response.contains("result")) << response.dump();
    return client;
  }

  /** 等条件成立（最多 3 秒）。 */
  template <typename Predicate>
  static bool waitFor(Predicate predicate) {
    for (int i = 0; i < 300; ++i) {
      if (predicate()) return true;
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return predicate();
  }

  std::shared_ptr<ember::test::RecordingErrorLog> errors;
  std::unique_ptr<DebugServer> server;
  bool started{false};
  std::string startError;
  std::atomic<bool> slowStarted{false};
};

/** 读到关闭帧并返回关闭码；没有收到关闭帧返回 0。 */
std::uint16_t expectClose(DebugTestClient& client) {
  for (int i = 0; i < 50; ++i) {
    const auto frame = client.receive(3000);
    if (!frame) return 0;
    if (frame->opcode == ws::Opcode::Close) return frame->closeCode();
  }
  return 0;
}

TEST(DebugServer, ListensOnLoopbackAndAnswersRequests) {
  Fixture fixture;
  ASSERT_TRUE(fixture.started) << fixture.startError;
  EXPECT_TRUE(fixture.server->listening());
  EXPECT_NE(fixture.server->port(), 0);
  auto client = fixture.connect();
  ASSERT_TRUE(client != nullptr);
  EXPECT_EQ(client->handshake().status, 101);
  EXPECT_NE(client->handshake().response.find("Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo="), std::string::npos);

  const auto hello = client->hello(TOKEN, "unit-test");
  EXPECT_EQ(hello["result"]["coreVersion"], "9.9.9");
  EXPECT_EQ(hello["result"]["appVersion"], "8.8.8");
  const auto response = client->call("runtime.status", {{"x", 1}}, 5);
  EXPECT_EQ(response["id"], 5);
  EXPECT_EQ(response["result"]["power"], "on");
  EXPECT_EQ(response["result"]["echo"]["x"], 1);
  EXPECT_EQ(client->call("nobody.home", json::object(), 6)["error"]["code"], -32601);
}

TEST(DebugServer, ServesSeveralRequestsInArrivalOrderOnOneConnection) {
  Fixture fixture;
  ASSERT_TRUE(fixture.started);
  auto client = fixture.login();
  ASSERT_TRUE(client != nullptr);
  for (int id = 1; id <= 20; ++id) client->sendText(json{{"jsonrpc", "2.0"}, {"id", id}, {"method", "runtime.status"}, {"params", {{"n", id}}}}.dump());
  for (int id = 1; id <= 20; ++id) {
    const auto message = client->receiveJson();
    ASSERT_TRUE(message.has_value());
    EXPECT_EQ((*message)["id"], id);
    EXPECT_EQ((*message)["result"]["echo"]["n"], id);
  }
}

TEST(DebugServer, RejectsHostOriginPathAndNonUpgradeRequests) {
  Fixture fixture;
  ASSERT_TRUE(fixture.started);
  const auto port = fixture.server->port();
  EXPECT_EQ(DebugTestClient::connect(port, "https://evil.example")->handshake().status, 403);
  EXPECT_EQ(DebugTestClient::connect(port, "", "evil.example:" + std::to_string(port))->handshake().status, 403);
  EXPECT_EQ(DebugTestClient::connect(port, "http://localhost:3000")->handshake().status, 101);

  std::string error;
  auto socket = ember::platform::TcpSocket::connectLoopback(port, error);
  ASSERT_TRUE(socket.has_value());
  const std::string plain = "GET / HTTP/1.1\r\nHost: 127.0.0.1:" + std::to_string(port) + "\r\n\r\n";
  ASSERT_TRUE(socket->writeAll(reinterpret_cast<const std::uint8_t*>(plain.data()), plain.size()));
  std::uint8_t buffer[512];
  const auto read = socket->read(buffer, sizeof(buffer), 3000);
  ASSERT_EQ(read.kind, ember::platform::TcpSocket::ReadResult::Kind::Data);
  EXPECT_TRUE(std::string(reinterpret_cast<char*>(buffer), read.size).starts_with("HTTP/1.1 426"));

  auto garbage = ember::platform::TcpSocket::connectLoopback(port, error);
  ASSERT_TRUE(garbage.has_value());
  const std::string junk = "not http at all\r\n\r\n";
  ASSERT_TRUE(garbage->writeAll(reinterpret_cast<const std::uint8_t*>(junk.data()), junk.size()));
  const auto reply = garbage->read(buffer, sizeof(buffer), 3000);
  ASSERT_EQ(reply.kind, ember::platform::TcpSocket::ReadResult::Kind::Data);
  EXPECT_TRUE(std::string(reinterpret_cast<char*>(buffer), reply.size).starts_with("HTTP/1.1 400"));
}

TEST(DebugServer, AWrongTokenAnswersThenClosesWith4001) {
  Fixture fixture;
  ASSERT_TRUE(fixture.started);
  auto client = fixture.connect();
  ASSERT_TRUE(client != nullptr);
  const auto response = client->hello("wrong-token");
  EXPECT_EQ(response["error"]["data"]["code"], "debug.unauthorized");
  EXPECT_EQ(expectClose(*client), ws::CLOSE_UNAUTHORIZED);
  EXPECT_TRUE(fixture.server->status().connections.empty());
}

TEST(DebugServer, RequestsBeforeHelloCloseTheConnection) {
  Fixture fixture;
  ASSERT_TRUE(fixture.started);
  auto client = fixture.connect();
  ASSERT_TRUE(client != nullptr);
  const auto response = client->call("runtime.status");
  EXPECT_EQ(response["error"]["data"]["code"], "debug.unauthorized");
  EXPECT_EQ(expectClose(*client), ws::CLOSE_UNAUTHORIZED);
}

TEST(DebugServer, ConnectionsThatNeverSayHelloAreClosedAfterTheTimeout) {
  DebugServerOptions options;
  options.handshakeTimeoutMs = 400;
  Fixture fixture(std::move(options));
  ASSERT_TRUE(fixture.started);
  auto client = fixture.connect();
  ASSERT_TRUE(client != nullptr);
  EXPECT_EQ(client->handshake().status, 101);
  EXPECT_EQ(expectClose(*client), ws::CLOSE_UNAUTHORIZED);
}

TEST(DebugServer, ConnectionsThatNeverFinishTheUpgradeAreDropped) {
  DebugServerOptions options;
  options.handshakeTimeoutMs = 300;
  Fixture fixture(std::move(options));
  ASSERT_TRUE(fixture.started);
  std::string error;
  auto socket = ember::platform::TcpSocket::connectLoopback(fixture.server->port(), error);
  ASSERT_TRUE(socket.has_value());
  std::uint8_t buffer[16];
  // 什么都不发：服务端超时后关闭连接，读到的是 Closed。
  ember::platform::TcpSocket::ReadResult read{ember::platform::TcpSocket::ReadResult::Kind::Timeout, 0};
  for (int i = 0; i < 20 && read.kind == ember::platform::TcpSocket::ReadResult::Kind::Timeout; ++i) read = socket->read(buffer, sizeof(buffer), 200);
  EXPECT_NE(read.kind, ember::platform::TcpSocket::ReadResult::Kind::Timeout);
}

TEST(DebugServer, SendsResultsWithBinaryDataAsBinaryFrames) {
  Fixture fixture;
  ASSERT_TRUE(fixture.started);
  auto client = fixture.login();
  ASSERT_TRUE(client != nullptr);
  client->sendText(json{{"jsonrpc", "2.0"}, {"id", 3}, {"method", "test.binary"}}.dump());
  const auto frame = client->receive();
  ASSERT_TRUE(frame.has_value());
  EXPECT_EQ(frame->opcode, ws::Opcode::Binary);
  const auto& payload = frame->payload;
  const std::uint32_t length = (static_cast<std::uint32_t>(payload[0]) << 24) | (payload[1] << 16) | (payload[2] << 8) | payload[3];
  const auto header = json::parse(std::string(payload.begin() + 4, payload.begin() + 4 + length));
  EXPECT_EQ(header["id"], 3);
  EXPECT_EQ(header["result"]["format"], "raw");
  EXPECT_EQ(payload.size(), 4u + length + 1000u);
  EXPECT_EQ(payload.back(), 0xAB);
}

TEST(DebugServer, PushesOnlySubscribedEvents) {
  Fixture fixture;
  ASSERT_TRUE(fixture.started);
  auto subscriber = fixture.login("subscriber");
  auto bystander = fixture.login("bystander");
  ASSERT_TRUE(subscriber != nullptr && bystander != nullptr);
  EXPECT_EQ(subscriber->call("debug.subscribe", {{"events", json::array({"runtime.*"})}}, 2)["result"]["events"], json::array({"runtime.*"}));

  fixture.server->publishEvent("output.volume", {{"volume", 5}});
  fixture.server->publishEvent("runtime.stateChanged", {{"power", "on"}});
  const auto event = subscriber->receiveJson();
  ASSERT_TRUE(event.has_value());
  EXPECT_EQ((*event)["method"], "runtime.stateChanged");
  EXPECT_EQ((*event)["params"]["power"], "on");
  EXPECT_FALSE(subscriber->receive(200).has_value());
  EXPECT_FALSE(bystander->receive(200).has_value());

  subscriber->call("debug.unsubscribe", {{"events", json::array({"runtime.*"})}}, 3);
  fixture.server->publishEvent("runtime.stateChanged", {{"power", "off"}});
  EXPECT_FALSE(subscriber->receive(200).has_value());
}

TEST(DebugServer, ASlowClientOnlyLosesItsOwnEventsAndIsToldHowMany) {
  DebugServerOptions options;
  options.eventQueueLimit = 4;
  Fixture fixture(std::move(options));
  ASSERT_TRUE(fixture.started);
  auto slow = fixture.login("slow");
  auto fast = fixture.login("fast");
  ASSERT_TRUE(slow != nullptr && fast != nullptr);
  slow->call("debug.subscribe", {{"events", json::array({"*"})}}, 2);
  fast->call("debug.subscribe", {{"events", json::array({"*"})}}, 2);

  // 慢客户端不读：事件大到把套接字缓冲塞满，写线程被阻塞，队列里的事件超过上限后被丢弃。
  const std::string big(512 * 1024, 'x');
  constexpr int TOTAL = 60;
  for (int i = 0; i < TOTAL; ++i) fixture.server->publishEvent("test.big", {{"n", i}, {"blob", big}});

  int received = 0;
  int dropped = 0;
  for (;;) {
    const auto message = slow->receiveJson(1000);
    if (!message) break;
    if ((*message)["method"] == "debug.eventsDropped") dropped += (*message)["params"]["count"].get<int>();
    else if ((*message)["method"] == "test.big") ++received;
  }
  EXPECT_GT(dropped, 0);
  EXPECT_LT(received, TOTAL);
  EXPECT_EQ(received + dropped, TOTAL);
  // 响应永远不会被丢弃。
  EXPECT_EQ(slow->call("runtime.status", json::object(), 77)["result"]["power"], "on");
}

TEST(DebugServer, AnswersPingWithPongAndEchoesTheClientClose) {
  Fixture fixture;
  ASSERT_TRUE(fixture.started);
  auto client = fixture.login();
  ASSERT_TRUE(client != nullptr);
  const std::vector<std::uint8_t> payload{'p', 'i', 'n', 'g'};
  ASSERT_TRUE(client->sendFrame(ws::Opcode::Ping, payload));
  const auto pong = client->receive();
  ASSERT_TRUE(pong.has_value());
  EXPECT_EQ(pong->opcode, ws::Opcode::Pong);
  EXPECT_EQ(pong->payload, payload);

  ASSERT_TRUE(client->sendFrame(ws::Opcode::Close, std::vector<std::uint8_t>{0x03, 0xE8}));
  EXPECT_EQ(expectClose(*client), 1000);
  EXPECT_TRUE(Fixture::waitFor([&] { return fixture.server->status().connections.empty(); }));
}

TEST(DebugServer, ClosesOnProtocolViolations) {
  Fixture fixture;
  ASSERT_TRUE(fixture.started);
  {
    auto client = fixture.login();
    ASSERT_TRUE(client != nullptr);
    ASSERT_TRUE(client->sendFrame(ws::Opcode::Binary, std::vector<std::uint8_t>{1, 2, 3}));
    EXPECT_EQ(expectClose(*client), 1003);
  }
  {
    auto client = fixture.login();
    ASSERT_TRUE(client != nullptr);
    ASSERT_TRUE(client->sendFrame(ws::Opcode::Text, std::vector<std::uint8_t>{0xC3, 0x28}));
    EXPECT_EQ(expectClose(*client), ws::CLOSE_INVALID_PAYLOAD);
  }
  {
    auto client = fixture.login();
    ASSERT_TRUE(client != nullptr);
    ASSERT_TRUE(client->sendRaw({0x81, 0x01, 'a'}));  // 没有掩码
    EXPECT_EQ(expectClose(*client), ws::CLOSE_PROTOCOL_ERROR);
  }
}

TEST(DebugServer, RejectsRequestsLargerThanTheLimit) {
  DebugServerOptions options;
  options.maxRequestBytes = 1024;
  Fixture fixture(std::move(options));
  ASSERT_TRUE(fixture.started);
  auto client = fixture.login();
  ASSERT_TRUE(client != nullptr);
  client->sendText(json{{"jsonrpc", "2.0"}, {"id", 1}, {"method", "runtime.status"}, {"params", {{"pad", std::string(2000, 'a')}}}}.dump());
  EXPECT_EQ(expectClose(*client), ws::CLOSE_TOO_BIG);
}

TEST(DebugServer, ListsAuthenticatedConnectionsInTheStatus) {
  Fixture fixture;
  ASSERT_TRUE(fixture.started);
  std::atomic<int> notified{0};
  fixture.server->setStatusObserver([&notified] { ++notified; });
  auto idle = fixture.connect();  // 只握手没有 hello：不算连接
  ASSERT_TRUE(idle != nullptr);
  auto first = fixture.login("first");
  auto second = fixture.login("second");
  ASSERT_TRUE(first != nullptr && second != nullptr);
  ASSERT_TRUE(Fixture::waitFor([&] { return fixture.server->status().connections.size() == 2; }));
  const auto status = fixture.server->status();
  EXPECT_TRUE(status.listening);
  EXPECT_EQ(status.port, fixture.server->port());
  EXPECT_EQ(status.connections[0].client, "first");
  EXPECT_EQ(status.connections[1].client, "second");
  EXPECT_NE(status.connections[0].id, status.connections[1].id);
  EXPECT_EQ(status.connections[0].connectedAt.size(), 20u);
  EXPECT_GE(notified.load(), 2);

  first.reset();
  ASSERT_TRUE(Fixture::waitFor([&] { return fixture.server->status().connections.size() == 1; }));
  EXPECT_EQ(fixture.server->status().connections[0].client, "second");
  EXPECT_GE(notified.load(), 3);
  // 其他连接不受影响。
  EXPECT_EQ(second->call("runtime.status", json::object(), 9)["result"]["power"], "on");
}

TEST(DebugServer, ChangingTheTokenClosesExistingConnectionsWith4002) {
  Fixture fixture;
  ASSERT_TRUE(fixture.started);
  auto client = fixture.login();
  ASSERT_TRUE(client != nullptr);
  fixture.server->setToken("brand-new-token");
  EXPECT_EQ(expectClose(*client), ws::CLOSE_TOKEN_CHANGED);

  auto old = fixture.connect();
  ASSERT_TRUE(old != nullptr);
  EXPECT_EQ(old->hello(TOKEN)["error"]["data"]["code"], "debug.unauthorized");
  auto fresh = fixture.connect();
  ASSERT_TRUE(fresh != nullptr);
  EXPECT_TRUE(fresh->hello("brand-new-token").contains("result"));
}

TEST(DebugServer, StopSendsCloseCancelsRunningRequestsAndReleasesThePort) {
  Fixture fixture;
  ASSERT_TRUE(fixture.started);
  const auto port = fixture.server->port();
  auto idle = fixture.login("idle");
  auto busy = fixture.login("busy");
  ASSERT_TRUE(idle != nullptr && busy != nullptr);
  busy->sendText(json{{"jsonrpc", "2.0"}, {"id", 1}, {"method", "test.slow"}}.dump());
  ASSERT_TRUE(Fixture::waitFor([&] { return fixture.slowStarted.load(); }));

  const auto started = std::chrono::steady_clock::now();
  fixture.server->stop();
  EXPECT_LT(std::chrono::steady_clock::now() - started, std::chrono::seconds(5));
  EXPECT_FALSE(fixture.server->listening());
  EXPECT_EQ(fixture.server->port(), 0);
  EXPECT_EQ(expectClose(*idle), 1001);
  // 被取消的请求仍然收到了（取消的）响应，然后是关闭帧。
  const auto answer = busy->receiveJson();
  ASSERT_TRUE(answer.has_value());
  EXPECT_EQ((*answer)["error"]["data"]["code"], "core.cancelled");

  std::string error;
  EXPECT_FALSE(ember::platform::TcpSocket::connectLoopback(port, error).has_value());
}

TEST(DebugServer, ReportsAnOccupiedPort) {
  Fixture first;
  ASSERT_TRUE(first.started);
  DebugServerOptions options;
  options.port = first.server->port();
  Fixture second(std::move(options));
  EXPECT_FALSE(second.started);
  EXPECT_NE(second.startError.find("端口"), std::string::npos);
  EXPECT_FALSE(second.server->listening());
}

TEST(DebugServer, RefusesToStartWithoutAToken) {
  DebugServer server(DebugServerOptions{}, nullptr, nullptr, nullptr);
  std::string error;
  EXPECT_FALSE(server.start(error));
  EXPECT_FALSE(error.empty());
}

TEST(DebugServer, WritesTheEndpointFileAndRemovesItOnStop) {
  const auto directory = std::filesystem::temp_directory_path() / "ember-debug-endpoint-test";
  std::filesystem::remove_all(directory);
  DebugServerOptions options;
  options.endpointFile = directory / "run" / "debug-endpoint.json";
  Fixture fixture(std::move(options));
  ASSERT_TRUE(fixture.started);
  const auto file = directory / "run" / "debug-endpoint.json";
  ASSERT_TRUE(std::filesystem::exists(file));
  auto read = [&file] {
    std::ifstream stream(file);
    return json::parse(stream);
  };
  auto content = read();
  EXPECT_EQ(content["url"], "ws://127.0.0.1:" + std::to_string(fixture.server->port()) + "/");
  EXPECT_EQ(content["port"], fixture.server->port());
  EXPECT_EQ(content["token"], TOKEN);
  EXPECT_EQ(content["coreVersion"], "9.9.9");
  EXPECT_TRUE(content["pid"].is_number_integer());

  fixture.server->setToken("rotated");
  EXPECT_EQ(read()["token"], "rotated");
  fixture.server->stop();
  EXPECT_FALSE(std::filesystem::exists(file));
  std::filesystem::remove_all(directory);
}

}  // namespace
