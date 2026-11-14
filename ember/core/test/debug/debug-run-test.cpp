/*
 * ------------------------------------------------------------------------
 * 名称：debug-run-test.cpp
 * 说明：调试服务相关的新请求的端到端测试：授权运行（runtime.run）、手柄与按键输入（input.*）、画面截图（output.screenshot，含二进制帧）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：经核心的请求通道与真实的调试连接（WebSocket）发请求，用最小的 iNES 程序开机。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <future>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <nlohmann/json.hpp>

#include "../support/debug-test-client.hpp"
#include "../support/runtime-test-support.hpp"
#include "bootstrap/core.hpp"
#include "platform/network/tcp-socket.hpp"

namespace {

using ember::bootstrap::Core;
using ember::test::DebugTestClient;
using nlohmann::json;
namespace ws = ember::platform::websocket;

struct Reply {
  bool ok = false;
  json payload;
};

Reply call(Core& core, const std::string& name, const json& params = json::object()) {
  static int counter = 0;
  std::promise<Reply> promise;
  auto future = promise.get_future();
  core.request("r" + std::to_string(++counter), name, params.dump(), [&promise](bool ok, std::string payload) { promise.set_value({ok, json::parse(payload)}); });
  if (future.wait_for(std::chrono::seconds(20)) != std::future_status::ready) throw std::runtime_error("请求超时");
  return future.get();
}

class DebugRunTest : public testing::Test {
 protected:
  void SetUp() override {
    root_ = std::filesystem::temp_directory_path() / ("ember-debug-run-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(root_);
    const auto rom = ember::test::makeInesFile(ember::test::makeBootMarkerProgram(0x11, 0x22));
    std::ofstream stream(root_ / "test.nes", std::ios::binary);
    stream.write(reinterpret_cast<const char*>(rom.data()), static_cast<std::streamsize>(rom.size()));
    stream.close();
    std::filesystem::create_directories(root_ / "bundled");
    const auto nrom = ember::test::officialPluginPackage("nrom");
    std::ofstream(root_ / "bundled" / "nrom.emap", std::ios::binary).write(reinterpret_cast<const char*>(nrom.data()), static_cast<std::streamsize>(nrom.size()));
  }
  void TearDown() override {
    std::error_code error;
    std::filesystem::remove_all(root_, error);
  }

  void start(Core& core) {
    const auto text = (root_ / "data").u8string();
    const auto bundledText = (root_ / "bundled").u8string();
    const json options{{"userDataPath", std::string(text.begin(), text.end())}, {"appVersion", "1.0.0"}, {"workerCount", 2}, {"bundledPluginsPath", std::string(bundledText.begin(), bundledText.end())}};
    core.setEventHandler([this](const std::string& name, const std::string& data) {
      std::lock_guard lock(mutex_);
      events_.push_back({{"name", name}, {"data", json::parse(data)}});
    });
    static_cast<void>(core.start(options.dump()));
  }

  void loadAndPowerOn(Core& core) {
    const auto text = (root_ / "test.nes").u8string();
    ASSERT_TRUE(call(core, "cartridges.load", {{"path", std::string(text.begin(), text.end())}}).ok);
    ASSERT_TRUE(call(core, "runtime.powerOn", {{"resume", false}}).ok);
  }

  [[nodiscard]] std::vector<json> eventsNamed(const std::string& name) {
    std::lock_guard lock(mutex_);
    std::vector<json> found;
    for (const auto& event : events_) {
      if (event["name"] == name) found.push_back(event["data"]);
    }
    return found;
  }

  std::filesystem::path root_;
  std::mutex mutex_;
  std::vector<json> events_;
};

TEST_F(DebugRunTest, RunRefusesWithoutACartridgeOrPower) {
  Core core;
  start(core);
  EXPECT_EQ(call(core, "runtime.run", {{"frames", 10}}).payload["code"], "runtime.noCartridge");
  const auto text = (root_ / "test.nes").u8string();
  ASSERT_TRUE(call(core, "cartridges.load", {{"path", std::string(text.begin(), text.end())}}).ok);
  EXPECT_EQ(call(core, "runtime.run", {{"frames", 10}}).payload["code"], "runtime.notPowered");
  EXPECT_EQ(call(core, "runtime.run", {{"frames", 0}}).payload["code"], "core.invalidParams");
  core.stop();
}

TEST_F(DebugRunTest, RunWithWaitReturnsTheExactFrameCountAndAnnouncesTheEnd) {
  Core core;
  start(core);
  loadAndPowerOn(core);
  const auto result = call(core, "runtime.run", {{"frames", 120}, {"wait", true}});
  ASSERT_TRUE(result.ok) << result.payload.dump();
  EXPECT_EQ(result.payload["stopped"], "completed");
  EXPECT_EQ(result.payload["frames"], 120);
  EXPECT_GT(result.payload["cycles"].get<std::int64_t>(), 0);
  ASSERT_TRUE(ember::test::waitFor([&] { return !eventsNamed("runtime.runFinished").empty(); }));
  const auto finished = eventsNamed("runtime.runFinished").front();
  EXPECT_EQ(finished["stopped"], "completed");
  EXPECT_EQ(finished["frames"], 120);
  core.stop();
}

TEST_F(DebugRunTest, RunWithoutWaitReturnsAtOnceAndAPauseInterruptsIt) {
  Core core;
  start(core);
  loadAndPowerOn(core);
  const auto started = call(core, "runtime.run", {{"frames", 10000000}});
  ASSERT_TRUE(started.ok) << started.payload.dump();
  EXPECT_EQ(started.payload["frames"], 0);
  ASSERT_TRUE(call(core, "runtime.pause").ok);
  ASSERT_TRUE(ember::test::waitFor([&] { return !eventsNamed("runtime.runFinished").empty(); }));
  EXPECT_EQ(eventsNamed("runtime.runFinished").front()["stopped"], "interrupted");
  core.stop();
}

TEST_F(DebugRunTest, PowerOffEndsARunWithPowerOff) {
  Core core;
  start(core);
  loadAndPowerOn(core);
  ASSERT_TRUE(call(core, "runtime.run", {{"frames", 10000000}}).ok);
  ASSERT_TRUE(call(core, "runtime.powerOff").ok);
  ASSERT_TRUE(ember::test::waitFor([&] { return !eventsNamed("runtime.runFinished").empty(); }));
  EXPECT_EQ(eventsNamed("runtime.runFinished").front()["stopped"], "powerOff");
  core.stop();
}

TEST_F(DebugRunTest, InputRequestsValidateAndAcceptEvents) {
  Core core;
  start(core);
  EXPECT_EQ(call(core, "input.pad", {{"port", 0}, {"buttons", 255}}).ok, true);
  EXPECT_EQ(call(core, "input.pad", {{"port", 7}, {"buttons", 1}}).payload["code"], "input.invalidPort");
  EXPECT_EQ(call(core, "input.pad", {{"port", 0}, {"buttons", 256}}).payload["code"], "core.invalidParams");
  EXPECT_TRUE(call(core, "input.key", {{"code", "KeyZ"}, {"down", true}}).ok);
  EXPECT_TRUE(call(core, "input.mouse", {{"kind", "move"}, {"x", 10}, {"y", 20}}).ok);
  EXPECT_TRUE(call(core, "input.gamepad", {{"index", 0}, {"connected", true}, {"buttons", json::array({true, false})}}).ok);
  EXPECT_TRUE(call(core, "input.releaseAll").ok);
  core.stop();
}

TEST_F(DebugRunTest, ScreenshotHasNoPictureBeforePowerOnAndSavesAPngAfterwards) {
  Core core;
  start(core);
  EXPECT_EQ(call(core, "output.screenshot").payload["code"], "output.noPicture");
  loadAndPowerOn(core);
  ASSERT_TRUE(call(core, "runtime.run", {{"frames", 3}, {"wait", true}}).ok);

  const auto saved = call(core, "output.screenshot", {{"file", "first"}});
  ASSERT_TRUE(saved.ok) << saved.payload.dump();
  EXPECT_EQ(saved.payload["format"], "png");
  EXPECT_EQ(saved.payload["width"], 256);
  EXPECT_EQ(saved.payload["height"], 240);
  EXPECT_FALSE(saved.payload.contains("data"));
  const auto path = std::filesystem::path(std::u8string(saved.payload["file"].get<std::string>().begin(), saved.payload["file"].get<std::string>().end()));
  EXPECT_EQ(path.filename().string(), "first.png");
  EXPECT_EQ(path.parent_path().filename().string(), "debug-output");
  ASSERT_TRUE(std::filesystem::exists(path));
  std::ifstream stream(path, std::ios::binary);
  std::string signature(8, '\0');
  stream.read(signature.data(), 8);
  EXPECT_EQ(signature, std::string("\x89PNG\r\n\x1a\n", 8));
  EXPECT_EQ(static_cast<std::size_t>(std::filesystem::file_size(path)), saved.payload["byteLength"].get<std::size_t>());

  for (const char* bad : {"../x.png", "a/b", "a\\b", ".hidden", "CON", "x:y", ""}) {
    EXPECT_EQ(call(core, "output.screenshot", {{"file", bad}}).payload["code"], std::string(bad).empty() ? "core.invalidParams" : "output.invalidFileName") << bad;
  }
  core.stop();
}

TEST_F(DebugRunTest, ScreenshotTravelsAsABinaryFrameOverTheDebugConnection) {
  Core core;
  start(core);
  loadAndPowerOn(core);
  ASSERT_TRUE(call(core, "runtime.run", {{"frames", 3}, {"wait", true}}).ok);

  std::string error;
  auto listener = ember::platform::TcpListener::listenLoopback(0, error);
  const auto port = listener->port();
  listener.reset();
  ASSERT_TRUE(call(core, "preferences.set", {{"id", "debug.port"}, {"value", port}}).ok);
  ASSERT_TRUE(call(core, "preferences.set", {{"id", "debug.enabled"}, {"value", true}}).ok);
  const auto listed = call(core, "preferences.list");
  std::string token;
  for (const auto& item : listed.payload["settings"]) {
    if (item["id"] == "debug.token") token = item["value"].get<std::string>();
  }

  auto client = DebugTestClient::connect(port);
  ASSERT_TRUE(client != nullptr);
  ASSERT_TRUE(client->hello(token).contains("result"));

  const json request{{"jsonrpc", "2.0"}, {"id", 7}, {"method", "output.screenshot"}, {"params", {{"format", "rgba"}}}};
  ASSERT_TRUE(client->sendText(request.dump()));
  const auto frame = client->receive();
  ASSERT_TRUE(frame.has_value());
  ASSERT_EQ(frame->opcode, ws::Opcode::Binary);
  ASSERT_GE(frame->payload.size(), 4u);
  const std::size_t jsonLength = (static_cast<std::size_t>(frame->payload[0]) << 24) | (static_cast<std::size_t>(frame->payload[1]) << 16) | (static_cast<std::size_t>(frame->payload[2]) << 8) | frame->payload[3];
  ASSERT_LE(4 + jsonLength, frame->payload.size());
  const auto header = json::parse(std::string(frame->payload.begin() + 4, frame->payload.begin() + 4 + static_cast<std::ptrdiff_t>(jsonLength)));
  EXPECT_EQ(header["id"], 7);
  EXPECT_EQ(header["result"]["format"], "rgba");
  EXPECT_EQ(header["result"]["byteLength"], 256 * 240 * 4);
  EXPECT_FALSE(header["result"].contains("data"));
  EXPECT_EQ(frame->payload.size() - 4 - jsonLength, static_cast<std::size_t>(256 * 240 * 4));
  core.stop();
}

}  // namespace
