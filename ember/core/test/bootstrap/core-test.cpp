/*
 * ------------------------------------------------------------------------
 * 名称：core-test.cpp
 * 说明：核心入口的测试：启动参数、system.version、mappers.*、output.setVolume、runtime.* 与 cartridges.* 请求、帧处理函数与按键输入、未登记请求被拒绝、错误码、未启动与停止后的请求。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * ------------------------------------------------------------------------
 */

#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <future>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "bootstrap/core.hpp"
#include "shared/errors/core-error.hpp"
#include "../support/plugin-test-support.hpp"
#include "../support/runtime-test-support.hpp"

namespace {

using ember::bootstrap::Core;

/** 一次请求的结果。 */
struct Reply {
  bool ok = false;
  nlohmann::json payload;
};

/** 同步等待请求结果。 */
Reply call(Core& core, const std::string& id, const std::string& name, const std::string& params) {
  std::promise<Reply> promise;
  auto future = promise.get_future();
  core.request(id, name, params, [&promise](bool ok, std::string payload) { promise.set_value({ok, nlohmann::json::parse(payload)}); });
  if (future.wait_for(std::chrono::seconds(10)) != std::future_status::ready) throw std::runtime_error("请求超时");
  return future.get();
}

/** 临时用户数据目录。 */
class CoreTest : public testing::Test {
 protected:
  void SetUp() override {
    userData_ = std::filesystem::temp_directory_path() / ("ember-core-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  }
  void TearDown() override {
    std::error_code error;
    std::filesystem::remove_all(userData_, error);
  }
  [[nodiscard]] std::string options() const {
    const auto text = userData_.u8string();
    return nlohmann::json{{"userDataPath", std::string(text.begin(), text.end())}, {"appVersion", "9.8.7"}, {"workerCount", 2}}.dump();
  }

  /** 带预装插件目录的启动参数。 */
  [[nodiscard]] std::string optionsWithBundled(const std::filesystem::path& bundled) const {
    const auto text = userData_.u8string();
    const auto bundledText = bundled.u8string();
    return nlohmann::json{{"userDataPath", std::string(text.begin(), text.end())},
                          {"appVersion", "9.8.7"},
                          {"workerCount", 2},
                          {"bundledPluginsPath", std::string(bundledText.begin(), bundledText.end())}}
        .dump();
  }

  std::filesystem::path userData_;
};

TEST_F(CoreTest, StartsAndAnswersSystemVersion) {
  Core core;
  const auto info = nlohmann::json::parse(core.start(options()));
  EXPECT_FALSE(info["coreVersion"].get<std::string>().empty());
  const auto& requests = info["requests"];
  EXPECT_EQ(requests.size(), 69U);
  for (const char* name : {"system.version", "preferences.list", "preferences.set", "preferences.reset", "input.getBindings", "input.setBindings", "input.resetBindings", "mappers.list", "mappers.info", "mappers.install", "mappers.uninstall", "mappers.setEnabled",
                           "mappers.reload", "output.setVolume", "runtime.status", "runtime.powerOn", "runtime.powerOff", "runtime.reset", "runtime.pause", "runtime.resume", "cartridges.load",
                           "cartridges.eject", "cartridges.info", "debug.hello", "debug.subscribe", "debug.unsubscribe", "debug.status", "machines.list", "machines.boot", "media.floppyInsert",
                           "media.floppyWriteProtect", "media.printerFlush"}) {
    EXPECT_NE(std::find(requests.begin(), requests.end(), nlohmann::json(name)), requests.end()) << name;
  }

  const auto reply = call(core, "1", "system.version", "{}");
  ASSERT_TRUE(reply.ok);
  EXPECT_EQ(reply.payload["appVersion"], "9.8.7");
  EXPECT_EQ(reply.payload["coreVersion"], info["coreVersion"]);
  core.stop();
}

TEST_F(CoreTest, RejectsUnregisteredRequestsAndBadParams) {
  Core core;
  static_cast<void>(core.start(options()));
  const auto unknown = call(core, "1", "system.missing", "{}");
  EXPECT_FALSE(unknown.ok);
  EXPECT_EQ(unknown.payload["code"], "core.unknownRequest");
  const auto unknownModule = call(core, "2", "nothing.version", "{}");
  EXPECT_FALSE(unknownModule.ok);
  EXPECT_EQ(unknownModule.payload["code"], "core.unknownRequest");

  const auto invalid = call(core, "3", "system.version", R"({"extra":1})");
  EXPECT_FALSE(invalid.ok);
  EXPECT_EQ(invalid.payload["code"], "core.invalidParams");
  const auto notJson = call(core, "4", "system.version", "{");
  EXPECT_FALSE(notJson.ok);
  EXPECT_EQ(notJson.payload["code"], "core.invalidParams");
  core.stop();
}

TEST_F(CoreTest, OutputSetVolumeKeepsTheValuesInCoreMemory) {
  Core core;
  static_cast<void>(core.start(options()));
  // 初始音量 100、未静音；省略的参数保持不变。
  const auto initial = call(core, "1", "output.setVolume", "{}");
  ASSERT_TRUE(initial.ok);
  EXPECT_EQ(initial.payload["volume"], 100);
  EXPECT_EQ(initial.payload["muted"], false);

  const auto quiet = call(core, "2", "output.setVolume", R"({"volume":35})");
  ASSERT_TRUE(quiet.ok);
  EXPECT_EQ(quiet.payload["volume"], 35);
  EXPECT_EQ(quiet.payload["muted"], false);

  const auto muted = call(core, "3", "output.setVolume", R"({"muted":true})");
  ASSERT_TRUE(muted.ok);
  EXPECT_EQ(muted.payload["volume"], 35);
  EXPECT_EQ(muted.payload["muted"], true);

  const auto both = call(core, "4", "output.setVolume", R"({"volume":0,"muted":false})");
  ASSERT_TRUE(both.ok);
  EXPECT_EQ(both.payload["volume"], 0);
  EXPECT_EQ(both.payload["muted"], false);

  for (const char* bad : {R"({"volume":101})", R"({"volume":-1})", R"({"volume":1.5})", R"({"muted":1})", R"({"volume":"loud"})", R"({"other":1})"}) {
    const auto reply = call(core, "5", "output.setVolume", bad);
    EXPECT_FALSE(reply.ok) << bad;
    EXPECT_EQ(reply.payload["code"], "core.invalidParams") << bad;
  }
  // 非法的请求不改变已保存的值。
  const auto after = call(core, "6", "output.setVolume", "{}");
  EXPECT_EQ(after.payload["volume"], 0);
  core.stop();
}

TEST_F(CoreTest, PreferencesListSetAndResetThroughTheRequests) {
  Core core;
  static_cast<void>(core.start(options()));
  const auto listed = call(core, "1", "preferences.list", "{}");
  ASSERT_TRUE(listed.ok);
  std::vector<std::string> groups;
  for (const auto& group : listed.payload["groups"]) groups.push_back(group["id"]);
  EXPECT_EQ(groups, (std::vector<std::string>{"appearance", "display", "sound", "input", "printer", "debug"}));
  std::map<std::string, nlohmann::json> byId;
  for (const auto& setting : listed.payload["settings"]) byId[setting["id"]] = setting;
  EXPECT_EQ(byId.size(), 13U);
  EXPECT_EQ(byId["input.captureMouse"]["kind"], "switch");
  EXPECT_EQ(byId["input.captureMouse"]["value"], false);
  EXPECT_EQ(byId["appearance.theme"]["kind"], "choice");
  EXPECT_EQ(byId["appearance.theme"]["value"], "system");
  EXPECT_EQ(byId["appearance.theme"]["options"].size(), 3U);
  EXPECT_EQ(byId["display.aspect"]["defaultValue"], "8:7");
  EXPECT_EQ(byId["display.showAllLines"]["value"], true);
  EXPECT_EQ(byId["sound.buffer"]["value"], "standard");
  EXPECT_EQ(byId["sound.startMuted"]["kind"], "switch");
  EXPECT_EQ(byId["sound.volume"]["hidden"], true);
  EXPECT_EQ(byId["sound.volume"]["minimum"], 0);
  EXPECT_EQ(byId["sound.volume"]["maximum"], 100);

  const auto set = call(core, "2", "preferences.set", R"({"id":"appearance.theme","value":"dark"})");
  ASSERT_TRUE(set.ok);
  EXPECT_EQ(set.payload["value"], "dark");
  EXPECT_EQ(call(core, "3", "preferences.list", "{}").payload["settings"][0]["value"], "dark");

  EXPECT_EQ(call(core, "4", "preferences.set", R"({"id":"appearance.theme","value":"pink"})").payload["code"], "preferences.invalidValue");
  EXPECT_EQ(call(core, "5", "preferences.set", R"({"id":"appearance.theme","value":1.5})").payload["code"], "preferences.invalidValue");
  EXPECT_EQ(call(core, "6", "preferences.set", R"({"id":"appearance.nothing","value":true})").payload["code"], "preferences.unknown");
  EXPECT_EQ(call(core, "7", "preferences.set", R"({"id":"appearance.theme"})").payload["code"], "core.invalidParams");
  EXPECT_EQ(call(core, "8", "preferences.reset", R"({"id":"appearance.nothing"})").payload["code"], "preferences.unknown");

  const auto reset = call(core, "9", "preferences.reset", R"({"id":"appearance.theme"})");
  ASSERT_TRUE(reset.ok);
  EXPECT_EQ(reset.payload["value"], "system");
  core.stop();
}

TEST_F(CoreTest, PreferencesAreKeptAcrossARestartIncludingTheVolume) {
  {
    Core core;
    static_cast<void>(core.start(options()));
    ASSERT_TRUE(call(core, "1", "preferences.set", R"({"id":"display.scaling","value":"integer"})").ok);
    ASSERT_TRUE(call(core, "2", "output.setVolume", R"({"volume":35,"muted":true})").ok);
    core.stop();
  }
  Core core;
  static_cast<void>(core.start(options()));
  std::map<std::string, nlohmann::json> byId;
  const auto relisted = call(core, "3", "preferences.list", "{}");
  for (const auto& setting : relisted.payload["settings"]) byId[setting["id"]] = setting;
  EXPECT_EQ(byId["display.scaling"]["value"], "integer");
  EXPECT_EQ(byId["sound.volume"]["value"], 35);
  const auto volume = call(core, "4", "output.setVolume", "{}");
  EXPECT_EQ(volume.payload["volume"], 35);
  EXPECT_EQ(volume.payload["muted"], false) << "静音状态不保存";

  // 启动时静音打开后，重启就是静音的。
  ASSERT_TRUE(call(core, "5", "preferences.set", R"({"id":"sound.startMuted","value":true})").ok);
  core.stop();
  Core again;
  static_cast<void>(again.start(options()));
  EXPECT_EQ(call(again, "6", "output.setVolume", "{}").payload["muted"], true);
  again.stop();
}

TEST_F(CoreTest, RejectsBadStartOptions) {
  Core core;
  EXPECT_THROW(core.start("{"), ember::shared::SchemaError);
  EXPECT_THROW(core.start(R"({"userDataPath":"x"})"), ember::shared::SchemaError);
  EXPECT_THROW(core.start(R"({"appVersion":"1.0.0"})"), ember::shared::SchemaError);
  EXPECT_THROW(core.start(R"({"userDataPath":"x","appVersion":"1.0.0","other":1})"), ember::shared::SchemaError);
  EXPECT_THROW(core.start(R"({"userDataPath":"x","appVersion":"1.0.0","bundledPluginsPath":1})"), ember::shared::SchemaError);
}

TEST_F(CoreTest, MappersRequestsManageTheBundledAndUserPlugins) {
  const std::filesystem::path bundled = userData_ / "bundled";
  std::filesystem::create_directories(bundled);
  std::ofstream(bundled / "nrom.emap", std::ios::binary).write(reinterpret_cast<const char*>(ember::test::officialPluginPackage("nrom").data()),
                                                                static_cast<std::streamsize>(ember::test::officialPluginPackage("nrom").size()));
  Core core;
  static_cast<void>(core.start(optionsWithBundled(bundled)));

  const auto listed = call(core, "1", "mappers.list", "{}");
  ASSERT_TRUE(listed.ok);
  ASSERT_EQ(listed.payload["plugins"].size(), 1U);
  EXPECT_EQ(listed.payload["plugins"][0]["id"], "nrom");
  EXPECT_EQ(listed.payload["plugins"][0]["source"], "bundled");
  EXPECT_EQ(listed.payload["plugins"][0]["enabled"], true);

  const auto info = call(core, "2", "mappers.info", R"({"id":"nrom"})");
  ASSERT_TRUE(info.ok);
  EXPECT_EQ(info.payload["plugin"]["windows"].size(), 4U);
  EXPECT_EQ(info.payload["plugin"]["summary"]["match"][0]["mapper"], 0);
  const auto missing = call(core, "3", "mappers.info", R"({"id":"nothing"})");
  EXPECT_FALSE(missing.ok);
  EXPECT_EQ(missing.payload["code"], "mappers.notFound");

  const auto disabled = call(core, "4", "mappers.setEnabled", R"({"id":"nrom","enabled":false})");
  ASSERT_TRUE(disabled.ok);
  EXPECT_EQ(disabled.payload["plugin"]["enabled"], false);
  EXPECT_EQ(call(core, "5", "mappers.list", "{}").payload["plugins"][0]["enabled"], false);

  const auto readonly = call(core, "6", "mappers.uninstall", R"({"id":"nrom"})");
  EXPECT_FALSE(readonly.ok);
  EXPECT_EQ(readonly.payload["code"], "mappers.bundledReadonly");

  // 安装的文件路径经请求传入（界面不能发起，在核心里经主进程或调试服务发起）。
  const auto file = userData_ / "extra.emap";
  std::ofstream(file, std::ios::binary).write(reinterpret_cast<const char*>(ember::test::testPluginPackage("cnrom").data()),
                                              static_cast<std::streamsize>(ember::test::testPluginPackage("cnrom").size()));
  const auto installed = call(core, "7", "mappers.install", nlohmann::json{{"path", file.string()}}.dump());
  ASSERT_TRUE(installed.ok) << installed.payload.dump();
  EXPECT_EQ(installed.payload["plugin"]["id"], "test-cnrom");
  EXPECT_EQ(installed.payload["plugin"]["source"], "user");
  EXPECT_TRUE(std::filesystem::exists(userData_ / "plugins" / "test-cnrom.emap"));
  EXPECT_EQ(call(core, "8", "mappers.list", "{}").payload["plugins"].size(), 2U);

  const auto uninstalled = call(core, "9", "mappers.uninstall", R"({"id":"test-cnrom"})");
  EXPECT_TRUE(uninstalled.ok);
  EXPECT_FALSE(std::filesystem::exists(userData_ / "plugins" / "test-cnrom.emap"));

  const auto reloaded = call(core, "10", "mappers.reload", "{}");
  ASSERT_TRUE(reloaded.ok);
  ASSERT_EQ(reloaded.payload["plugins"].size(), 1U);
  EXPECT_EQ(reloaded.payload["plugins"][0]["enabled"], false);  // 停用状态跨重新加载保持

  const auto invalid = call(core, "11", "mappers.setEnabled", R"({"id":"nrom"})");
  EXPECT_FALSE(invalid.ok);
  EXPECT_EQ(invalid.payload["code"], "core.invalidParams");
  const auto badPackage = call(core, "12", "mappers.install", nlohmann::json{{"path", (userData_ / "missing.emap").string()}}.dump());
  EXPECT_FALSE(badPackage.ok);
  EXPECT_EQ(badPackage.payload["code"], "mappers.invalidPackage");
  core.stop();
}

TEST_F(CoreTest, RuntimeAndCartridgeRequestsRunACartridgeAndDeliverFramesAndEvents) {
  const std::filesystem::path bundled = userData_ / "bundled";
  std::filesystem::create_directories(bundled);
  const auto nrom = ember::test::officialPluginPackage("nrom");
  std::ofstream(bundled / "nrom.emap", std::ios::binary).write(reinterpret_cast<const char*>(nrom.data()), static_cast<std::streamsize>(nrom.size()));
  ember::test::Assembler program(0x8000);
  program.jmpSelf();
  const auto rom = ember::test::makeInesFile(program.bytes());
  const auto romPath = userData_ / "idle.nes";
  std::ofstream(romPath, std::ios::binary).write(reinterpret_cast<const char*>(rom.data()), static_cast<std::streamsize>(rom.size()));
  const auto romText = romPath.u8string();

  Core core;
  std::atomic<int> frames{0};
  std::atomic<std::uint32_t> width{0};
  std::atomic<std::uint32_t> height{0};
  core.setFrameHandler([&](const ember::platform::FrameView& frame) {
    width = frame.width;
    height = frame.height;
    ++frames;
  });
  std::mutex eventsMutex;
  std::vector<std::string> eventNames;
  core.setEventHandler([&](const std::string& name, const std::string&) {
    std::lock_guard lock(eventsMutex);
    eventNames.push_back(name);
  });
  static_cast<void>(core.start(optionsWithBundled(bundled)));

  const auto idle = call(core, "1", "runtime.status", "{}");
  ASSERT_TRUE(idle.ok);
  EXPECT_EQ(idle.payload["power"], "off");
  EXPECT_FALSE(idle.payload.contains("cartridge"));
  EXPECT_EQ(call(core, "2", "runtime.powerOn", "{}").payload["code"], "runtime.noCartridge");
  EXPECT_EQ(call(core, "3", "cartridges.info", "{}").payload["code"], "cartridges.empty");

  const auto loaded = call(core, "4", "cartridges.load", nlohmann::json{{"path", std::string(romText.begin(), romText.end())}}.dump());
  ASSERT_TRUE(loaded.ok) << loaded.payload.dump();
  EXPECT_EQ(loaded.payload["cartridge"]["pluginId"], "nrom");
  EXPECT_EQ(loaded.payload["state"], (nlohmann::json{{"power", "on"}, {"mode", "running"}}));
  ASSERT_TRUE(ember::test::waitFor([&] { return frames.load() >= 10; }));
  EXPECT_EQ(width.load(), 256U);
  EXPECT_EQ(height.load(), 240U);

  const auto running = call(core, "5", "runtime.status", "{}");
  EXPECT_EQ(running.payload["machine"], "fc");
  EXPECT_EQ(running.payload["cartridge"]["mapper"], 0);
  EXPECT_GE(running.payload["frame"].get<int>(), 10);
  EXPECT_EQ(running.payload["speed"], 100);

  EXPECT_EQ(call(core, "6", "runtime.pause", "{}").payload["state"]["mode"], "paused");
  EXPECT_EQ(call(core, "7", "runtime.resume", "{}").payload["state"]["mode"], "running");
  EXPECT_TRUE(call(core, "8", "runtime.reset", R"({"kind":"soft"})").ok);
  EXPECT_TRUE(call(core, "9", "runtime.reset", R"({"kind":"hard"})").ok);
  EXPECT_EQ(call(core, "10", "runtime.reset", R"({"kind":"warm"})").payload["code"], "core.invalidParams");
  EXPECT_EQ(call(core, "11", "runtime.reset", "{}").payload["code"], "core.invalidParams");
  EXPECT_EQ(call(core, "12", "cartridges.load", "{}").payload["code"], "core.invalidParams");
  EXPECT_EQ(call(core, "13", "cartridges.load", R"({"path":"nothing.nes"})").payload["code"], "cartridges.fileNotFound");
  EXPECT_EQ(call(core, "14", "runtime.powerOff", "{}").payload["state"]["power"], "off");
  EXPECT_EQ(call(core, "15", "runtime.powerOn", R"({"resume":true})").payload["state"]["mode"], "running");

  // 输入经命令队列交给卡带的脚本；不合格的消息立即报错。
  EXPECT_NO_THROW(core.sendInput(R"({"type":"key","code":"Space","down":true})"));
  EXPECT_THROW(core.sendInput(R"({"type":"key","code":"Sp ace","down":true})"), ember::shared::CoreError);

  EXPECT_EQ(call(core, "16", "cartridges.info", "{}").payload["cartridge"]["pluginId"], "nrom");
  EXPECT_EQ(call(core, "17", "cartridges.eject", "{}").payload["state"]["power"], "off");
  EXPECT_EQ(call(core, "18", "cartridges.info", "{}").payload["code"], "cartridges.empty");

  {
    std::lock_guard lock(eventsMutex);
    EXPECT_NE(std::find(eventNames.begin(), eventNames.end(), "cartridges.loaded"), eventNames.end());
    EXPECT_NE(std::find(eventNames.begin(), eventNames.end(), "runtime.stateChanged"), eventNames.end());
  }

  core.stop();
  const int framesAtStop = frames.load();
  EXPECT_THROW(core.sendInput(R"({"type":"key","code":"Space","down":true})"), ember::shared::CoreError);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  EXPECT_EQ(frames.load(), framesAtStop);
}

TEST_F(CoreTest, StoppingTheCoreWhileACartridgeRunsExitsCleanly) {
  const std::filesystem::path bundled = userData_ / "bundled";
  std::filesystem::create_directories(bundled);
  const auto nrom = ember::test::officialPluginPackage("nrom");
  std::ofstream(bundled / "nrom.emap", std::ios::binary).write(reinterpret_cast<const char*>(nrom.data()), static_cast<std::streamsize>(nrom.size()));
  ember::test::Assembler program(0x8000);
  program.jmpSelf();
  const auto rom = ember::test::makeInesFile(program.bytes());
  const auto romPath = userData_ / "idle.nes";
  std::ofstream(romPath, std::ios::binary).write(reinterpret_cast<const char*>(rom.data()), static_cast<std::streamsize>(rom.size()));
  const auto romText = romPath.u8string();

  Core core;
  std::atomic<int> frames{0};
  core.setFrameHandler([&](const ember::platform::FrameView&) { ++frames; });
  static_cast<void>(core.start(optionsWithBundled(bundled)));
  ASSERT_TRUE(call(core, "1", "cartridges.load", nlohmann::json{{"path", std::string(romText.begin(), romText.end())}}.dump()).ok);
  ASSERT_TRUE(ember::test::waitFor([&] { return frames.load() >= 3; }));
  // 运行中直接停止：线程干净退出，不死锁；之后的请求以 core.unavailable 拒绝。
  auto stopped = std::async(std::launch::async, [&] { core.stop(); });
  ASSERT_EQ(stopped.wait_for(std::chrono::seconds(10)), std::future_status::ready);
  EXPECT_EQ(call(core, "2", "runtime.status", "{}").payload["code"], "core.unavailable");
}

TEST_F(CoreTest, RefusesSecondStartAndRequestsAfterStop) {
  Core core;
  const auto before = call(core, "0", "system.version", "{}");
  EXPECT_FALSE(before.ok);
  EXPECT_EQ(before.payload["code"], "core.unavailable");

  static_cast<void>(core.start(options()));
  EXPECT_THROW(core.start(options()), ember::shared::CoreError);
  ASSERT_TRUE(call(core, "1", "system.version", "{}").ok);
  core.stop();
  const auto after = call(core, "2", "system.version", "{}");
  EXPECT_FALSE(after.ok);
  EXPECT_EQ(after.payload["code"], "core.unavailable");
}

}  // namespace
