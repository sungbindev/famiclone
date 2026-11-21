/*
 * ------------------------------------------------------------------------
 * 名称：machine-manager-test.cpp
 * 说明：机型服务的测试：机型清单、添加与替换 ROM（含文件校验与失败时原有的保持不变）、从目录添加、ROM 版本、移除、machines.changed 事件、整机机型开机（含各种错误）与开机后的运行状态。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：用真实的运行服务、sb2000 主机与磁盘上的 ROM 存取；合成的 ROM 是不带文件头的纯 bin（复位向量指向一个原地跳转的小程序），写在临时目录里；
 *       “从目录添加”与版本的用例用仓库 ember/roms 里的真实 ROM（按内容的校验值认机型），文件不存在时跳过。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "../support/real-rom-support.hpp"
#include "../support/runtime-test-support.hpp"
#include "bootstrap/machines/machine-catalog.hpp"
#include "contracts/machines/machines-error.hpp"
#include "infra/machines/disk-rom-storage.hpp"
#include "logic/machines/known-roms.hpp"
#include "logic/machines/machine-manager.hpp"
#include "logic/runtime/simulation-runtime.hpp"
#include "shared/errors/core-error.hpp"

namespace {

namespace api = ember::contracts::machines;
using ember::test::SyncEventSink;
using ember::test::waitFor;

/** 合成的 ROM：prgUnits 个 16KB，复位向量指向 $C000，那里是一条原地跳转（32 个单位是 sb2000 的 512KB，64 个是 sb97 与裕兴的 1024KB）。 */
std::vector<std::uint8_t> romFile(std::size_t prgUnits = 32) {
  std::vector<std::uint8_t> file(prgUnits * 16 * 1024, 0xEA);
  if (prgUnits == 32) {
    file[0x7FFC] = 0x00;
    file[0x7FFD] = 0xC0;
    file[0x4000] = 0x4C;  // JMP $C000
    file[0x4001] = 0x00;
    file[0x4002] = 0xC0;
  } else if (prgUnits == 64) {
    // sb97：第 0 页固定在 $C000，复位向量和原地跳转都在这一页。
    file[0x3FFC] = 0x00;
    file[0x3FFD] = 0xC0;
    file[0x0000] = 0x4C;  // JMP $C000
    file[0x0001] = 0x00;
    file[0x0002] = 0xC0;
  }
  return file;
}

/** 真实的 ROM 文件；仓库里没有时为空。 */
std::vector<std::uint8_t> realRom(const std::string& name) { return ember::test::readMachineRom(name); }

/** 真实的机型服务加运行服务，ROM 保存在临时目录。 */
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
    storage = std::make_shared<ember::infra::machines::DiskRomStorage>(dir.path() / "roms");
    manager = std::make_unique<ember::logic::machines::MachineManager>(storage, runtime, events);
  }

  /** 把文件写到临时目录，返回路径（UTF-8）。 */
  [[nodiscard]] std::string write(const std::string& name, const std::vector<std::uint8_t>& bytes) const {
    const auto text = dir.write(name, bytes).u8string();
    return std::string(text.begin(), text.end());
  }

  /** 与 write 相同，不需要路径时用。 */
  void put(const std::string& name, const std::vector<std::uint8_t>& bytes) const { static_cast<void>(write(name, bytes)); }

  /** 临时目录里放 ROM 的子目录（UTF-8）。 */
  [[nodiscard]] std::string packDirectory() const {
    const auto text = (dir.path() / "pack").u8string();
    return std::string(text.begin(), text.end());
  }

  [[nodiscard]] api::MachineInfo find(const std::string& id) const {
    for (const auto& item : manager->list().machines) {
      if (item.id == id) return item;
    }
    throw std::runtime_error("清单里没有 " + id);
  }

  api::MachinesSetRomParams setParams(const std::string& machine, const std::string& path) const {
    api::MachinesSetRomParams params;
    params.machine = machine;
    params.path = path;
    return params;
  }

  api::MachinesSetRomsParams setRomsParams() const {
    api::MachinesSetRomsParams params;
    params.path = packDirectory();
    return params;
  }

  ember::test::TempDir dir;
  std::shared_ptr<ember::infra::mappers::PluginRegistry> registry;
  std::shared_ptr<ember::test::FrameCapture> frames = std::make_shared<ember::test::FrameCapture>();
  std::shared_ptr<SyncEventSink> events = std::make_shared<SyncEventSink>();
  std::shared_ptr<ember::logic::runtime::SimulationRuntime> runtime;
  std::shared_ptr<ember::infra::machines::DiskRomStorage> storage;
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

