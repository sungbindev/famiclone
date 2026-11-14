/*
 * ------------------------------------------------------------------------
 * 名称：debug-session-test.cpp
 * 说明：调试连接会话逻辑的测试：JSON-RPC 报文校验（解析错误、非法请求、参数类型、通知、id 原样返回）、握手与令牌（关闭码）、核心错误到 JSON-RPC 错误码的映射、订阅写法与事件过滤、二进制结果与事件的帧格式、状态与接口描述、取消。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "../support/plugin-test-support.hpp"
#include "contracts/debug/debug-error.hpp"
#include "infra/debug/debug-session.hpp"
#include "shared/errors/core-error.hpp"

namespace {

using ember::infra::debug::DebugSession;
using ember::infra::debug::HandleResult;
using ember::infra::debug::Outgoing;
using ember::infra::debug::SessionContext;
namespace ws = ember::platform::websocket;
namespace debug = ember::infra::debug;
using nlohmann::json;

constexpr const char* TOKEN = "secret-token";

/** 一个带假分发函数的会话。 */
struct Rig {
  Rig() : errors(std::make_shared<ember::test::RecordingErrorLog>()) {
    auto context = std::make_shared<SessionContext>();
    context->errors = errors;
    context->coreVersion = "1.2.3";
    context->appVersion = "4.5.6";
    context->token = [] { return std::string(TOKEN); };
    context->status = [] {
      ember::contracts::debug::DebugStatusResult result;
      result.enabled = true;
      result.listening = true;
      result.port = 5688;
      return result;
    };
    context->discovery = std::make_shared<const json>(json{{"openrpc", "1.3.2"}});
    context->dispatch = [this](std::string_view name, const json& params, const ember::platform::CancellationToken& token) -> json {
      calls.emplace_back(std::string(name), params);
      lastTokenCancelled = token.isCancelled();
      if (name == "test.fail") throw ember::shared::CoreError("test.broken", "坏了");
      if (name == "test.invalid") throw ember::shared::SchemaError("参数不对");
      if (name == "test.crash") throw std::runtime_error("内部细节不能泄露");
      if (name == "test.binary") return json{{"format", "png"}, {"data", json::binary({1, 2, 3, 4})}};
      if (name == "test.cancelSelf") {
        session->cancel();
        return json{{"seen", token.isCancelled()}};
      }
      if (name == "runtime.status") return json{{"power", "on"}};
      throw ember::shared::CoreError(ember::shared::ERROR_UNKNOWN_REQUEST, "没有登记：" + std::string(name));
    };
    context_ = context;
    session = std::make_unique<DebugSession>(7, context_);
  }

  /** 发一条请求文本，返回第一条响应的 JSON。 */
  json call(const json& request, HandleResult* resultOut = nullptr) {
    auto result = session->handleText(request.dump());
    json response = result.outgoing.empty() ? json(nullptr) : json::parse(std::string(result.outgoing[0].payload.begin(), result.outgoing[0].payload.end()));
    if (resultOut != nullptr) *resultOut = std::move(result);
    return response;
  }

  json request(const std::string& method, const json& params = json::object(), const json& id = 1) { return json{{"jsonrpc", "2.0"}, {"id", id}, {"method", method}, {"params", params}}; }

  void login() {
    const auto response = call(request("debug.hello", {{"client", "tester"}, {"token", TOKEN}}));
    ASSERT_TRUE(response.contains("result")) << response.dump();
  }

  std::shared_ptr<ember::test::RecordingErrorLog> errors;
  std::shared_ptr<const SessionContext> context_;
  std::unique_ptr<DebugSession> session;
  std::vector<std::pair<std::string, json>> calls;
  bool lastTokenCancelled{false};
};

