/*
 * ------------------------------------------------------------------------
 * 名称：debug-module-test.cpp
 * 说明：调试服务模块的端到端测试（真实的核心、真实的套接字）：默认关闭、令牌自动生成并保存、首选项里开启与关闭、端口与令牌更换、启动参数强制开启、端点文件、经调试连接调用核心请求与接收事件、debug.* 请求经界面通道的行为、接口描述文档。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include <nlohmann/json.hpp>

#include "../support/debug-test-client.hpp"
#include "bootstrap/core.hpp"
#include "platform/network/tcp-socket.hpp"
#include "shared/errors/core-error.hpp"

namespace {

using ember::bootstrap::Core;
using ember::test::DebugTestClient;
namespace ws = ember::platform::websocket;
using nlohmann::json;

struct Reply {
  bool ok = false;
  json payload;
};

Reply call(Core& core, const std::string& name, const json& params = json::object()) {
  static int counter = 0;
  std::promise<Reply> promise;
  auto future = promise.get_future();
  core.request("r" + std::to_string(++counter), name, params.dump(), [&promise](bool ok, std::string payload) { promise.set_value({ok, json::parse(payload)}); });
  if (future.wait_for(std::chrono::seconds(10)) != std::future_status::ready) throw std::runtime_error("请求超时");
  return future.get();
}

/** 找一个当前空闲的端口（先监听 0 号端口取到编号，再释放）。 */
std::uint16_t freePort() {
  std::string error;
  auto listener = ember::platform::TcpListener::listenLoopback(0, error);
  return listener->port();
}