/** 被跳过的文件对应的原因；没有时为空串。 */
std::string skippedReason(const api::MachinesSetRomsResult& result, const std::string& fileName) {
  for (const auto& item : result.skipped) {
    if (item.fileName == fileName) return item.reason;
  }
  return {};
}

TEST(MachineManager, ListsTheEightMachinesInFixedOrder) {
  Rig rig;
  const auto machines = rig.manager->list().machines;
  ASSERT_EQ(machines.size(), 8U);
  const char* ids[] = {"fc", "sb97", "sb2000", "bbg1", "bbg98", "pec9588", "pec586", "yuxing"};
  for (std::size_t index = 0; index < machines.size(); ++index) EXPECT_EQ(machines[index].id, ids[index]);
  EXPECT_EQ(machines[5].name, "金字塔 PEC-9588");
  EXPECT_EQ(machines[6].name, "金字塔 PEC-586、686");
  EXPECT_EQ(machines[7].name, "裕兴");
  for (std::size_t index = 1; index < machines.size(); ++index) {
    EXPECT_TRUE(machines[index].needsRom) << ids[index];
    EXPECT_TRUE(machines[index].supported) << ids[index];
  }
  EXPECT_FALSE(machines[0].needsRom);
  EXPECT_TRUE(machines[0].supported);
  EXPECT_FALSE(machines[2].rom.has_value());
}

TEST(MachineManager, SettingARomKeepsItAndPublishesAChange) {
  Rig rig;
  const auto path = rig.write("SB2K_ROM.bin", romFile());
  const auto result = rig.manager->setRom(rig.setParams("sb2000", path));
  ASSERT_TRUE(result.machine.rom.has_value());
  EXPECT_EQ(result.machine.rom->fileName, "SB2K_ROM.bin");
  EXPECT_EQ(result.machine.rom->size, static_cast<std::int64_t>(512 * 1024));
  EXPECT_EQ(result.machine.rom->label, "SB2K_ROM.bin") << "认不出的 ROM 显示文件名";
  EXPECT_EQ(rig.events->count("machines.changed"), 1);
  EXPECT_EQ(rig.events->last("machines.changed")["machine"], "sb2000");
  EXPECT_TRUE(rig.find("sb2000").rom.has_value());
  EXPECT_TRUE(std::filesystem::exists(rig.dir.path() / "roms" / "sb2000" / "SB2K_ROM.bin"));
}

TEST(MachineManager, ReplacingARomDeletesTheOldFile) {
  Rig rig;
  rig.manager->setRom(rig.setParams("sb2000", rig.write("first.bin", romFile())));
  rig.manager->setRom(rig.setParams("sb2000", rig.write("second.bin", romFile())));
  EXPECT_EQ(rig.find("sb2000").rom->fileName, "second.bin");
  EXPECT_FALSE(std::filesystem::exists(rig.dir.path() / "roms" / "sb2000" / "first.bin"));
  EXPECT_EQ(rig.events->count("machines.changed"), 2);
}

