/*
 * ------------------------------------------------------------------------
 * 名称：cartridge-manager-test.cpp
 * 说明：卡带服务的测试：从磁盘加载 NES 文件并运行、powerOn 与 resume 参数、cartridges.loaded 事件、取出与卡带信息、各种错误路径（文件不存在、不是普通文件、不是 iNES、被截断、没有匹配的插件、文件太大）及失败时原有卡带保持不变。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：用真实的运行服务、nrom 插件与磁盘文件读取；文件写在临时目录里。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>

#include "../support/runtime-test-support.hpp"
#include "bootstrap/machines/machine-catalog.hpp"
#include "contracts/cartridges/cartridge-error.hpp"
#include "infra/cartridges/disk-cartridge-file-reader.hpp"
#include "logic/cartridges/cartridge-manager.hpp"
#include "logic/runtime/simulation-runtime.hpp"
#include "shared/errors/core-error.hpp"

namespace {

namespace api = ember::contracts::cartridges;
using ember::test::SyncEventSink;
using ember::test::waitFor;

/** 路径的 UTF-8 文本。 */
std::string utf8(const std::filesystem::path& path) {
  const auto text = path.u8string();
  return std::string(text.begin(), text.end());
}

/** 真实的卡带服务加运行服务，卡带文件写在临时目录。 */
struct Rig {
  Rig() {
    registry = std::make_shared<ember::infra::mappers::PluginRegistry>(ember::test::fcRegistryOptions());
    registry->installPackage(ember::test::officialPluginPackage("nrom"));
    ember::logic::runtime::SimulationRuntimeOptions options;
    options.machines = std::make_shared<ember::bootstrap::MachineCatalog>();
    options.mappers = registry;
    options.frames = frames;
    options.events = events;
    options.errors = std::make_shared<ember::test::RecordingErrorLog>();
    options.framesPerSecond = 0;
    runtime = std::make_shared<ember::logic::runtime::SimulationRuntime>(std::move(options));
    manager = std::make_unique<ember::logic::cartridges::CartridgeManager>(std::make_shared<ember::infra::cartridges::DiskCartridgeFileReader>(),
                                                                          std::make_shared<ember::logic::cartridges::InesCartridgeLoader>(registry), runtime, events);
  }

  /** 把 NES 文件写到临时目录，返回它的路径（UTF-8）。 */
  [[nodiscard]] std::string write(const std::string& name, const std::vector<std::uint8_t>& bytes) const { return utf8(dir.write(name, bytes)); }

  [[nodiscard]] static std::vector<std::uint8_t> idleRom(unsigned mapper = 0) {
    ember::test::Assembler main(0x8000);
    main.jmpSelf();
    return ember::test::makeInesFile(main.bytes(), mapper);
  }

  ember::test::TempDir dir;
  std::shared_ptr<ember::infra::mappers::PluginRegistry> registry;
  std::shared_ptr<ember::test::FrameCapture> frames = std::make_shared<ember::test::FrameCapture>();
  std::shared_ptr<SyncEventSink> events = std::make_shared<SyncEventSink>();
  std::shared_ptr<ember::logic::runtime::SimulationRuntime> runtime;
  std::unique_ptr<ember::logic::cartridges::CartridgeManager> manager;
};

api::CartridgesLoadParams loadParams(const std::string& path, std::optional<bool> powerOn = std::nullopt, std::optional<bool> resume = std::nullopt) {
  api::CartridgesLoadParams params;
  params.path = path;
  params.powerOn = powerOn;
  params.resume = resume;
  return params;
}

/** 调用并取出错误码；没有抛出时返回空串。 */
template <typename Function>
std::string errorCodeOf(Function&& call) {
  try {
    call();
  } catch (const ember::shared::CoreError& error) {
    return error.code();
  }
  return {};
}

TEST(CartridgeManager, LoadsAFileFromDiskPowersOnAndRuns) {
  Rig rig;
  const auto path = rig.write("idle.nes", Rig::idleRom());
  const auto result = rig.manager->load(loadParams(path));
  EXPECT_EQ(result.cartridge.path, path);
  EXPECT_EQ(result.cartridge.pluginId, "nrom");
  EXPECT_EQ(result.cartridge.mapper, 0);
  EXPECT_EQ(result.state.power, "on");
  EXPECT_EQ(result.state.mode, "running");
  ASSERT_TRUE(waitFor([&] { return rig.frames->count() >= 5; }));

  const auto loaded = rig.events->last("cartridges.loaded");
  EXPECT_EQ(loaded["cartridge"]["path"], path);
  EXPECT_EQ(loaded["cartridge"]["pluginId"], "nrom");
  EXPECT_EQ(rig.events->count("cartridges.loaded"), 1);

  const auto info = rig.manager->info();
  EXPECT_EQ(info.cartridge.path, path);
  EXPECT_EQ(info.cartridge.pluginName, "NROM");
  EXPECT_EQ(info.cartridge.pluginVersion, "1.0.0");
  EXPECT_EQ(info.cartridge.storage.size(), 3U);
  EXPECT_EQ(rig.runtime->status().cartridge->path, path);
}