TEST(DebugSessionProtocol, ReportsParseErrorsAndInvalidRequests) {
  Rig rig;
  auto result = rig.session->handleText("{不是 JSON");
  ASSERT_EQ(result.outgoing.size(), 1u);
  auto response = json::parse(std::string(result.outgoing[0].payload.begin(), result.outgoing[0].payload.end()));
  EXPECT_EQ(response["error"]["code"], debug::RPC_PARSE_ERROR);
  EXPECT_TRUE(response["id"].is_null());
  EXPECT_FALSE(result.closeCode.has_value());

  rig.login();
  EXPECT_EQ(rig.call(json::array({rig.request("runtime.status")}))["error"]["code"], debug::RPC_INVALID_REQUEST);
  EXPECT_EQ(rig.call(json(42))["error"]["code"], debug::RPC_INVALID_REQUEST);
  EXPECT_EQ(rig.call(json{{"id", 1}, {"method", "runtime.status"}})["error"]["code"], debug::RPC_INVALID_REQUEST);
  EXPECT_EQ(rig.call(json{{"jsonrpc", "1.0"}, {"id", 1}, {"method", "runtime.status"}})["error"]["code"], debug::RPC_INVALID_REQUEST);
  EXPECT_EQ(rig.call(json{{"jsonrpc", "2.0"}, {"id", 1}, {"method", 5}})["error"]["code"], debug::RPC_INVALID_REQUEST);
  const auto badId = rig.call(json{{"jsonrpc", "2.0"}, {"id", json::object()}, {"method", "runtime.status"}});
  EXPECT_EQ(badId["error"]["code"], debug::RPC_INVALID_REQUEST);
  EXPECT_TRUE(badId["id"].is_null());
  EXPECT_EQ(rig.call(json{{"jsonrpc", "2.0"}, {"id", 1}, {"method", "runtime.status"}, {"params", json::array()}})["error"]["code"], debug::RPC_INVALID_PARAMS);
  EXPECT_TRUE(rig.calls.empty());
}

TEST(DebugSessionProtocol, EchoesTheRequestIdAndOmitsParams) {
  Rig rig;
  rig.login();
  const auto numeric = rig.call(rig.request("runtime.status", json::object(), 42));
  EXPECT_EQ(numeric["id"], 42);
  EXPECT_EQ(numeric["jsonrpc"], "2.0");
  EXPECT_EQ(numeric["result"]["power"], "on");
  EXPECT_EQ(rig.call(rig.request("runtime.status", json::object(), "abc-1"))["id"], "abc-1");
  const auto noParams = rig.call(json{{"jsonrpc", "2.0"}, {"id", 3}, {"method", "runtime.status"}});
  EXPECT_EQ(noParams["result"]["power"], "on");
  EXPECT_EQ(rig.calls.back().second, json::object());
}

TEST(DebugSessionProtocol, NotificationsRunButGetNoResponse) {
  Rig rig;
  rig.login();
  HandleResult result;
  const auto response = rig.call(json{{"jsonrpc", "2.0"}, {"method", "runtime.status"}}, &result);
  EXPECT_TRUE(result.outgoing.empty());
  EXPECT_TRUE(response.is_null());
  ASSERT_EQ(rig.calls.size(), 1u);
  EXPECT_EQ(rig.calls[0].first, "runtime.status");

  // 通知出错同样没有响应。
  rig.call(json{{"jsonrpc", "2.0"}, {"method", "test.fail"}}, &result);
  EXPECT_TRUE(result.outgoing.empty());
}

TEST(DebugSessionAuth, MustSayHelloFirstAndTheConnectionIsClosedOtherwise) {
  Rig rig;
  HandleResult result;
  const auto response = rig.call(rig.request("runtime.status"), &result);
  EXPECT_EQ(response["error"]["data"]["code"], "debug.unauthorized");
  ASSERT_TRUE(result.closeCode.has_value());
  EXPECT_EQ(*result.closeCode, ws::CLOSE_UNAUTHORIZED);
  EXPECT_TRUE(rig.calls.empty());
  EXPECT_FALSE(rig.session->authenticated());
}

