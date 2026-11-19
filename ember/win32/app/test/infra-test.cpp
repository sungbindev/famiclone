/*
 * ------------------------------------------------------------------------
 * 名称：infra-test.cpp
 * 说明：实现层的测试：界面状态、最近打开的卡带、记住上次目录、拖入文件服务的读写与持久化，画面帧单槽缓冲，进程内核心网关（真实启动核心）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：文件服务用临时目录里的真实文件；核心网关的测试在进程内启动真实的核心（用户数据目录是临时目录），请求 system.version、preferences.set，再停止。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <chrono>
#include <fstream>
#include <thread>

#include "infra/core-client/in-process-core-gateway.hpp"
#include "infra/core-client/latest-frame-slot.hpp"
#include "infra/dropped-files/memory-dropped-files-service.hpp"
#include "infra/last-directories/file-last-directories-service.hpp"
#include "infra/recent-cartridges/file-recent-cartridges-service.hpp"
#include "infra/shell-state/file-shell-state-service.hpp"
#include "infra/updates/unsupported-app-updater.hpp"
#include "platform/text/wide-text.hpp"
#include "support/fakes.hpp"
#include "support/temp-directory.hpp"

namespace {

using namespace ember::win32;
using nlohmann::json;

template <typename Condition>
bool pumpUntil(Condition condition, int timeoutMilliseconds = 15000) {
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMilliseconds);
  while (!condition()) {
    if (std::chrono::steady_clock::now() > deadline) return false;
    platform::UiThread::pumpMessages();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
  return true;
}

TEST(FileShellStateService, StartsFromDefaultsAndPersistsEveryKindOfChange) {
  test::TempDirectory directory;
  platform::UiThread ui;
  const auto file = directory.path() / "config" / "shell-state.json";
  {
    platform::JsonFileStore store(file, ui);
    infra::FileShellStateService service(store);
    EXPECT_FALSE(service.get().window);
    service.updateWindow({30, 40});
    service.updateLayout({std::string("sb2000"), {"runtime.panel"}});
    service.updateReleaseNotesVersion("0.3.0");
    service.flush();
  }
  platform::JsonFileStore store(file, ui);
  infra::FileShellStateService service(store);
  ASSERT_TRUE(service.get().window);
  EXPECT_EQ(*service.get().window, (shared::WindowPlacement{30, 40}));
  EXPECT_EQ(service.get().layout.selectedMachine, "sb2000");
  EXPECT_EQ(service.get().layout.autoCollapseOffPanels, std::vector<std::string>{"runtime.panel"});
  EXPECT_EQ(service.get().releaseNotesVersion, "0.3.0");
}

TEST(FileShellStateService, ReadsTheFileFormatOfTheElectronShell) {
  test::TempDirectory directory;
  platform::UiThread ui;
  const auto file = directory.path() / "shell-state.json";
  std::ofstream(file) << R"({"window":{"x":100,"y":50},"layout":{"selectedMachine":null,"autoCollapseOffPanels":["media.devices"]},"releaseNotesVersion":"0.2.2"})";
  platform::JsonFileStore store(file, ui);
  infra::FileShellStateService service(store);
  EXPECT_EQ(*service.get().window, (shared::WindowPlacement{100, 50}));
  EXPECT_FALSE(service.get().layout.selectedMachine);
  EXPECT_EQ(service.get().layout.autoCollapseOffPanels, std::vector<std::string>{"media.devices"});
}

TEST(FileRecentCartridgesService, KeepsIdsStableAndPersists) {
  test::TempDirectory directory;
  platform::UiThread ui;
  const auto file = directory.path() / "recent.json";
  {
    platform::JsonFileStore store(file, ui);
    infra::FileRecentCartridgesService service(store);
    service.add("D:\\roms\\a.nes");
    service.add("D:\\roms\\b.nes");
    const auto before = service.list();
    ASSERT_EQ(before.size(), 2u);
    EXPECT_EQ(before[0].name, "b.nes");
    const auto idOfA = before[1].id;
    service.add("D:\\roms\\a.nes");
    EXPECT_EQ(service.list()[0].id, idOfA);
    EXPECT_EQ(service.resolve(idOfA), "D:\\roms\\a.nes");
    EXPECT_FALSE(service.resolve(9999));
    service.add("D:\\roms\\notes.txt");
    EXPECT_EQ(service.list().size(), 2u);
    service.remove("D:\\roms\\b.nes");
    service.flush();
  }
  platform::JsonFileStore store(file, ui);
  infra::FileRecentCartridgesService service(store);
  ASSERT_EQ(service.list().size(), 1u);
  EXPECT_EQ(service.list()[0].name, "a.nes");
  EXPECT_EQ(service.list()[0].id, 1);
}

TEST(FileLastDirectoriesService, RemembersPerPurposeAndRejectsBadInput) {
  test::TempDirectory directory;
  platform::UiThread ui;
  const auto file = directory.path() / "last.json";
  {
    platform::JsonFileStore store(file, ui);
    infra::FileLastDirectoriesService service(store);
    EXPECT_FALSE(service.get("cartridge"));
    service.set("cartridge", "D:\\roms");
    service.set("floppy", "D:\\disks");
    service.set("bad purpose", "x");
    service.set("tape", "");
    service.flush();
  }
  platform::JsonFileStore store(file, ui);
  infra::FileLastDirectoriesService service(store);
  EXPECT_EQ(service.get("cartridge"), "D:\\roms");
  EXPECT_EQ(service.get("floppy"), "D:\\disks");
  EXPECT_FALSE(service.get("bad purpose"));
  EXPECT_FALSE(service.get("tape"));
}

TEST(MemoryDroppedFilesService, NumbersFromOneAndForgetsTheOldestBeyondSixtyFour) {
  infra::MemoryDroppedFilesService service;
  const auto first = service.registerFiles({"D:\\a\\x.nes", "D:\\a\\y.img"});
  EXPECT_EQ(first[0].id, 1);
  EXPECT_EQ(first[1].id, 2);
  EXPECT_EQ(first[1].name, "y.img");
  EXPECT_EQ(service.resolve(2), "D:\\a\\y.img");
  std::vector<std::string> many;
  for (int i = 0; i < 64; ++i) many.push_back("D:\\m\\" + std::to_string(i) + ".nes");
  static_cast<void>(service.registerFiles(many));
  EXPECT_FALSE(service.resolve(1));
  EXPECT_FALSE(service.resolve(2));
  EXPECT_TRUE(service.resolve(66));
}

TEST(UnsupportedAppUpdater, IsUnsupportedAndNeverInstalls) {
  infra::UnsupportedAppUpdater updater;
  EXPECT_EQ(updater.status().state, "unsupported");
  EXPECT_EQ(updater.check().state, "unsupported");
  EXPECT_FALSE(updater.install());
}

TEST(LatestFrameSlot, DeliversTheNewestFrameAndCountsEachFrameOnce) {
  platform::UiThread ui;
  infra::LatestFrameSlot slot(ui);
  int notified = 0;
  slot.setOnFrame([&] { ++notified; });
  const std::uint32_t a[4] = {1, 2, 3, 4};
  const std::uint32_t b[4] = {5, 6, 7, 8};
  slot.publish(2, 2, 1, a);
  slot.publish(2, 2, 2, b);
  ASSERT_TRUE(pumpUntil([&] { return notified > 0; }, 2000));
  EXPECT_EQ(notified, 1);

  contracts::FrameImage frame;
  ASSERT_TRUE(slot.takeLatest(frame));
  EXPECT_EQ(frame.number, 2u);
  EXPECT_EQ(frame.pixels, (std::vector<std::uint32_t>{5, 6, 7, 8}));
  EXPECT_FALSE(slot.takeLatest(frame));
  const auto stats = slot.stats();
  EXPECT_EQ(stats.published, 2u);
  EXPECT_EQ(stats.consumed, 1u);
  EXPECT_EQ(stats.dropped, 1u);
}

TEST(LatestFrameSlot, NotifiesAgainAfterTheFrameWasTaken) {
  platform::UiThread ui;
  infra::LatestFrameSlot slot(ui);
  contracts::FrameImage frame;
  int notified = 0;
  slot.setOnFrame([&] {
    ++notified;
    static_cast<void>(slot.takeLatest(frame));
  });
  const std::uint32_t a[1] = {9};
  slot.publish(1, 1, 1, a);
  ASSERT_TRUE(pumpUntil([&] { return notified == 1; }, 2000));
  slot.publish(1, 1, 2, a);
  ASSERT_TRUE(pumpUntil([&] { return notified == 2; }, 2000));
  EXPECT_EQ(slot.stats().dropped, 0u);
  EXPECT_EQ(slot.stats().consumed, 2u);
}

TEST(LatestFrameSlot, FramesFromAnotherThreadAreHandedToTheUiThread) {
  platform::UiThread ui;
  infra::LatestFrameSlot slot(ui);
  std::thread::id notifiedOn;
  bool notified = false;
  slot.setOnFrame([&] {
    notifiedOn = std::this_thread::get_id();
    notified = true;
  });
  const std::uint32_t a[1] = {1};
  std::thread producer([&] { slot.publish(1, 1, 1, a); });
  producer.join();
  ASSERT_TRUE(pumpUntil([&] { return notified; }, 2000));
  EXPECT_EQ(notifiedOn, std::this_thread::get_id());
}

/** 真实核心的测试夹具。 */
struct CoreFixture {
  test::TempDirectory directory;
  platform::UiThread ui;
  infra::LatestFrameSlot frames{ui};
  test::FakeErrorLogger logger;
  infra::InProcessCoreGateway gateway{infra::InProcessCoreOptions{platform::pathToUtf8(directory.path()), "9.9.9-test", std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt}, ui, frames, logger};