TEST(MachineManager, SettingARomRejectsBadInputAndKeepsTheOldOne) {
  Rig rig;
  rig.manager->setRom(rig.setParams("sb2000", rig.write("good.bin", romFile())));
  EXPECT_EQ(errorCodeOf([&] { rig.manager->setRom(rig.setParams("nope", rig.write("a.bin", romFile()))); }), api::ERROR_NOT_FOUND);
  EXPECT_EQ(errorCodeOf([&] { rig.manager->setRom(rig.setParams("fc", rig.write("b.bin", romFile()))); }), api::ERROR_ROM_NOT_NEEDED);
  EXPECT_EQ(errorCodeOf([&] { rig.manager->setRom(rig.setParams("sb97", rig.write("c.bin", romFile()))); }), api::ERROR_ROM_INVALID) << "sb97 要 1024KB，512KB 的不行";
  EXPECT_EQ(errorCodeOf([&] { rig.manager->setRom(rig.setParams("sb2000", (rig.dir.path() / "missing.bin").string())); }), api::ERROR_FILE_NOT_FOUND);
  EXPECT_EQ(errorCodeOf([&] { rig.manager->setRom(rig.setParams("sb2000", rig.write("small.bin", romFile(16)))); }), api::ERROR_ROM_INVALID);
  EXPECT_EQ(errorCodeOf([&] { rig.manager->setRom(rig.setParams("sb2000", rig.write("text.bin", std::vector<std::uint8_t>(100, 'x')))); }), api::ERROR_ROM_INVALID);
  auto truncated = romFile();
  truncated.resize(1000);
  EXPECT_EQ(errorCodeOf([&] { rig.manager->setRom(rig.setParams("sb2000", rig.write("cut.bin", truncated))); }), api::ERROR_ROM_INVALID);
  EXPECT_EQ(rig.find("sb2000").rom->fileName, "good.bin") << "失败时原有的 ROM 不变";
  EXPECT_EQ(rig.events->count("machines.changed"), 1);
}