TEST(DebugSessionAuth, AWrongTokenClosesTheConnection) {
  Rig rig;
  HandleResult result;
  const auto response = rig.call(rig.request("debug.hello", {{"client", "x"}, {"token", "wrong"}}), &result);
  EXPECT_EQ(response["error"]["data"]["code"], "debug.unauthorized");
  ASSERT_TRUE(result.closeCode.has_value());
  EXPECT_EQ(*result.closeCode, ws::CLOSE_UNAUTHORIZED);
  EXPECT_FALSE(rig.session->authenticated());
  // 长度相同但内容不同、前缀相同但更短、空令牌都不能通过。
  for (const std::string token : {"secret-tokeX", "secret", "secret-token-and-more"}) {
    Rig other;
    HandleResult other_result;
    other.call(other.request("debug.hello", {{"client", "x"}, {"token", token}}), &other_result);
    EXPECT_TRUE(other_result.closeCode.has_value()) << token;
  }
}

TEST(DebugSessionAuth, HelloWithTheRightTokenOpensTheSession) {
  Rig rig;
  HandleResult result;
  const auto response = rig.call(rig.request("debug.hello", {{"client", "copilot"}, {"token", TOKEN}}), &result);
  EXPECT_FALSE(result.closeCode.has_value());
  EXPECT_EQ(response["result"]["protocol"], 1);
  EXPECT_EQ(response["result"]["coreVersion"], "1.2.3");
  EXPECT_EQ(response["result"]["appVersion"], "4.5.6");
  EXPECT_EQ(response["result"]["session"], 7);
  EXPECT_TRUE(rig.session->authenticated());
  EXPECT_EQ(rig.session->client(), "copilot");
  // 握手后可以调用请求，重复握手没有害处。
  EXPECT_EQ(rig.call(rig.request("runtime.status"))["result"]["power"], "on");
  EXPECT_TRUE(rig.call(rig.request("debug.hello", {{"client", "copilot"}, {"token", TOKEN}})).contains("result"));
}

TEST(DebugSessionAuth, HelloParametersAreValidated) {
  Rig rig;
  EXPECT_EQ(rig.call(rig.request("debug.hello", {{"client", "x"}}))["error"]["code"], debug::RPC_INVALID_PARAMS);
  EXPECT_EQ(rig.call(rig.request("debug.hello", {{"client", ""}, {"token", TOKEN}}))["error"]["code"], debug::RPC_INVALID_PARAMS);
  EXPECT_EQ(rig.call(rig.request("debug.hello", {{"client", "x"}, {"token", TOKEN}, {"protocol", 2}}))["error"]["code"], debug::RPC_INVALID_PARAMS);
  EXPECT_FALSE(rig.session->authenticated());
  EXPECT_TRUE(rig.call(rig.request("debug.hello", {{"client", "x"}, {"token", TOKEN}, {"protocol", 1}})).contains("result"));
}

TEST(DebugSessionErrors, MapsCoreErrorsToJsonRpcCodes) {
  Rig rig;
  rig.login();
  const auto failed = rig.call(rig.request("test.fail"));
  EXPECT_EQ(failed["error"]["code"], debug::RPC_APPLICATION_ERROR);
  EXPECT_EQ(failed["error"]["message"], "坏了");
  EXPECT_EQ(failed["error"]["data"]["code"], "test.broken");

  const auto invalid = rig.call(rig.request("test.invalid"));
  EXPECT_EQ(invalid["error"]["code"], debug::RPC_INVALID_PARAMS);
  EXPECT_EQ(invalid["error"]["data"]["code"], "core.invalidParams");

  const auto unknown = rig.call(rig.request("nobody.home"));
  EXPECT_EQ(unknown["error"]["code"], debug::RPC_METHOD_NOT_FOUND);
  EXPECT_EQ(unknown["error"]["data"]["code"], "core.unknownRequest");
}

TEST(DebugSessionErrors, InternalErrorsAreLoggedAndNotLeaked) {
  Rig rig;
  rig.login();
  const auto response = rig.call(rig.request("test.crash"));
  EXPECT_EQ(response["error"]["code"], debug::RPC_INTERNAL_ERROR);
  EXPECT_EQ(response["error"]["data"]["code"], "core.internal");
  EXPECT_EQ(response["error"].dump().find("内部细节"), std::string::npos);
  ASSERT_EQ(rig.errors->entries.size(), 1u);
  EXPECT_EQ(rig.errors->entries[0].code, "core.internal");
  EXPECT_EQ(rig.errors->entries[0].message, "内部细节不能泄露");
  EXPECT_EQ(rig.errors->entries[0].module, "test");
}