TEST(CartridgeManager, LoadsFilesWithNonAsciiPaths) {
  Rig rig;
  const auto file = rig.dir.path() / std::filesystem::path(u8"超级卡带 测试.nes");
  const auto bytes = Rig::idleRom();
  std::ofstream(file, std::ios::binary).write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  const auto path = utf8(file);
  const auto result = rig.manager->load(loadParams(path));
  EXPECT_EQ(result.cartridge.path, path);
}

TEST(CartridgeManager, PowerOnAndResumeParametersControlTheStateAfterLoading) {
  Rig rig;
  const auto path = rig.write("idle.nes", Rig::idleRom());
  const auto off = rig.manager->load(loadParams(path, false));
  EXPECT_EQ(off.state.power, "off");
  EXPECT_EQ(off.state.mode, "paused");
  const auto paused = rig.manager->load(loadParams(path, true, false));
  EXPECT_EQ(paused.state.power, "on");
  EXPECT_EQ(paused.state.mode, "paused");
  const auto running = rig.manager->load(loadParams(path, true, true));
  EXPECT_EQ(running.state.mode, "running");
  EXPECT_EQ(rig.events->count("cartridges.loaded"), 3);
}

TEST(CartridgeManager, EjectingEmptiesTheSlotAndInfoReportsIt) {
  Rig rig;
  EXPECT_EQ(errorCodeOf([&] { static_cast<void>(rig.manager->info()); }), api::ERROR_EMPTY);
  EXPECT_EQ(rig.manager->eject().state.power, "off");  // 空槽取出没有效果

  rig.manager->load(loadParams(rig.write("idle.nes", Rig::idleRom())));
  const auto ejected = rig.manager->eject();
  EXPECT_EQ(ejected.state.power, "off");
  EXPECT_EQ(ejected.state.mode, "paused");
  EXPECT_EQ(errorCodeOf([&] { static_cast<void>(rig.manager->info()); }), api::ERROR_EMPTY);
  EXPECT_FALSE(rig.runtime->status().cartridge.has_value());
}

TEST(CartridgeManager, FailedLoadsReportTheReasonAndKeepTheCurrentCartridge) {
  Rig rig;
  const auto good = rig.write("good.nes", Rig::idleRom());
  rig.manager->load(loadParams(good));
  ASSERT_TRUE(waitFor([&] { return rig.frames->count() >= 3; }));

  EXPECT_EQ(errorCodeOf([&] { rig.manager->load(loadParams(utf8(rig.dir.path() / "does-not-exist.nes"))); }), api::ERROR_FILE_NOT_FOUND);
  EXPECT_EQ(errorCodeOf([&] { rig.manager->load(loadParams(utf8(rig.dir.path()))); }), api::ERROR_FILE_UNREADABLE);

  EXPECT_EQ(errorCodeOf([&] { rig.manager->load(loadParams(rig.write("not-nes.nes", {'M', 'Z', 0x90, 0, 3, 0, 0, 0, 4, 0, 0, 0, 0xFF, 0xFF, 0, 0}))); }), api::ERROR_NOT_INES);
  EXPECT_EQ(errorCodeOf([&] { rig.manager->load(loadParams(rig.write("empty.nes", {}))); }), api::ERROR_NOT_INES);

  auto truncated = Rig::idleRom();
  truncated.resize(100);
  EXPECT_EQ(errorCodeOf([&] { rig.manager->load(loadParams(rig.write("short.nes", truncated))); }), api::ERROR_TRUNCATED);

  // mapper 4 没有插件。
  EXPECT_EQ(errorCodeOf([&] { rig.manager->load(loadParams(rig.write("mmc3.nes", Rig::idleRom(4)))); }), "mappers.noMatch");

  // 失败都没有改动正在运行的卡带，也没有发多余的事件。
  EXPECT_EQ(rig.runtime->status().cartridge->path, good);
  EXPECT_EQ(rig.runtime->status().mode, "running");
  EXPECT_EQ(rig.events->count("cartridges.loaded"), 1);
}

TEST(CartridgeManager, FilesTooLargeToBeCartridgesAreRejectedWithoutReadingThem) {
  Rig rig;
  const auto path = rig.write("big.nes", {'N', 'E', 'S', 0x1A});
  std::filesystem::resize_file(std::filesystem::path(std::u8string(path.begin(), path.end())), ember::infra::cartridges::DiskCartridgeFileReader::MAX_FILE_SIZE + 1);
  EXPECT_EQ(errorCodeOf([&] { rig.manager->load(loadParams(path)); }), api::ERROR_UNSUPPORTED_FORMAT);
}

}  // namespace