  /** 启动并等到就绪。 */
  bool startAndWait() {
    gateway.start();
    return pumpUntil([&] { return gateway.status().state == contracts::CoreState::Running; });
  }
};

TEST(InProcessCoreGateway, StartsTheRealCoreAndAnswersRequests) {
  CoreFixture f;
  std::vector<contracts::CoreState> states;
  const auto unsubscribe = f.gateway.onStatusChange([&](const contracts::CoreStatus& status) { states.push_back(status.state); });
  EXPECT_EQ(f.gateway.status().state, contracts::CoreState::Stopped);
  ASSERT_TRUE(f.startAndWait());
  EXPECT_EQ(states, (std::vector<contracts::CoreState>{contracts::CoreState::Starting, contracts::CoreState::Running}));
  EXPECT_FALSE(f.gateway.status().coreVersion.empty());
  EXPECT_GT(f.gateway.status().requests.size(), 50u);

  std::optional<contracts::CoreReply> reply;
  f.gateway.request("system.version", json::object(), [&](contracts::CoreReply value) { reply = std::move(value); });
  ASSERT_TRUE(pumpUntil([&] { return reply.has_value(); }));
  ASSERT_TRUE(reply->ok);
  EXPECT_EQ(reply->result["appVersion"], "9.9.9-test");
  EXPECT_EQ(reply->result["coreVersion"], f.gateway.status().coreVersion);
  unsubscribe();
}