TEST(DebugSessionSubscriptions, ValidatesAndMatchesPatterns) {
  EXPECT_TRUE(debug::subscriptionValid("*"));
  EXPECT_TRUE(debug::subscriptionValid("runtime.*"));
  EXPECT_TRUE(debug::subscriptionValid("runtime.stateChanged"));
  for (const char* bad : {"", "runtime", "Runtime.x", "runtime.", ".x", "a.b.c", "*.x", "runtime.*.x", "run time.x", "runtime.9x", "**"}) EXPECT_FALSE(debug::subscriptionValid(bad)) << bad;

  EXPECT_TRUE(debug::subscriptionMatches("*", "anything.here"));
  EXPECT_TRUE(debug::subscriptionMatches("runtime.*", "runtime.frame"));
  EXPECT_FALSE(debug::subscriptionMatches("runtime.*", "runtimex.frame"));
  EXPECT_FALSE(debug::subscriptionMatches("runtime.*", "output.frame"));
  EXPECT_TRUE(debug::subscriptionMatches("runtime.frame", "runtime.frame"));
  EXPECT_FALSE(debug::subscriptionMatches("runtime.frame", "runtime.frames"));
}

TEST(DebugSessionSubscriptions, EventsOnlyReachSubscribedAuthenticatedSessions) {
  Rig rig;
  const json data{{"power", "on"}};
  EXPECT_FALSE(rig.session->eventMessage("runtime.stateChanged", data).has_value());
  rig.login();
  EXPECT_FALSE(rig.session->eventMessage("runtime.stateChanged", data).has_value());

  const auto subscribed = rig.call(rig.request("debug.subscribe", {{"events", json::array({"runtime.stateChanged", "output.*"})}}));
  EXPECT_EQ(subscribed["result"]["events"], json::array({"output.*", "runtime.stateChanged"}));
  const auto message = rig.session->eventMessage("runtime.stateChanged", data);
  ASSERT_TRUE(message.has_value());
  EXPECT_EQ(message->opcode, ws::Opcode::Text);
  const auto notification = json::parse(std::string(message->payload.begin(), message->payload.end()));
  EXPECT_EQ(notification["jsonrpc"], "2.0");
  EXPECT_EQ(notification["method"], "runtime.stateChanged");
  EXPECT_EQ(notification["params"], data);
  EXPECT_FALSE(notification.contains("id"));
  EXPECT_TRUE(rig.session->eventMessage("output.volume", data).has_value());
  EXPECT_FALSE(rig.session->eventMessage("runtime.frame", data).has_value());

  rig.call(rig.request("debug.unsubscribe", {{"events", json::array({"runtime.stateChanged", "never.subscribed"})}}));
  EXPECT_FALSE(rig.session->eventMessage("runtime.stateChanged", data).has_value());
  EXPECT_TRUE(rig.session->eventMessage("output.volume", data).has_value());

  rig.call(rig.request("debug.subscribe", {{"events", json::array({"*"})}}));
  EXPECT_TRUE(rig.session->eventMessage("anything.else", data).has_value());
}

TEST(DebugSessionSubscriptions, RejectsBadPatternsAndTooManySubscriptions) {
  Rig rig;
  rig.login();
  const auto bad = rig.call(rig.request("debug.subscribe", {{"events", json::array({"runtime.stateChanged", "bogus"})}}));
  EXPECT_EQ(bad["error"]["code"], debug::RPC_INVALID_PARAMS);
  // 一次出错整条请求都不生效。
  EXPECT_FALSE(rig.session->eventMessage("runtime.stateChanged", json::object()).has_value());
  EXPECT_EQ(rig.call(rig.request("debug.subscribe", {{"events", json::array()}}))["error"]["code"], debug::RPC_INVALID_PARAMS);
  EXPECT_EQ(rig.call(rig.request("debug.subscribe", {}))["error"]["code"], debug::RPC_INVALID_PARAMS);
}

