/*
 * ------------------------------------------------------------------------
 * 名称：machine-manager-test.cpp
 * 说明：机型服务的测试：机型清单、添加与替换 BIOS（含文件校验与失败时原有的保持不变）、移除、machines.changed 事件、整机机型开机（含各种错误）与开机后的运行状态。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：用真实的运行服务、sb2000 主机与磁盘上的 BIOS 存取；BIOS 文件是合成的 iNES 文件（512KB 的 PRG，复位向量指向一个原地跳转的小程序），写在临时目录里。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "../support/runtime-test-support.hpp"
#include "bootstrap/machines/machine-catalog.hpp"
#include "contracts/machines/machines-error.hpp"
#include "infra/machines/disk-bios-storage.hpp"
#include "logic/machines/machine-manager.hpp"
#include "logic/runtime/simulation-runtime.hpp"
#include "shared/errors/core-error.hpp"

namespace {

namespace api = ember::contracts::machines;
using ember::test::SyncEventSink;
using ember::test::waitFor;

/** 合成的 sb2000 BIOS 文件：iNES 头加 512KB 的 PRG，复位向量指向 $C000，那里是一条原地跳转。 */
std::vector<std::uint8_t> biosFile(std::size_t prgUnits = 32) {
  std::vector<std::uint8_t> file(16 + prgUnits * 16 * 1024, 0xEA);
  file[0] = 'N';
  file[1] = 'E';
  file[2] = 'S';
  file[3] = 0x1A;
  file[4] = static_cast<std::uint8_t>(prgUnits);
  for (std::size_t index = 5; index < 16; ++index) file[index] = 0;
  if (prgUnits == 32) {
    const std::size_t prg = 16;
    file[prg + 0x7FFC] = 0x00;
    file[prg + 0x7FFD] = 0xC0;
    file[prg + 0x4000] = 0x4C;  // JMP $C000
    file[prg + 0x4001] = 0x00;
    file[prg + 0x4002] = 0xC0;
  }
  return file;
}

/** 纯 BIOS 文件：与 biosFile 的 PRG 相同，没有 iNES 文件头。 */
std::vector<std::uint8_t> cleanBiosFile() {
  const auto file = biosFile();
  return std::vector<std::uint8_t>(file.begin() + 16, file.end());
}

/** 真实的机型服务加运行服务，BIOS 保存在临时目录。 */
struct Rig {
  Rig() {
    registry = std::make_shared<ember::infra::mappers::PluginRegistry>(ember::test::fcRegistryOptions());
    ember::logic::runtime::SimulationRuntimeOptions options;
    options.machines = std::make_shared<ember::bootstrap::MachineCatalog>();
    options.mappers = registry;
    options.frames = frames;
    options.events = events;
    options.errors = std::make_shared<ember::test::RecordingErrorLog>();
    options.framesPerSecond = 0;
    runtime = std::make_shared<ember::logic::runtime::SimulationRuntime>(std::move(options));
    storage = std::make_shared<ember::infra::machines::DiskBiosStorage>(dir.path() / "bios");
    manager = std::make_unique<ember::logic::machines::MachineManager>(storage, runtime, events);
  }

  /** 把文件写到临时目录，返回路径（UTF-8）。 */
  [[nodiscard]] std::string write(const std::string& name, const std::vector<std::uint8_t>& bytes) const {
    const auto text = dir.write(name, bytes).u8string();
    return std::string(text.begin(), text.end());
  }

  [[nodiscard]] api::MachineInfo find(const std::string& id) const {
    for (const auto& item : manager->list().machines) {
      if (item.id == id) return item;
    }
    throw std::runtime_error("清单里没有 " + id);
  }

  api::MachinesSetBiosParams setParams(const std::string& machine, const std::string& path) const {
    api::MachinesSetBiosParams params;
    params.machine = machine;
    params.path = path;
    return params;
  }

  ember::test::TempDir dir;
  std::shared_ptr<ember::infra::mappers::PluginRegistry> registry;
  std::shared_ptr<ember::test::FrameCapture> frames = std::make_shared<ember::test::FrameCapture>();
  std::shared_ptr<SyncEventSink> events = std::make_shared<SyncEventSink>();
  std::shared_ptr<ember::logic::runtime::SimulationRuntime> runtime;
  std::shared_ptr<ember::infra::machines::DiskBiosStorage> storage;
  std::unique_ptr<ember::logic::machines::MachineManager> manager;
};

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