TEST(InProcessCoreGateway, RequestsMadeWhileStartingAreQueuedAndAnsweredInOrder) {
  CoreFixture f;
  f.gateway.start();
  EXPECT_EQ(f.gateway.status().state, contracts::CoreState::Starting);
  std::vector<std::string> order;
  for (const char* name : {"system.version", "runtime.status"}) {
    f.gateway.request(name, json::object(), [&order, name](contracts::CoreReply reply) {
      EXPECT_TRUE(reply.ok) << name;
      order.push_back(name);
    });
  }
  EXPECT_TRUE(order.empty());
  ASSERT_TRUE(pumpUntil([&] { return order.size() == 2; }));
  EXPECT_EQ(order.front(), "system.version");
}

TEST(InProcessCoreGateway, CoreErrorsKeepTheirCodes) {
  CoreFixture f;
  ASSERT_TRUE(f.startAndWait());
  std::optional<contracts::CoreReply> unknown;
  f.gateway.request("nope.nothing", json::object(), [&](contracts::CoreReply value) { unknown = std::move(value); });
  std::optional<contracts::CoreReply> invalid;
  f.gateway.request("preferences.set", {{"id", 5}}, [&](contracts::CoreReply value) { invalid = std::move(value); });
  ASSERT_TRUE(pumpUntil([&] { return unknown && invalid; }));
  EXPECT_FALSE(unknown->ok);
  EXPECT_EQ(unknown->error.code, "core.unknownRequest");
  EXPECT_FALSE(invalid->ok);
  EXPECT_EQ(invalid->error.code, "core.invalidParams");
}