TEST(DebugSessionBinary, ResultsWithBinaryDataBecomeBinaryFrames) {
  Rig rig;
  rig.login();
  HandleResult result = rig.session->handleText(rig.request("test.binary", json::object(), 9).dump());
  ASSERT_EQ(result.outgoing.size(), 1u);
  const auto& frame = result.outgoing[0];
  EXPECT_EQ(frame.opcode, ws::Opcode::Binary);
  ASSERT_GE(frame.payload.size(), 4u);
  const std::uint32_t jsonLength = (static_cast<std::uint32_t>(frame.payload[0]) << 24) | (frame.payload[1] << 16) | (frame.payload[2] << 8) | frame.payload[3];
  ASSERT_EQ(frame.payload.size(), 4u + jsonLength + 4u);
  const auto header = json::parse(std::string(frame.payload.begin() + 4, frame.payload.begin() + 4 + jsonLength));
  EXPECT_EQ(header["id"], 9);
  EXPECT_EQ(header["result"]["format"], "png");
  EXPECT_FALSE(header["result"].contains("data"));
  EXPECT_EQ(std::vector<std::uint8_t>(frame.payload.end() - 4, frame.payload.end()), (std::vector<std::uint8_t>{1, 2, 3, 4}));
}

TEST(DebugSessionBinary, EventsWithBinaryDataAreBinaryFramesToo) {
  Rig rig;
  rig.login();
  rig.call(rig.request("debug.subscribe", {{"events", json::array({"*"})}}));
  const auto message = rig.session->eventMessage("runtime.frame", json{{"frame", 5}, {"data", json::binary({9, 8, 7})}});
  ASSERT_TRUE(message.has_value());
  EXPECT_EQ(message->opcode, ws::Opcode::Binary);
  const std::uint32_t jsonLength = (static_cast<std::uint32_t>(message->payload[0]) << 24) | (message->payload[1] << 16) | (message->payload[2] << 8) | message->payload[3];
  const auto header = json::parse(std::string(message->payload.begin() + 4, message->payload.begin() + 4 + jsonLength));
  EXPECT_EQ(header["method"], "runtime.frame");
  EXPECT_EQ(header["params"]["frame"], 5);
  EXPECT_FALSE(header["params"].contains("data"));
  EXPECT_EQ(std::vector<std::uint8_t>(message->payload.end() - 3, message->payload.end()), (std::vector<std::uint8_t>{9, 8, 7}));
}

TEST(DebugSessionBuiltIns, StatusAndDiscoverAreAnsweredByTheSession) {
  Rig rig;
  rig.login();
  const auto status = rig.call(rig.request("debug.status"));
  EXPECT_EQ(status["result"]["enabled"], true);
  EXPECT_EQ(status["result"]["port"], 5688);
  EXPECT_EQ(rig.call(rig.request("rpc.discover"))["result"]["openrpc"], "1.3.2");
  EXPECT_TRUE(rig.calls.empty());
}

TEST(DebugSessionBuiltIns, DiscoverIsMissingWithoutADocument) {
  Rig rig;
  auto context = std::make_shared<SessionContext>(*rig.context_);
  context->discovery = nullptr;
  rig.session = std::make_unique<DebugSession>(8, context);
  rig.context_ = context;
  rig.login();
  EXPECT_EQ(rig.call(rig.request("rpc.discover"))["error"]["code"], debug::RPC_METHOD_NOT_FOUND);
}

TEST(DebugSessionCancel, CancelMarksTheRunningRequestsToken) {
  Rig rig;
  rig.login();
  const auto response = rig.call(rig.request("test.cancelSelf"));
  EXPECT_EQ(response["result"]["seen"], true);
  // 下一个请求用新的标记。
  rig.call(rig.request("runtime.status"));
  EXPECT_FALSE(rig.lastTokenCancelled);
}

}  // namespace