TEST(MachineManager, ListsTheFiveMachinesInFixedOrder) {
  Rig rig;
  const auto machines = rig.manager->list().machines;
  ASSERT_EQ(machines.size(), 5U);
  const char* ids[] = {"fc", "sb97", "sb2000", "bbk10", "bbk20"};
  for (std::size_t index = 0; index < machines.size(); ++index) EXPECT_EQ(machines[index].id, ids[index]);
  EXPECT_FALSE(machines[0].needsBios);
  EXPECT_TRUE(machines[0].supported);
  EXPECT_TRUE(machines[2].needsBios);
  EXPECT_TRUE(machines[2].supported);
  EXPECT_FALSE(machines[1].supported);
  EXPECT_FALSE(machines[2].bios.has_value());
}

TEST(MachineManager, SettingABiosKeepsItAndPublishesAChange) {
  Rig rig;
  const auto path = rig.write("SB2K_BIOS.NES", biosFile());
  const auto result = rig.manager->setBios(rig.setParams("sb2000", path));
  ASSERT_TRUE(result.machine.bios.has_value());
  EXPECT_EQ(result.machine.bios->fileName, "SB2K_BIOS.NES");
  EXPECT_EQ(result.machine.bios->size, static_cast<std::int64_t>(16 + 512 * 1024));
  EXPECT_EQ(rig.events->count("machines.changed"), 1);
  EXPECT_EQ(rig.events->last("machines.changed")["machine"], "sb2000");
  EXPECT_TRUE(rig.find("sb2000").bios.has_value());
  EXPECT_TRUE(std::filesystem::exists(rig.dir.path() / "bios" / "sb2000" / "SB2K_BIOS.NES"));
}

TEST(MachineManager, ReplacingABiosDeletesTheOldFile) {
  Rig rig;
  rig.manager->setBios(rig.setParams("sb2000", rig.write("first.nes", biosFile())));
  rig.manager->setBios(rig.setParams("sb2000", rig.write("second.nes", biosFile())));
  EXPECT_EQ(rig.find("sb2000").bios->fileName, "second.nes");
  EXPECT_FALSE(std::filesystem::exists(rig.dir.path() / "bios" / "sb2000" / "first.nes"));
  EXPECT_EQ(rig.events->count("machines.changed"), 2);
}

TEST(MachineManager, SettingABiosRejectsBadInputAndKeepsTheOldOne) {
  Rig rig;
  rig.manager->setBios(rig.setParams("sb2000", rig.write("good.nes", biosFile())));
  EXPECT_EQ(errorCodeOf([&] { rig.manager->setBios(rig.setParams("nope", rig.write("a.nes", biosFile()))); }), api::ERROR_NOT_FOUND);
  EXPECT_EQ(errorCodeOf([&] { rig.manager->setBios(rig.setParams("fc", rig.write("b.nes", biosFile()))); }), api::ERROR_BIOS_NOT_NEEDED);
  EXPECT_EQ(errorCodeOf([&] { rig.manager->setBios(rig.setParams("sb97", rig.write("c.nes", biosFile()))); }), api::ERROR_UNSUPPORTED);
  EXPECT_EQ(errorCodeOf([&] { rig.manager->setBios(rig.setParams("sb2000", (rig.dir.path() / "missing.nes").string())); }), api::ERROR_FILE_NOT_FOUND);
  EXPECT_EQ(errorCodeOf([&] { rig.manager->setBios(rig.setParams("sb2000", rig.write("small.nes", biosFile(16)))); }), api::ERROR_BIOS_INVALID);
  EXPECT_EQ(errorCodeOf([&] { rig.manager->setBios(rig.setParams("sb2000", rig.write("text.nes", std::vector<std::uint8_t>(100, 'x')))); }), api::ERROR_BIOS_INVALID);
  auto truncated = biosFile();
  truncated.resize(1000);
  EXPECT_EQ(errorCodeOf([&] { rig.manager->setBios(rig.setParams("sb2000", rig.write("cut.nes", truncated))); }), api::ERROR_BIOS_INVALID);
  EXPECT_EQ(rig.find("sb2000").bios->fileName, "good.nes") << "失败时原有的 BIOS 不变";
  EXPECT_EQ(rig.events->count("machines.changed"), 1);
}

TEST(MachineManager, ACleanBiosFileWithoutAHeaderIsAcceptedAndBoots) {
  Rig rig;
  const auto result = rig.manager->setBios(rig.setParams("sb2000", rig.write("SB2K_BIOS.bin", cleanBiosFile())));
  ASSERT_TRUE(result.machine.bios.has_value());
  EXPECT_EQ(result.machine.bios->fileName, "SB2K_BIOS.bin");
  EXPECT_EQ(result.machine.bios->size, static_cast<std::int64_t>(512 * 1024));
  api::MachinesBootParams params;
  params.machine = "sb2000";
  rig.manager->boot(params);
  ASSERT_TRUE(waitFor([&] { return rig.frames->count() >= 5; }));
  EXPECT_EQ(rig.runtime->status().machine, "sb2000");
}