class DebugModuleTest : public testing::Test {
 protected:
  void SetUp() override { userData_ = std::filesystem::temp_directory_path() / ("ember-debug-module-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())); }
  void TearDown() override {
    std::error_code error;
    std::filesystem::remove_all(userData_, error);
  }

  [[nodiscard]] json baseOptions() const {
    const auto text = userData_.u8string();
    return json{{"userDataPath", std::string(text.begin(), text.end())}, {"appVersion", "9.8.7"}, {"workerCount", 2}};
  }

  /** 首选项里某个设置项的当前值。 */
  static json setting(Core& core, const std::string& id) {
    const auto listed = call(core, "preferences.list");
    for (const auto& item : listed.payload["settings"]) {
      if (item["id"] == id) return item;
    }
    return nullptr;
  }

  static bool waitFor(const std::function<bool()>& predicate) {
    for (int i = 0; i < 300; ++i) {
      if (predicate()) return true;
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return predicate();
  }

  std::filesystem::path userData_;
};

TEST_F(DebugModuleTest, IsOffByDefaultButAlreadyHasAToken) {
  Core core;
  static_cast<void>(core.start(baseOptions().dump()));
  const auto status = call(core, "debug.status");
  ASSERT_TRUE(status.ok);
  EXPECT_EQ(status.payload["enabled"], false);
  EXPECT_EQ(status.payload["listening"], false);
  EXPECT_EQ(status.payload["port"], 0);
  EXPECT_TRUE(status.payload["connections"].empty());
  EXPECT_FALSE(std::filesystem::exists(userData_ / "run" / "debug-endpoint.json"));

  const auto token = setting(core, "debug.token");
  EXPECT_EQ(token["kind"], "text");
  EXPECT_EQ(token["maxLength"], 128);
  EXPECT_EQ(token["value"].get<std::string>().size(), 32u);
  EXPECT_EQ(setting(core, "debug.enabled")["value"], false);
  EXPECT_EQ(setting(core, "debug.port")["value"], 5688);
  core.stop();

  // 令牌保存在首选项里，重启后不变。
  Core again;
  static_cast<void>(again.start(baseOptions().dump()));
  EXPECT_EQ(setting(again, "debug.token")["value"], token["value"]);
  again.stop();
}

TEST_F(DebugModuleTest, StartsAndStopsFromThePreferencesAndServesCoreRequests) {
  Core core;
  std::mutex mutex;
  std::vector<json> events;
  core.setEventHandler([&](const std::string& name, const std::string& data) {
    std::lock_guard lock(mutex);
    events.push_back({{"name", name}, {"data", json::parse(data)}});
  });
  static_cast<void>(core.start(baseOptions().dump()));
  const auto port = freePort();
  ASSERT_TRUE(call(core, "preferences.set", {{"id", "debug.port"}, {"value", port}}).ok);
  ASSERT_TRUE(call(core, "preferences.set", {{"id", "debug.enabled"}, {"value", true}}).ok);

  const auto status = call(core, "debug.status").payload;
  EXPECT_EQ(status["enabled"], true);
  EXPECT_EQ(status["listening"], true);
  EXPECT_EQ(status["port"], port);

  const auto token = setting(core, "debug.token")["value"].get<std::string>();
  auto client = DebugTestClient::connect(port);
  ASSERT_TRUE(client != nullptr);
  ASSERT_EQ(client->handshake().status, 101);
  const auto hello = client->hello(token, "e2e");
  ASSERT_TRUE(hello.contains("result")) << hello.dump();
  EXPECT_EQ(hello["result"]["appVersion"], "9.8.7");

  // 经调试连接调用核心的请求，结果与经界面通道一致。
  const auto viaDebug = client->call("runtime.status", json::object(), 2);
  EXPECT_EQ(viaDebug["result"], call(core, "runtime.status").payload);
  EXPECT_EQ(client->call("system.version", json::object(), 3)["result"]["coreVersion"], call(core, "system.version").payload["coreVersion"]);
  // 参数校验与界面通道相同。
  EXPECT_EQ(client->call("output.setVolume", {{"volume", 101}}, 4)["error"]["code"], -32602);
  // 握手类请求不会在核心的请求通道里被处理。
  EXPECT_EQ(call(core, "debug.hello", {{"client", "x"}, {"token", token}}).payload["code"], "debug.connectionOnly");

  // 订阅后，首选项的变化与状态变化都会推给调试连接。
  ASSERT_TRUE(client->call("debug.subscribe", {{"events", json::array({"preferences.*", "debug.*"})}}, 5).contains("result"));
  ASSERT_TRUE(call(core, "preferences.set", {{"id", "appearance.theme"}, {"value", "dark"}}).ok);
  const auto changed = client->receiveJson();
  ASSERT_TRUE(changed.has_value());
  EXPECT_EQ((*changed)["method"], "preferences.changed");
  EXPECT_EQ((*changed)["params"]["id"], "appearance.theme");
  EXPECT_EQ((*changed)["params"]["value"], "dark");

  // 状态里能看到这条连接，界面通道的事件里也有 debug.statusChanged。
  const auto listed = client->call("debug.status", json::object(), 6)["result"];
  ASSERT_EQ(listed["connections"].size(), 1u);
  EXPECT_EQ(listed["connections"][0]["client"], "e2e");
  EXPECT_TRUE(waitFor([&] {
    std::lock_guard lock(mutex);
    return std::ranges::any_of(events, [](const json& event) { return event["name"] == "debug.statusChanged" && event["data"]["connections"] == 1; });
  }));

  // 端点文件里有地址与令牌。
  json endpointJson;
  {
    std::ifstream endpoint(userData_ / "run" / "debug-endpoint.json");
    ASSERT_TRUE(endpoint.good());
    endpointJson = json::parse(endpoint);
  }
  EXPECT_EQ(endpointJson["port"], port);
  EXPECT_EQ(endpointJson["token"], token);

  // 重新生成令牌（恢复默认）：旧令牌的连接被关闭，新令牌才能连。
  ASSERT_TRUE(call(core, "preferences.reset", {{"id", "debug.token"}}).ok);
  const auto newToken = setting(core, "debug.token")["value"].get<std::string>();
  EXPECT_EQ(newToken.size(), 32u);
  EXPECT_NE(newToken, token);
  bool closedWithTokenChange = false;
  for (int i = 0; i < 20 && !closedWithTokenChange; ++i) {
    const auto frame = client->receive(500);
    if (!frame) break;
    closedWithTokenChange = frame->opcode == ws::Opcode::Close && frame->closeCode() == ws::CLOSE_TOKEN_CHANGED;
  }
  EXPECT_TRUE(closedWithTokenChange);
  auto stale = DebugTestClient::connect(port);
  ASSERT_TRUE(stale != nullptr);
  EXPECT_EQ(stale->hello(token)["error"]["data"]["code"], "debug.unauthorized");
  auto fresh = DebugTestClient::connect(port);
  ASSERT_TRUE(fresh != nullptr);
  EXPECT_TRUE(fresh->hello(newToken).contains("result"));

  // 关闭：连接被关闭（1001），端口释放，端点文件删除，状态变为未监听。
  ASSERT_TRUE(call(core, "preferences.set", {{"id", "debug.enabled"}, {"value", false}}).ok);
  bool goingAway = false;
  for (int i = 0; i < 20 && !goingAway; ++i) {
    const auto frame = fresh->receive(500);
    if (!frame) break;
    goingAway = frame->opcode == ws::Opcode::Close && frame->closeCode() == 1001;
  }
  EXPECT_TRUE(goingAway);
  const auto off = call(core, "debug.status").payload;
  EXPECT_EQ(off["enabled"], false);
  EXPECT_EQ(off["listening"], false);
  EXPECT_FALSE(std::filesystem::exists(userData_ / "run" / "debug-endpoint.json"));
  EXPECT_TRUE(DebugTestClient::connect(port) == nullptr);
  core.stop();
}

TEST_F(DebugModuleTest, ChangingThePortRestartsTheListener) {
  Core core;
  static_cast<void>(core.start(baseOptions().dump()));
  const auto first = freePort();
  ASSERT_TRUE(call(core, "preferences.set", {{"id", "debug.port"}, {"value", first}}).ok);
  ASSERT_TRUE(call(core, "preferences.set", {{"id", "debug.enabled"}, {"value", true}}).ok);
  ASSERT_EQ(call(core, "debug.status").payload["port"], first);
  std::uint16_t second = freePort();
  while (second == first) second = freePort();
  ASSERT_TRUE(call(core, "preferences.set", {{"id", "debug.port"}, {"value", second}}).ok);
  const auto status = call(core, "debug.status").payload;
  EXPECT_EQ(status["listening"], true);
  EXPECT_EQ(status["port"], second);
  EXPECT_TRUE(DebugTestClient::connect(first) == nullptr);
  EXPECT_TRUE(DebugTestClient::connect(second) != nullptr);
  core.stop();
}

TEST_F(DebugModuleTest, ReportsWhyItIsNotListeningWhenThePortIsTaken) {
  Core core;
  static_cast<void>(core.start(baseOptions().dump()));
  std::string error;
  auto occupied = ember::platform::TcpListener::listenLoopback(0, error);
  ASSERT_TRUE(occupied != nullptr);
  ASSERT_TRUE(call(core, "preferences.set", {{"id", "debug.port"}, {"value", occupied->port()}}).ok);
  ASSERT_TRUE(call(core, "preferences.set", {{"id", "debug.enabled"}, {"value", true}}).ok);
  const auto status = call(core, "debug.status").payload;
  EXPECT_EQ(status["enabled"], true);
  EXPECT_EQ(status["listening"], false);
  EXPECT_EQ(status["port"], 0);
  EXPECT_NE(status["error"].get<std::string>().find("端口"), std::string::npos);
  core.stop();
}

TEST_F(DebugModuleTest, TheStartupOptionForcesTheServiceOnWithoutTouchingThePreferences) {
  const auto port = freePort();
  auto options = baseOptions();
  options["debugPort"] = port;
  options["debugToken"] = "from-the-command-line";
  Core core;
  static_cast<void>(core.start(options.dump()));
  EXPECT_EQ(call(core, "debug.status").payload["port"], port);
  EXPECT_EQ(setting(core, "debug.enabled")["value"], false);
  EXPECT_EQ(setting(core, "debug.port")["value"], 5688);
  EXPECT_NE(setting(core, "debug.token")["value"], "from-the-command-line");

  auto client = DebugTestClient::connect(port);
  ASSERT_TRUE(client != nullptr);
  EXPECT_TRUE(client->hello("from-the-command-line").contains("result"));
  json endpointJson;
  {
    std::ifstream endpoint(userData_ / "run" / "debug-endpoint.json");
    endpointJson = json::parse(endpoint);
  }
  EXPECT_EQ(endpointJson["token"], "from-the-command-line");
  core.stop();
  EXPECT_FALSE(std::filesystem::exists(userData_ / "run" / "debug-endpoint.json"));
}

TEST_F(DebugModuleTest, RejectsBadStartupOptions) {
  for (const json bad : {json{{"debugPort", 0}}, json{{"debugPort", 70000}}, json{{"debugPort", "5688"}}, json{{"debugToken", ""}}, json{{"debugToken", 5}}}) {
    auto options = baseOptions();
    options.update(bad);
    Core core;
    EXPECT_THROW(static_cast<void>(core.start(options.dump())), ember::shared::SchemaError) << bad.dump();
  }
}

TEST_F(DebugModuleTest, DiscoverReturnsTheOpenRpcDocumentCoveringEveryRequest) {
  const auto port = freePort();
  auto options = baseOptions();
  options["debugPort"] = port;
  options["debugToken"] = "discover-token";
  Core core;
  const auto info = json::parse(core.start(options.dump()));
  auto client = DebugTestClient::connect(port);
  ASSERT_TRUE(client != nullptr);
  ASSERT_TRUE(client->hello("discover-token").contains("result"));
  const auto document = client->call("rpc.discover", json::object(), 2)["result"];
  EXPECT_EQ(document["openrpc"], "1.3.2");
  std::set<std::string> described;
  for (const auto& method : document["methods"]) described.insert(method["name"]);
  EXPECT_TRUE(described.contains("rpc.discover"));
  // 核心登记的每个请求都有描述，描述里的每个请求核心也都登记了。
  for (const auto& name : info["requests"]) EXPECT_TRUE(described.contains(name.get<std::string>())) << name;
  for (const auto& name : described) {
    if (name == "rpc.discover") continue;
    EXPECT_NE(std::find(info["requests"].begin(), info["requests"].end(), json(name)), info["requests"].end()) << name;
  }
  core.stop();
}

}  // namespace