TEST(MachineManager, ARomMustBeExactlyTheMachinesSizeAndHasNoHeader) {
  Rig rig;
  auto shortFile = romFile();
  shortFile.pop_back();
  auto longFile = romFile();
  longFile.push_back(0);
  auto withHeader = romFile();
  withHeader.insert(withHeader.begin(), {'N', 'E', 'S', 0x1A, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
  EXPECT_EQ(errorCodeOf([&] { rig.manager->setRom(rig.setParams("sb2000", rig.write("short.bin", shortFile))); }), api::ERROR_ROM_INVALID);
  EXPECT_EQ(errorCodeOf([&] { rig.manager->setRom(rig.setParams("sb2000", rig.write("long.bin", longFile))); }), api::ERROR_ROM_INVALID);
  EXPECT_EQ(errorCodeOf([&] { rig.manager->setRom(rig.setParams("sb2000", rig.write("header.bin", withHeader))); }), api::ERROR_ROM_INVALID) << "带 iNES 文件头的文件不再接受";
  EXPECT_EQ(errorCodeOf([&] { rig.manager->setRom(rig.setParams("bbg1", rig.write("wrong-machine.bin", romFile()))); }), api::ERROR_ROM_INVALID);
  EXPECT_FALSE(rig.find("sb2000").rom.has_value());
}

TEST(MachineManager, ARomBootsAndTheStatusCarriesItsLabel) {
  Rig rig;
  rig.manager->setRom(rig.setParams("sb2000", rig.write("SB2K_ROM.bin", romFile())));
  api::MachinesBootParams params;
  params.machine = "sb2000";
  rig.manager->boot(params);
  ASSERT_TRUE(waitFor([&] { return rig.frames->count() >= 5; }));
  EXPECT_EQ(rig.runtime->status().machine, "sb2000");
  EXPECT_EQ(rig.runtime->status().romLabel, "SB2K_ROM.bin");
}

TEST(MachineManager, RemovingARomIsIdempotent) {
  Rig rig;
  rig.manager->setRom(rig.setParams("sb2000", rig.write("good.bin", romFile())));
  api::MachinesRemoveRomParams params;
  params.machine = "sb2000";
  EXPECT_FALSE(rig.manager->removeRom(params).machine.rom.has_value());
  EXPECT_EQ(rig.events->count("machines.changed"), 2);
  EXPECT_FALSE(rig.manager->removeRom(params).machine.rom.has_value());
  EXPECT_EQ(rig.events->count("machines.changed"), 2) << "没有 ROM 时没有效果，不发事件";
  params.machine = "fc";
  EXPECT_EQ(errorCodeOf([&] { rig.manager->removeRom(params); }), api::ERROR_ROM_NOT_NEEDED);
  params.machine = "nope";
  EXPECT_EQ(errorCodeOf([&] { rig.manager->removeRom(params); }), api::ERROR_NOT_FOUND);
}

TEST(MachineManager, BootNeedsAMachineWithARom) {
  Rig rig;
  api::MachinesBootParams params;
  params.machine = "nope";
  EXPECT_EQ(errorCodeOf([&] { rig.manager->boot(params); }), api::ERROR_NOT_FOUND);
  params.machine = "fc";
  EXPECT_EQ(errorCodeOf([&] { rig.manager->boot(params); }), api::ERROR_CARTRIDGE_REQUIRED);
  params.machine = "sb97";
  EXPECT_EQ(errorCodeOf([&] { rig.manager->boot(params); }), api::ERROR_NO_ROM);
  params.machine = "sb2000";
  EXPECT_EQ(errorCodeOf([&] { rig.manager->boot(params); }), api::ERROR_NO_ROM);
  EXPECT_EQ(rig.runtime->status().machine, std::nullopt) << "失败时没有装配主机";
}

TEST(MachineManager, ASb97RomOfOneMegabyteIsAcceptedAndBoots) {
  Rig rig;
  const auto result = rig.manager->setRom(rig.setParams("sb97", rig.write("SB97.bin", romFile(64))));
  ASSERT_TRUE(result.machine.rom.has_value());
  EXPECT_EQ(result.machine.rom->size, static_cast<std::int64_t>(1024 * 1024));
  api::MachinesBootParams params;
  params.machine = "sb97";
  const auto booted = rig.manager->boot(params);
  EXPECT_EQ(booted.machine.id, "sb97");
  ASSERT_TRUE(waitFor([&] { return rig.frames->count() >= 5; }));
  EXPECT_EQ(rig.runtime->status().machine, "sb97");
}

TEST(MachineManager, AYuxingRomOfOneMegabyteIsAcceptedAndBoots) {
  Rig rig;
  EXPECT_EQ(errorCodeOf([&] { rig.manager->setRom(rig.setParams("yuxing", rig.write("small.bin", romFile()))); }), api::ERROR_ROM_INVALID) << "裕兴要 1024KB，512KB 的不行";
  const auto result = rig.manager->setRom(rig.setParams("yuxing", rig.write("YX.bin", romFile(64))));
  ASSERT_TRUE(result.machine.rom.has_value());
  EXPECT_EQ(result.machine.rom->size, static_cast<std::int64_t>(1024 * 1024));
  api::MachinesBootParams params;
  params.machine = "yuxing";
  const auto booted = rig.manager->boot(params);
  EXPECT_EQ(booted.machine.id, "yuxing");
  ASSERT_TRUE(waitFor([&] { return rig.frames->count() >= 5; }));
  EXPECT_EQ(rig.runtime->status().machine, "yuxing");
}

TEST(MachineManager, BootRunsTheMachineWithoutACartridge) {
  Rig rig;
  rig.manager->setRom(rig.setParams("sb2000", rig.write("good.bin", romFile())));
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
  rig.manager->setRom(rig.setParams("sb2000", rig.write("good.bin", romFile())));
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

TEST(KnownRoms, IdentifiesRomsByContentAndLabelsThemWithVersionOrModel) {
  using ember::logic::machines::findKnownRom;
  using ember::logic::machines::romLabel;
  const auto yuxing = findKnownRom(0x6085FEE8U);
  ASSERT_TRUE(yuxing.has_value());
  EXPECT_EQ(yuxing->machine, "yuxing");
  EXPECT_EQ(yuxing->version, "V9.0-98");
  EXPECT_EQ(romLabel(0x6085FEE8U, "yx-v9.0-98.bin"), "V9.0-98");
  EXPECT_EQ(findKnownRom(0x05F958C4U)->machine, "pec586");
  EXPECT_EQ(romLabel(0x05F958C4U, "x.bin"), "PEC-686F");
  EXPECT_EQ(findKnownRom(0xE07BD2C3U)->machine, "sb2000");
  EXPECT_EQ(romLabel(0xE07BD2C3U, "sb2000_bios.bin"), "SB2000") << "只有一种 ROM 的机型显示型号名";
  EXPECT_EQ(romLabel(0x8C8FE800U, "bbg98_bios.bin"), "98型");
  EXPECT_EQ(romLabel(0xB3D11A3CU, "bbg1_bios.bin"), "软驱一号");
  EXPECT_FALSE(findKnownRom(0x12345678U).has_value());
  EXPECT_EQ(romLabel(0x12345678U, "other.bin"), "other.bin");
}

TEST(MachineManager, AddingFromADirectoryIdentifiesRomsByContent) {
  const auto sb2000 = realRom("sb2000_bios.bin");
  const auto yuxing90 = realRom("yuxing_v9.0-98.bin");
  const auto yuxing80 = realRom("yuxing_v8.0-c.bin");
  if (sb2000.empty() || yuxing90.empty() || yuxing80.empty()) GTEST_SKIP() << "没有找到 ember/roms 里的真实 ROM";
  Rig rig;
  rig.put("pack/random-name.bin", sb2000);
  rig.put("pack/yx-a.bin", yuxing80);
  rig.put("pack/yx-b.BIN", yuxing90);
  rig.put("pack/junk.bin", romFile());
  rig.put("pack/notes.txt", {1, 2, 3});
  rig.put("pack/game.nes", sb2000);
  const auto result = rig.manager->setRoms(rig.setRomsParams());
  ASSERT_EQ(result.added.size(), 2U);
  EXPECT_EQ(result.added[0].id, "sb2000");
  EXPECT_EQ(result.added[1].id, "yuxing");
  EXPECT_EQ(result.added[1].rom->fileName, "yx-a.bin") << "同一机型的多个文件取文件名排在最前的";
  EXPECT_EQ(result.added[1].rom->label, "V8.0-C");
  EXPECT_EQ(skippedReason(result, "yx-b.BIN"), "duplicate");
  EXPECT_EQ(skippedReason(result, "junk.bin"), "unrecognized");
  EXPECT_EQ(result.skipped.size(), 2U) << "txt 与 nes 不在候选里";
  EXPECT_EQ(rig.find("sb2000").rom->fileName, "random-name.bin");
  EXPECT_EQ(rig.events->count("machines.changed"), 2);
}

TEST(MachineManager, AddingFromADirectoryKeepsMachinesThatAlreadyHaveARom) {
  const auto yuxing90 = realRom("yuxing_v9.0-98.bin");
  if (yuxing90.empty()) GTEST_SKIP() << "没有找到 ember/roms/yuxing_v9.0-98.bin";
  Rig rig;
  rig.manager->setRom(rig.setParams("yuxing", rig.write("mine.bin", romFile(64))));
  rig.put("pack/yx.bin", yuxing90);
  const auto result = rig.manager->setRoms(rig.setRomsParams());
  EXPECT_TRUE(result.added.empty());
  EXPECT_EQ(skippedReason(result, "yx.bin"), "exists");
  EXPECT_EQ(rig.find("yuxing").rom->fileName, "mine.bin");
  EXPECT_EQ(rig.events->count("machines.changed"), 1);
}

TEST(MachineManager, ABootedRomCarriesItsVersionLabel) {
  const auto yuxing90 = realRom("yuxing_v9.0-98.bin");
  if (yuxing90.empty()) GTEST_SKIP() << "没有找到 ember/roms/yuxing_v9.0-98.bin";
  Rig rig;
  rig.manager->setRom(rig.setParams("yuxing", rig.write("anything.bin", yuxing90)));
  EXPECT_EQ(rig.find("yuxing").rom->label, "V9.0-98");
  api::MachinesBootParams params;
  params.machine = "yuxing";
  rig.manager->boot(params);
  ASSERT_TRUE(waitFor([&] { return rig.frames->count() >= 5; }));
  EXPECT_EQ(rig.runtime->status().romLabel, "V9.0-98");
}

TEST(MachineManager, AddingFromADirectoryReportsAMissingOrNonDirectoryPath) {
  Rig rig;
  api::MachinesSetRomsParams params;
  params.path = (rig.dir.path() / "nowhere").string();
  EXPECT_EQ(errorCodeOf([&] { rig.manager->setRoms(params); }), api::ERROR_FILE_NOT_FOUND);
  params.path = rig.write("file.bin", romFile());
  EXPECT_EQ(errorCodeOf([&] { rig.manager->setRoms(params); }), api::ERROR_FILE_UNREADABLE);
  EXPECT_EQ(rig.events->count("machines.changed"), 0);
}

}  // namespace