TEST(MachineManager, ACleanBiosFileMustBeExactlyTheMachinesPrgSize) {
  Rig rig;
  auto shortFile = cleanBiosFile();
  shortFile.pop_back();
  auto longFile = cleanBiosFile();
  longFile.push_back(0);
  EXPECT_EQ(errorCodeOf([&] { rig.manager->setBios(rig.setParams("sb2000", rig.write("short.bin", shortFile))); }), api::ERROR_BIOS_INVALID);
  EXPECT_EQ(errorCodeOf([&] { rig.manager->setBios(rig.setParams("sb2000", rig.write("long.bin", longFile))); }), api::ERROR_BIOS_INVALID);
  EXPECT_EQ(errorCodeOf([&] { rig.manager->setBios(rig.setParams("bbk10", rig.write("wrong-machine.bin", cleanBiosFile()))); }), api::ERROR_BIOS_INVALID);
  EXPECT_FALSE(rig.find("sb2000").bios.has_value());
}

TEST(MachineManager, RemovingABiosIsIdempotent) {
  Rig rig;
  rig.manager->setBios(rig.setParams("sb2000", rig.write("good.nes", biosFile())));
  api::MachinesRemoveBiosParams params;
  params.machine = "sb2000";
  EXPECT_FALSE(rig.manager->removeBios(params).machine.bios.has_value());
  EXPECT_EQ(rig.events->count("machines.changed"), 2);
  EXPECT_FALSE(rig.manager->removeBios(params).machine.bios.has_value());
  EXPECT_EQ(rig.events->count("machines.changed"), 2) << "没有 BIOS 时没有效果，不发事件";
  params.machine = "fc";
  EXPECT_EQ(errorCodeOf([&] { rig.manager->removeBios(params); }), api::ERROR_BIOS_NOT_NEEDED);
  params.machine = "nope";
  EXPECT_EQ(errorCodeOf([&] { rig.manager->removeBios(params); }), api::ERROR_NOT_FOUND);
}

TEST(MachineManager, BootNeedsAMachineWithABios) {
  Rig rig;
  api::MachinesBootParams params;
  params.machine = "nope";
  EXPECT_EQ(errorCodeOf([&] { rig.manager->boot(params); }), api::ERROR_NOT_FOUND);
  params.machine = "fc";
  EXPECT_EQ(errorCodeOf([&] { rig.manager->boot(params); }), api::ERROR_CARTRIDGE_REQUIRED);
  params.machine = "sb97";
  EXPECT_EQ(errorCodeOf([&] { rig.manager->boot(params); }), api::ERROR_UNSUPPORTED);
  params.machine = "sb2000";
  EXPECT_EQ(errorCodeOf([&] { rig.manager->boot(params); }), api::ERROR_NO_BIOS);
  EXPECT_EQ(rig.runtime->status().machine, std::nullopt) << "失败时没有装配主机";
}

TEST(MachineManager, BootRunsTheMachineWithoutACartridge) {
  Rig rig;
  rig.manager->setBios(rig.setParams("sb2000", rig.write("good.nes", biosFile())));
  api::MachinesBootParams params;
  params.machine = "sb2000";
  const auto result = rig.manager->boot(params);
  EXPECT_EQ(result.machine.id, "sb2000");
  EXPECT_EQ(result.state.power, "on");
  EXPECT_EQ(result.state.mode, "running");
  ASSERT_TRUE(waitFor([&] { return rig.frames->count() >= 5; }));
  const auto status = rig.runtime->status();
  EXPECT_EQ(status.machine, "sb2000");
  EXPECT_FALSE(status.cartridge.has_value()) << "整机机型没有卡带";
  EXPECT_EQ(rig.events->count("cartridges.loaded"), 0);
  EXPECT_GE(rig.events->count("runtime.stateChanged"), 1);
  EXPECT_EQ(rig.events->last("runtime.stateChanged")["reason"], "load");
}

TEST(MachineManager, BootCanLeaveTheMachinePoweredOff) {
  Rig rig;
  rig.manager->setBios(rig.setParams("sb2000", rig.write("good.nes", biosFile())));
  api::MachinesBootParams params;
  params.machine = "sb2000";
  params.powerOn = false;
  const auto result = rig.manager->boot(params);
  EXPECT_EQ(result.state.power, "off");
  EXPECT_EQ(rig.runtime->status().machine, "sb2000");
  const auto on = rig.runtime->powerOn(false);
  EXPECT_EQ(on.power, "on");
  EXPECT_EQ(on.mode, "paused");
}

}  // namespace