TEST(InProcessCoreGateway, CoreEventsReachSubscribersOnTheUiThread) {
  CoreFixture f;
  std::vector<std::string> names;
  std::thread::id eventThread;
  const auto unsubscribe = f.gateway.onEvent([&](const std::string& name, const json&) {
    names.push_back(name);
    eventThread = std::this_thread::get_id();
  });
  ASSERT_TRUE(f.startAndWait());
  // 主题设置变化时核心发 preferences.changed 事件；两个取值各设一次，总有一次与当前值不同
  for (const char* value : {"light", "dark"}) {
    bool done = false;
    f.gateway.request("preferences.set", json{{"id", "appearance.theme"}, {"value", value}}, [&](contracts::CoreReply) { done = true; });
    ASSERT_TRUE(pumpUntil([&] { return done; }));
  }
  ASSERT_TRUE(pumpUntil([&] { return !names.empty(); }));
  EXPECT_EQ(names.front(), "preferences.changed");
  EXPECT_EQ(eventThread, std::this_thread::get_id());
  unsubscribe();
}

TEST(InProcessCoreGateway, RequestsAfterStopFailWithUnavailable) {
  CoreFixture f;
  ASSERT_TRUE(f.startAndWait());
  f.gateway.stop();
  EXPECT_EQ(f.gateway.status().state, contracts::CoreState::Stopped);
  std::optional<contracts::CoreReply> reply;
  f.gateway.request("system.version", json::object(), [&](contracts::CoreReply value) { reply = std::move(value); });
  ASSERT_TRUE(pumpUntil([&] { return reply.has_value(); }, 2000));
  EXPECT_FALSE(reply->ok);
  EXPECT_EQ(reply->error.code, "core.unavailable");
  EXPECT_NO_THROW(f.gateway.sendInput({{"type", "key"}, {"code", "KeyW"}, {"down", true}}));
}

TEST(InProcessCoreGateway, StopWhileStartingFailsTheQueuedRequests) {
  CoreFixture f;
  f.gateway.start();
  std::optional<contracts::CoreReply> reply;
  f.gateway.request("system.version", json::object(), [&](contracts::CoreReply value) { reply = std::move(value); });
  f.gateway.stop();
  ASSERT_TRUE(reply.has_value());
  EXPECT_FALSE(reply->ok);
  EXPECT_EQ(reply->error.code, "core.unavailable");
}

TEST(InProcessCoreGateway, CoreThatCannotStartEndsInFailedAndIsLogged) {
  // 没有用户数据目录，核心拒绝启动
  platform::UiThread ui;
  infra::LatestFrameSlot frames(ui);
  test::FakeErrorLogger logger;
  infra::InProcessCoreGateway gateway(infra::InProcessCoreOptions{"", "1.0.0", std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt}, ui, frames, logger);
  std::optional<contracts::CoreReply> queued;
  gateway.start();
  gateway.request("system.version", json::object(), [&](contracts::CoreReply value) { queued = std::move(value); });
  ASSERT_TRUE(pumpUntil([&] { return gateway.status().state == contracts::CoreState::Failed; }));
  EXPECT_FALSE(gateway.status().message.empty());
  ASSERT_TRUE(queued.has_value());
  EXPECT_EQ(queued->error.code, "core.unavailable");
  ASSERT_EQ(logger.entries.size(), 1u);
  EXPECT_EQ(logger.entries[0].code, "core.internal");
}

}  // namespace
