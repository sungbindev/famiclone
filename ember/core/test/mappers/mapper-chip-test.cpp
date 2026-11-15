/*
 * ------------------------------------------------------------------------
 * 名称：mapper-chip-test.cpp
 * 说明：由插件创建的卡带的测试：窗口译码与镜像、开放总线、ROM 拒绝写、CHR RAM 后备、寄存器回调、脚本改窗口与名称表、状态位宽、中断引脚、
 *       电池 RAM、复位、脚本出错后停止回调、清单声明了回调但脚本没有导出的错误、脚本日志。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：数据按位置取值（PRG 第 i 字节为 i×7+i/256，CHR 第 i 字节为 0x80+i×3），便于核对每个窗口指到了哪里。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "../support/plugin-test-support.hpp"
#include "contracts/mappers/mapper-error.hpp"
#include "infra/mappers/emap-package.hpp"
#include "infra/mappers/mapper-chip.hpp"
#include "shared/errors/core-error.hpp"

namespace {

using nlohmann::json;
using namespace ember::infra::mappers;
namespace hw = ember::contracts::hardware;
namespace mapper_error = ember::contracts::mappers;
using ember::test::RecordingIrq;
using ember::test::RecordingPads;

std::uint8_t prgByte(std::size_t index) { return static_cast<std::uint8_t>(index * 7 + index / 256); }
std::uint8_t chrByte(std::size_t index) { return static_cast<std::uint8_t>(0x80 + index * 3); }

/** 按字节数构造卡带映像。 */
mapper_error::CartridgeImage makeImage(std::size_t prgBytes, std::size_t chrBytes, mapper_error::Mirroring mirroring = mapper_error::Mirroring::Horizontal) {
  mapper_error::CartridgeImage image;
  for (std::size_t i = 0; i < prgBytes; ++i) image.prg.push_back(prgByte(i));
  for (std::size_t i = 0; i < chrBytes; ++i) image.chr.push_back(chrByte(i));
  image.description.mapper = 7;
  image.description.prgSize = prgBytes;
  image.description.chrSize = chrBytes;
  image.description.mirroring = mirroring;
  return image;
}

/** 由清单与脚本构造插件。 */
std::shared_ptr<const LoadedPlugin> makePlugin(const json& manifest, const std::string& script) {
  auto plugin = std::make_shared<LoadedPlugin>();
  plugin->manifest = parseManifest(manifest.dump());
  plugin->script = script;
  return plugin;
}

/** 卡带加记录型的 IRQ 引脚。 */
struct Rig {
  Rig(const json& manifest, const std::string& script, const mapper_error::CartridgeImage& image, ScriptLimits limits = {}) : chip(makePlugin(manifest, script), image, irq, pads, limits) {}

  hw::BusRead cpu(std::uint16_t address) { return chip.cpuSide().read(address); }
  hw::BusRead ppu(std::uint16_t address) { return chip.ppuSide().read(address); }
  void cpuWrite(std::uint16_t address, std::uint8_t value) { chip.cpuSide().write(address, value); }
  void ppuWrite(std::uint16_t address, std::uint8_t value) { chip.ppuSide().write(address, value); }

  RecordingIrq irq;
  RecordingPads pads;
  MapperChip chip;
};

/** 在最小清单上加寄存器范围与状态。 */
json withRegisters(const std::string& registers, const std::string& state = "[]") {
  json manifest = json::parse(ember::test::minimalManifest());
  manifest["registers"] = json::parse(registers);
  manifest["state"] = json::parse(state);
  return manifest;
}

/** 脚本的 write 里按第一个参数 value 分支的写法：每个测试给出自己的函数体。 */
std::string writeScript(const std::string& body) { return "export default { write(ctx, bus, addr, value) {\n" + body + "\n} };"; }

constexpr const char* WRITE_ALL = R"([{ "bus": "cpu", "range": "$8000-$FFFF", "write": true }])";

TEST(MapperChip, TheWindowsMapPrgAndAMirroredSixteenKilobytePrgAppearsTwice) {
  Rig rig(json::parse(ember::test::minimalManifest()), "", makeImage(16384, 8192));
  EXPECT_EQ(rig.cpu(0x8000).value, prgByte(0));
  EXPECT_EQ(rig.cpu(0xBFFF).value, prgByte(16383));
  EXPECT_EQ(rig.cpu(0xC000).value, prgByte(0));
  EXPECT_EQ(rig.cpu(0xC123).value, prgByte(0x123));
  EXPECT_EQ(rig.cpu(0xFFFF).value, prgByte(16383));
  EXPECT_EQ(rig.cpu(0x8000).drivenMask, 0xFF);
}

TEST(MapperChip, AThirtyTwoKilobytePrgUsesTheFirstAndLastBanks) {
  Rig rig(json::parse(ember::test::minimalManifest()), "", makeImage(32768, 8192));
  EXPECT_EQ(rig.cpu(0x8000).value, prgByte(0));
  EXPECT_EQ(rig.cpu(0xBFFF).value, prgByte(16383));
  EXPECT_EQ(rig.cpu(0xC000).value, prgByte(16384));
  EXPECT_EQ(rig.cpu(0xFFFF).value, prgByte(32767));
}

TEST(MapperChip, AddressesNoWindowCoversAreOpenBusAndWritesToThemAreDropped) {
  Rig rig(json::parse(ember::test::minimalManifest()), "", makeImage(16384, 8192));
  for (const int address : {0x4020, 0x5FFF, 0x6000, 0x7FFF}) {
    EXPECT_EQ(rig.cpu(static_cast<std::uint16_t>(address)).drivenMask, 0) << address;
    EXPECT_NO_THROW(rig.cpuWrite(static_cast<std::uint16_t>(address), 0x55));
  }
}

TEST(MapperChip, WritesToARomWindowAreIgnored) {
  Rig rig(json::parse(ember::test::minimalManifest()), "", makeImage(16384, 8192));
  rig.cpuWrite(0x8000, 0xFF);
  rig.cpuWrite(0xC000, 0xFF);
  EXPECT_EQ(rig.cpu(0x8000).value, prgByte(0));
}

TEST(MapperChip, ChrIsRomWhenTheFileHasItAndRamWhenItDoesNot) {
  Rig withChr(json::parse(ember::test::minimalManifest()), "", makeImage(16384, 8192));
  EXPECT_EQ(withChr.ppu(0x0100).value, chrByte(0x100));
  withChr.ppuWrite(0x0100, 0x11);
  EXPECT_EQ(withChr.ppu(0x0100).value, chrByte(0x100));
  EXPECT_EQ(withChr.chip.storage("chr")->kind(), hw::StorageKind::Rom);

  Rig withoutChr(json::parse(ember::test::minimalManifest()), "", makeImage(16384, 0));
  EXPECT_EQ(withoutChr.chip.storage("chr")->kind(), hw::StorageKind::Ram);
  EXPECT_EQ(withoutChr.chip.storage("chr")->size(), 8192U);
  withoutChr.ppuWrite(0x0100, 0x11);
  withoutChr.ppuWrite(0x1FFF, 0x22);
  EXPECT_EQ(withoutChr.ppu(0x0100).value, 0x11);
  EXPECT_EQ(withoutChr.ppu(0x1FFF).value, 0x22);
}

TEST(MapperChip, TheStorageAndStateAccessorsFindDeclaredItemsOnly) {
  Rig rig(withRegisters(WRITE_ALL, R"([{ "name": "bank", "bits": 8, "description": "x" }])"), writeScript(""), makeImage(16384, 8192));
  ASSERT_NE(rig.chip.storage("prg"), nullptr);
  EXPECT_EQ(rig.chip.storage("prg")->size(), 16384U);
  EXPECT_EQ(rig.chip.storage("prg")->kind(), hw::StorageKind::Rom);
  EXPECT_EQ(rig.chip.storage("nothing"), nullptr);
  EXPECT_EQ(rig.chip.state("bank"), 0U);
  EXPECT_EQ(rig.chip.state("nothing"), std::nullopt);
  EXPECT_EQ(rig.chip.pluginId(), "test-plugin");
}

TEST(MapperChip, DeclaredRegisterWritesSwitchAChrBankAndStoreTheValueWithoutAScript) {
  const auto cnrom = ember::infra::mappers::readEmapPackage(ember::test::testPluginPackage("cnrom"));
  EXPECT_FALSE(cnrom.hasScript);
  auto plugin = std::make_shared<LoadedPlugin>();
  plugin->manifest = parseManifest(cnrom.manifestText);
  RecordingIrq irq;
  RecordingPads pads;
  MapperChip chip(plugin, makeImage(32768, 32768), irq, pads);
  chip.reset(true);
  EXPECT_EQ(chip.ppuSide().read(0x0000).value, chrByte(0));
  chip.cpuSide().write(0x8000, 2);
  EXPECT_EQ(chip.ppuSide().read(0x0000).value, chrByte(2 * 8192));
  EXPECT_EQ(chip.ppuSide().read(0x1FFF).value, chrByte(2 * 8192 + 0x1FFF));
  EXPECT_EQ(chip.state("chrBank"), 2U);
  // 超出 bank 数时按 bank 数取模（真机的地址线只连低位）。
  chip.cpuSide().write(0xFFFF, 6);
  EXPECT_EQ(chip.ppuSide().read(0x0000).value, chrByte(2 * 8192));
  // 只读的 PRG 窗口读到的仍是 ROM；软复位保持寄存器，上电复位后回到第 0 页。
  EXPECT_EQ(chip.cpuSide().read(0x8000).value, prgByte(0));
  chip.reset(false);
  EXPECT_EQ(chip.ppuSide().read(0x0000).value, chrByte(2 * 8192));
  chip.reset(true);
  EXPECT_EQ(chip.ppuSide().read(0x0000).value, chrByte(0));
  EXPECT_EQ(chip.state("chrBank"), 0U);
  EXPECT_FALSE(chip.failure().has_value());
}

TEST(MapperChip, RegisterReadsCallTheScriptAndUndefinedMeansNoDriver) {
  json manifest = json::parse(ember::test::minimalManifest());
  manifest["registers"] = json::parse(R"([{ "bus": "cpu", "range": "$5000-$5001", "read": true }])");
  Rig rig(manifest, "export default { read(ctx, bus, addr) { return addr === 0x5000 ? 0x42 : undefined; } };", makeImage(16384, 8192));
  const hw::BusRead driven = rig.cpu(0x5000);
  EXPECT_EQ(driven.value, 0x42);
  EXPECT_EQ(driven.drivenMask, 0xFF);
  EXPECT_EQ(rig.cpu(0x5001).drivenMask, 0);
  EXPECT_EQ(rig.cpu(0x5002).drivenMask, 0);  // 没声明的地址不调用脚本
}

TEST(MapperChip, RegisterRangesWinOverWindowsOnTheSameAddresses) {
  // 寄存器覆盖整个 $8000-$FFFF 的读与写：读由脚本回答，窗口不再响应。
  json manifest = withRegisters(R"([{ "bus": "cpu", "range": "$8000-$FFFF", "read": true, "write": true }])");
  Rig rig(manifest, "export default { read() { return 0x77; }, write() {} };", makeImage(16384, 8192));
  EXPECT_EQ(rig.cpu(0x8000).value, 0x77);
  EXPECT_EQ(rig.cpu(0xFFFF).value, 0x77);
}

TEST(MapperChip, PpuRegistersCallTheScriptToo) {
  json manifest = json::parse(ember::test::minimalManifest());
  manifest["registers"] = json::parse(R"([{ "bus": "ppu", "range": "$1000-$1FFF", "read": true, "write": true }])");
  manifest["state"] = json::parse(R"([{ "name": "latch", "bits": 8, "description": "x" }])");
  Rig rig(manifest, "export default { read(ctx, bus, addr) { return bus === 'ppu' ? ctx.state.latch : 0; }, write(ctx, bus, addr, value) { ctx.state.latch = value; } };", makeImage(16384, 8192));
  EXPECT_EQ(rig.ppu(0x0000).value, chrByte(0));  // $0000-$0FFF 仍由窗口响应
  rig.ppuWrite(0x1800, 9);
  EXPECT_EQ(rig.ppu(0x1000).value, 9);
}

TEST(MapperChip, StateWritesAreTruncatedToTheDeclaredWidth) {
  Rig rig(withRegisters(WRITE_ALL, R"([{ "name": "nibble", "bits": 4, "description": "x" }, { "name": "wide", "bits": 32, "description": "y" }])"),
          writeScript("ctx.state.nibble = value; ctx.state.wide = 0xFFFFFFFF;"), makeImage(16384, 8192));
  rig.cpuWrite(0x8000, 0x1F);
  EXPECT_EQ(rig.chip.state("nibble"), 0xFU);
  EXPECT_EQ(rig.chip.state("wide"), 0xFFFFFFFFU);
}

TEST(MapperChip, TheScriptMovesWindowsByBankByOffsetAndCanUnmapThem) {
  json manifest = withRegisters(R"([{ "bus": "cpu", "range": "$5000-$5000", "write": true }])");
  manifest["windows"] = json::parse(R"([{ "id": "w", "bus": "cpu", "range": "$8000-$9FFF", "storage": "prg", "size": 8192, "initial": 1, "access": "r" }])");
  Rig rig(manifest, writeScript("const w = ctx.window('w'); if (value === 200) w.unmap(); else if (value >= 100) w.setOffset((value - 100) * 1000); else w.setBank(value);"),
          makeImage(32768, 0));
  EXPECT_EQ(rig.cpu(0x8000).value, prgByte(8192));  // initial 为整数 1
  rig.cpuWrite(0x5000, 3);
  EXPECT_EQ(rig.cpu(0x8000).value, prgByte(3 * 8192));
  rig.cpuWrite(0x5000, 6);  // 4 个 bank，6 取模为 2
  EXPECT_EQ(rig.cpu(0x8000).value, prgByte(2 * 8192));
  rig.cpuWrite(0x5000, 101);  // 按字节偏移 1000
  EXPECT_EQ(rig.cpu(0x8000).value, prgByte(1000));
  EXPECT_EQ(rig.cpu(0x9FFF).value, prgByte(1000 + 8191));
  rig.cpuWrite(0x5000, 100 + 32);  // 偏移 32000：窗口末尾越过存储区时回绕
  EXPECT_EQ(rig.cpu(0x8000 + 768).value, prgByte(0));
  rig.cpuWrite(0x5000, 200);
  EXPECT_EQ(rig.cpu(0x8000).drivenMask, 0);
  rig.cpuWrite(0x5000, 1);  // 再次设置 bank 就重新映射
  EXPECT_EQ(rig.cpu(0x8000).value, prgByte(8192));
}

TEST(MapperChip, WindowsStartOnTheLastBankOrTheFirstAsDeclaredAndABankCountRoundsDown) {
  json manifest = json::parse(ember::test::minimalManifest());
  manifest["windows"][0]["size"] = 16384;
  Rig rig(manifest, "", makeImage(24576, 0));  // 24K：16K 的窗口只有 1 个完整的 bank，last 也是 bank 0
  EXPECT_EQ(rig.cpu(0x8000).value, prgByte(0));
  EXPECT_EQ(rig.cpu(0xC000).value, prgByte(0));
}

TEST(MapperChip, NametableFollowsTheHeaderTheManifestAndTheScript) {
  json manifest = withRegisters(WRITE_ALL);
  const std::string script = writeScript("ctx.nametable.set(value === 0 ? 'horizontal' : value === 1 ? 'vertical' : value === 2 ? 'single0' : value === 3 ? 'single1' : 'four');");
  Rig vertical(manifest, script, makeImage(16384, 8192, mapper_error::Mirroring::Vertical));
  // 垂直：$2000 与 $2800 同页，$2400 与 $2C00 同页。
  EXPECT_FALSE(vertical.chip.ciramControl(0x2000).a10);
  EXPECT_TRUE(vertical.chip.ciramControl(0x2400).a10);
  EXPECT_FALSE(vertical.chip.ciramControl(0x2800).a10);
  EXPECT_TRUE(vertical.chip.ciramControl(0x2C00).a10);
  EXPECT_TRUE(vertical.chip.ciramControl(0x2000).enable);

  vertical.cpuWrite(0x8000, 0);  // 水平
  EXPECT_FALSE(vertical.chip.ciramControl(0x2000).a10);
  EXPECT_FALSE(vertical.chip.ciramControl(0x2400).a10);
  EXPECT_TRUE(vertical.chip.ciramControl(0x2800).a10);
  vertical.cpuWrite(0x8000, 2);
  EXPECT_FALSE(vertical.chip.ciramControl(0x2C00).a10);
  vertical.cpuWrite(0x8000, 3);
  EXPECT_TRUE(vertical.chip.ciramControl(0x2000).a10);
  vertical.chip.reset(true);  // 上电复位回到文件头的连法
  EXPECT_TRUE(vertical.chip.ciramControl(0x2400).a10);
  EXPECT_FALSE(vertical.chip.ciramControl(0x2800).a10);

  manifest["nametable"] = "horizontal";  // 清单覆盖文件头
  Rig overridden(manifest, script, makeImage(16384, 8192, mapper_error::Mirroring::Vertical));
  EXPECT_FALSE(overridden.chip.ciramControl(0x2400).a10);
  EXPECT_TRUE(overridden.chip.ciramControl(0x2800).a10);
}

TEST(MapperChip, FourScreenGivesTheCartridgeItsOwnNametableMemory) {
  Rig rig(json::parse(ember::test::minimalManifest()), "", makeImage(16384, 8192, mapper_error::Mirroring::FourScreen));
  EXPECT_FALSE(rig.chip.ciramControl(0x2000).enable);
  rig.ppuWrite(0x2000, 0x11);
  rig.ppuWrite(0x2C00, 0x22);
  rig.ppuWrite(0x2FFF, 0x33);
  EXPECT_EQ(rig.ppu(0x2000).value, 0x11);
  EXPECT_EQ(rig.ppu(0x2400).value, 0);  // 四页互相独立
  EXPECT_EQ(rig.ppu(0x2C00).value, 0x22);
  EXPECT_EQ(rig.ppu(0x2FFF).value, 0x33);
  EXPECT_EQ(rig.ppu(0x3000).value, 0x11);  // $3000-$3EFF 是 $2000-$2EFF 的镜像
}

TEST(MapperChip, TheIrqPinIsDrivenByTheScriptAndReleasedOnPowerReset) {
  Rig rig(withRegisters(WRITE_ALL), writeScript("if (value === 1) ctx.irq.assert(); else ctx.irq.release();"), makeImage(16384, 8192));
  EXPECT_FALSE(rig.irq.active());
  rig.cpuWrite(0x8000, 1);
  EXPECT_TRUE(rig.irq.active());
  rig.cpuWrite(0x8000, 0);
  EXPECT_FALSE(rig.irq.active());
  rig.cpuWrite(0x8000, 1);
  rig.chip.reset(true);
  EXPECT_FALSE(rig.irq.active());
  EXPECT_EQ(rig.irq.assertions(), 2);
}

TEST(MapperChip, RamWindowsAreWritableAndOnlyBatteryBackedRamSurvivesAPowerCycle) {
  json manifest = json::parse(ember::test::minimalManifest());
  manifest["storage"].push_back(json::parse(R"({ "id": "work", "kind": "ram", "size": 8192 })"));
  manifest["storage"].push_back(json::parse(R"({ "id": "save", "kind": "ram", "size": 4096, "battery": true })"));
  manifest["windows"].push_back(json::parse(R"({ "id": "work", "bus": "cpu", "range": "$6000-$7FFF", "storage": "work", "size": 8192, "access": "rw" })"));
  manifest["windows"].push_back(json::parse(R"({ "id": "save", "bus": "cpu", "range": "$5000-$5FFF", "storage": "save", "size": 4096, "access": "rw" })"));
  Rig rig(manifest, "", makeImage(16384, 8192));
  rig.cpuWrite(0x6000, 0xAA);
  rig.cpuWrite(0x7FFF, 0xBB);
  rig.cpuWrite(0x5000, 0xCC);
  EXPECT_EQ(rig.cpu(0x6000).value, 0xAA);
  EXPECT_EQ(rig.cpu(0x7FFF).value, 0xBB);
  rig.chip.reset(false);  // 软复位：RAM 保持
  EXPECT_EQ(rig.cpu(0x6000).value, 0xAA);
  rig.chip.reset(true);  // 上电：不带电池的清零，带电池的保持
  EXPECT_EQ(rig.cpu(0x6000).value, 0);
  EXPECT_EQ(rig.cpu(0x7FFF).value, 0);
  EXPECT_EQ(rig.cpu(0x5000).value, 0xCC);
}

TEST(MapperChip, AScriptErrorStopsTheScriptAndIsReportedUntilTheNextReset) {
  json manifest = withRegisters(R"([{ "bus": "cpu", "range": "$8000-$FFFF", "read": true, "write": true }])", R"([{ "name": "count", "bits": 8, "description": "x" }])");
  Rig rig(manifest, "export default { read() { return 1; }, write(ctx, bus, addr, value) { if (value === 1) throw new Error('bad write'); ctx.state.count = ctx.state.count + 1; } };",
          makeImage(16384, 8192));
  rig.cpuWrite(0x8000, 0);
  EXPECT_EQ(rig.chip.state("count"), 1U);
  EXPECT_EQ(rig.cpu(0x8000).value, 1);
  EXPECT_FALSE(rig.chip.failure().has_value());

  rig.cpuWrite(0x8000, 1);
  ASSERT_TRUE(rig.chip.failure().has_value());
  EXPECT_EQ(rig.chip.failure()->code, mapper_error::ERROR_SCRIPT_ERROR);
  EXPECT_EQ(rig.chip.failure()->pluginId, "test-plugin");
  EXPECT_NE(rig.chip.failure()->message.find("bad write"), std::string::npos);
  EXPECT_NE(rig.chip.failure()->message.find("test-plugin"), std::string::npos);

  // 出错之后不再调用脚本：写入被丢弃，读不驱动总线。
  rig.cpuWrite(0x8000, 0);
  EXPECT_EQ(rig.chip.state("count"), 1U);
  EXPECT_EQ(rig.cpu(0x8000).drivenMask, 0);

  rig.chip.reset(false);
  EXPECT_FALSE(rig.chip.failure().has_value());
  rig.cpuWrite(0x8000, 0);
  EXPECT_EQ(rig.chip.state("count"), 2U);
}

TEST(MapperChip, ExceedingALimitIsReportedAsLimitExceeded) {
  ScriptLimits limits;
  limits.callTimeout = std::chrono::milliseconds(30);
  Rig rig(withRegisters(WRITE_ALL), writeScript("for (;;) {}"), makeImage(16384, 8192), limits);
  rig.cpuWrite(0x8000, 0);
  ASSERT_TRUE(rig.chip.failure().has_value());
  EXPECT_EQ(rig.chip.failure()->code, mapper_error::ERROR_LIMIT_EXCEEDED);
}

TEST(MapperChip, ResetCallsTheScriptWithPowerOrSoft) {
  json manifest = withRegisters(WRITE_ALL, R"([{ "name": "last", "bits": 8, "description": "x" }])");
  Rig rig(manifest, "export default { reset(ctx, kind) { ctx.state.last = kind === 'power' ? 1 : 2; }, write() {} };", makeImage(16384, 8192));
  rig.chip.reset(true);
  EXPECT_EQ(rig.chip.state("last"), 1U);
  rig.chip.reset(false);
  EXPECT_EQ(rig.chip.state("last"), 2U);
  rig.chip.reset(true);
  EXPECT_EQ(rig.chip.state("last"), 1U);
}

TEST(MapperChip, TheScriptCanReadTheStorageAndWriteTheLog) {
  Rig rig(withRegisters(WRITE_ALL), writeScript("ctx.log.warn('prg1=' + ctx.storage('prg').read(1) + ' size=' + ctx.storage('prg').size + ' rom=' + ctx.storage('prg').write(0, 1));"),
          makeImage(16384, 8192));
  rig.cpuWrite(0x8000, 0);
  ASSERT_EQ(rig.chip.scriptLog().size(), 1U);
  EXPECT_EQ(rig.chip.scriptLog()[0], "prg1=" + std::to_string(prgByte(1)) + " size=16384 rom=false");
  EXPECT_EQ(rig.cpu(0x8000).value, prgByte(0));
}

TEST(MapperChip, ScriptLogEntriesAreCappedInSizeAndCount) {
  Rig rig(withRegisters(WRITE_ALL), writeScript("if (value === 255) ctx.log.warn('a'.repeat(2500)); else if (value === 254) ctx.log.warn('你'.repeat(1000)); else ctx.log.warn('n' + value);"), makeImage(16384, 8192));
  rig.cpuWrite(0x8000, 255);
  rig.cpuWrite(0x8000, 254);
  ASSERT_EQ(rig.chip.scriptLog().size(), 2U);
  EXPECT_EQ(rig.chip.scriptLog()[0].size(), 2000U);
  EXPECT_EQ(rig.chip.scriptLog()[1].size(), 1998U);  // 退到 UTF-8 字符的边界，不留半个字符
  for (int i = 0; i < 250; ++i) rig.cpuWrite(0x8000, static_cast<std::uint8_t>(i % 200));
  const auto log = rig.chip.scriptLog();
  EXPECT_EQ(log.size(), 200U);
  EXPECT_EQ(log.back(), "n" + std::to_string(249 % 200));
}

TEST(MapperChipErrors, RegistersWithoutTheMatchingScriptFunctionAreRejected) {
  json manifest = json::parse(ember::test::minimalManifest());
  manifest["registers"] = json::parse(R"([{ "bus": "cpu", "range": "$5000-$5001", "read": true }])");
  try {
    Rig rig(manifest, "export default { write() {} };", makeImage(16384, 0));
    FAIL() << "应当抛出异常";
  } catch (const ember::shared::CoreError& error) {
    EXPECT_EQ(error.code(), mapper_error::ERROR_INVALID_PACKAGE);
    EXPECT_NE(std::string(error.what()).find("没有导出 read"), std::string::npos) << error.what();
  }
  manifest["registers"] = json::parse(R"([{ "bus": "cpu", "range": "$5000-$5001", "write": true }])");
  EXPECT_THROW(Rig(manifest, "export default { read() {} };", makeImage(16384, 0)), ember::shared::CoreError);
  try {
    Rig rig(manifest, "", makeImage(16384, 0));
    FAIL() << "应当抛出异常";
  } catch (const ember::shared::CoreError& error) {
    EXPECT_EQ(error.code(), mapper_error::ERROR_INVALID_PACKAGE);
    EXPECT_NE(std::string(error.what()).find("没有 main.js"), std::string::npos) << error.what();
  }
}

/** 带磁带数据线的卡带：磁带线由测试控制，主机的手柄口把它交给卡带。 */
struct TapeRig {
  TapeRig(const json& manifest, const std::string& script, const mapper_error::CartridgeImage& image) : pads(&tape), chip(makePlugin(manifest, script), image, irq, pads) {}

  ember::test::FakeTapeLine tape;
  RecordingIrq irq;
  RecordingPads pads;
  MapperChip chip;
};

TEST(MapperChipDeclared, ReadsAnswerWithAFixedValueAStateOrNothingWithoutAScript) {
  json manifest = withRegisters(R"([
    { "bus": "cpu", "range": "$5000-$5000", "read": { "value": 66 } },
    { "bus": "cpu", "range": "$5001-$5001", "read": { "state": "latch" } },
    { "bus": "cpu", "range": "$5002-$5002", "read": { "open": true } },
    { "bus": "cpu", "range": "$5003-$5003", "write": { "state": "latch" } }
  ])",
                                R"([{ "name": "latch", "bits": 8, "description": "x" }])");
  Rig rig(manifest, "", makeImage(16384, 8192));
  const hw::BusRead fixed = rig.cpu(0x5000);
  EXPECT_EQ(fixed.value, 66);
  EXPECT_EQ(fixed.drivenMask, 0xFF);
  rig.cpuWrite(0x5003, 0x5A);
  EXPECT_EQ(rig.cpu(0x5001).value, 0x5A);
  EXPECT_EQ(rig.cpu(0x5002).drivenMask, 0);
  EXPECT_EQ(rig.cpu(0x5004).drivenMask, 0);
  // 调试读取不触发回调，声明的固定值与状态可以直接给出。
  EXPECT_EQ(rig.chip.cpuSide().peek(0x5000).value, 66);
  EXPECT_EQ(rig.chip.cpuSide().peek(0x5001).value, 0x5A);
  EXPECT_EQ(rig.chip.cpuSide().peek(0x5002).drivenMask, 0);
  EXPECT_FALSE(rig.chip.failure().has_value());
}

TEST(MapperChipDeclared, WritesSwitchWindowsByShiftedAndMaskedDataAndStoreTheState) {
  json manifest = withRegisters(R"([{ "bus": "cpu", "range": "$5000-$5000", "write": { "state": "last", "bank": [{ "window": "w", "mask": 3 }, { "window": "v", "shift": 2, "mask": 3 }] } }])",
                                R"([{ "name": "last", "bits": 8, "description": "x" }])");
  manifest["windows"] = json::parse(R"([
    { "id": "w", "bus": "cpu", "range": "$8000-$9FFF", "storage": "prg", "size": 8192, "initial": 0, "access": "r" },
    { "id": "v", "bus": "cpu", "range": "$A000-$BFFF", "storage": "prg", "size": 8192, "initial": 0, "access": "r" }
  ])");
  Rig rig(manifest, "", makeImage(32768, 0));
  rig.cpuWrite(0x5000, 0b1110);  // w 取低 2 位 = 2，v 取右移 2 位后的低 2 位 = 3
  EXPECT_EQ(rig.cpu(0x8000).value, prgByte(2 * 8192));
  EXPECT_EQ(rig.cpu(0xA000).value, prgByte(3 * 8192));
  EXPECT_EQ(rig.chip.state("last"), 0b1110U);
}

TEST(MapperChipDeclared, TablesAreComputedByTheScriptOnceThenServedFromTheCache) {
  json manifest = withRegisters(R"([
    { "bus": "cpu", "range": "$5000-$5001", "write": { "update": { "table": "sum", "into": "acc" } } },
    { "bus": "cpu", "range": "$5002-$5002", "write": { "state": "acc" } }
  ])",
                                R"([{ "name": "acc", "bits": 8, "description": "x" }])");
  manifest["tables"] = json::parse(R"([{ "id": "sum", "index": [{ "from": "state", "name": "acc" }, { "from": "data", "shift": 0, "bits": 4 }, { "from": "address", "shift": 0, "bits": 1 }], "bits": 8 }])");
  Rig rig(manifest, "export default { tables: { sum(ctx, acc, data, odd) { ctx.log.warn('call'); return acc + data * 2 + odd; } } };", makeImage(16384, 8192));
  rig.cpuWrite(0x5000, 3);
  EXPECT_EQ(rig.chip.state("acc"), 6U);
  EXPECT_EQ(rig.chip.scriptLog().size(), 1U);
  rig.cpuWrite(0x5002, 0);
  rig.cpuWrite(0x5000, 3);  // 同一个索引：不再调用脚本
  EXPECT_EQ(rig.chip.state("acc"), 6U);
  EXPECT_EQ(rig.chip.scriptLog().size(), 1U);
  rig.cpuWrite(0x5002, 0);
  rig.cpuWrite(0x5001, 3);  // 地址位 0 不同，是另一个索引
  EXPECT_EQ(rig.chip.state("acc"), 7U);
  EXPECT_EQ(rig.chip.scriptLog().size(), 2U);
  EXPECT_FALSE(rig.chip.failure().has_value());
}

TEST(MapperChipDeclared, ATableIndexCanUseTheTapeLevelAndTheResultIsTruncatedToTheTableWidth) {
  json manifest = withRegisters(R"([{ "bus": "cpu", "range": "$5000-$5000", "write": { "update": { "table": "pick", "into": "acc" } } }])", R"([{ "name": "acc", "bits": 8, "description": "x" }])");
  manifest["tables"] = json::parse(R"([{ "id": "pick", "index": [{ "from": "tape" }], "bits": 4 }])");
  TapeRig rig(manifest, "export default { tables: { pick(ctx, tape) { return tape ? 0x1F : 0x22; } } };", makeImage(16384, 8192));
  rig.chip.cpuSide().write(0x5000, 0);
  EXPECT_EQ(rig.chip.state("acc"), 0x2U);  // 0x22 按 4 位截断
  rig.tape.setLevel(true);
  rig.chip.cpuSide().write(0x5000, 0);
  EXPECT_EQ(rig.chip.state("acc"), 0xFU);
}

TEST(MapperChipDeclared, StatesWithAnInitialValueReturnToItOnEveryResetAndTheOthersOnlyOnPowerUp) {
  json manifest = withRegisters(R"([
    { "bus": "cpu", "range": "$5000-$5000", "write": { "state": "a" } },
    { "bus": "cpu", "range": "$5001-$5001", "write": { "state": "b" } }
  ])",
                                R"([{ "name": "a", "bits": 8, "initial": 255, "description": "x" }, { "name": "b", "bits": 8, "description": "y" }])");
  Rig rig(manifest, "", makeImage(16384, 8192));
  rig.chip.reset(true);
  EXPECT_EQ(rig.chip.state("a"), 255U);
  rig.cpuWrite(0x5000, 1);
  rig.cpuWrite(0x5001, 2);
  rig.chip.reset(false);
  EXPECT_EQ(rig.chip.state("a"), 255U);
  EXPECT_EQ(rig.chip.state("b"), 2U);
  rig.chip.reset(true);
  EXPECT_EQ(rig.chip.state("a"), 255U);
  EXPECT_EQ(rig.chip.state("b"), 0U);
}

TEST(MapperChipDeclared, TheHostExecutesTheTapeComparatorAndTheEdgeTimerAndRaisesTheIrq) {
  json manifest = withRegisters(R"([{ "bus": "cpu", "range": "$5000-$5000", "read": { "state": "ticks", "releaseIrq": true } }])",
                                R"([{ "name": "ticks", "bits": 8, "initial": 0, "description": "x" }, { "name": "samples", "bits": 32, "initial": 0, "description": "y" }])");
  manifest["tape"] = json::parse(R"({ "comparator": { "low": -1000, "high": -999, "invert": true }, "timer": { "samples": "samples", "ticks": "ticks", "scale": [3, 2], "irq": true } })");
  TapeRig rig(manifest, "", makeImage(16384, 8192));
  rig.chip.reset(true);
  EXPECT_EQ(rig.tape.comparator.low, -1000);
  EXPECT_EQ(rig.tape.comparator.high, -999);
  EXPECT_TRUE(rig.tape.comparator.invert);

  rig.tape.edge(true, 100);  // 上升沿只累加采样数
  EXPECT_EQ(rig.chip.state("samples"), 100U);
  EXPECT_EQ(rig.chip.state("ticks"), 0U);
  EXPECT_FALSE(rig.irq.active());
  rig.tape.edge(false, 50);  // 下降沿：150 个采样折成 150 * 3 / 2 = 225 个计时，清零采样数，拉低 IRQ
  EXPECT_EQ(rig.chip.state("ticks"), 225U);
  EXPECT_EQ(rig.chip.state("samples"), 0U);
  EXPECT_TRUE(rig.irq.active());
  rig.tape.edge(true, 10);
  rig.tape.edge(false, 30);  // 再加 60，按 8 位回绕
  EXPECT_EQ(rig.chip.state("ticks"), (225U + 60U) & 0xFFU);
  EXPECT_EQ(rig.chip.cpuSide().read(0x5000).value, (225 + 60) & 0xFF);
  EXPECT_FALSE(rig.irq.active()) << "读完撤销 IRQ";

  rig.tape.edge(true, 5);
  rig.tape.edge(false, 5);
  ASSERT_TRUE(rig.irq.active());
  rig.chip.reset(false);  // 软复位：计时回到初值，IRQ 撤销
  EXPECT_EQ(rig.chip.state("ticks"), 0U);
  EXPECT_EQ(rig.chip.state("samples"), 0U);
  EXPECT_FALSE(rig.irq.active());
}

TEST(MapperChipDeclared, ADeclaredTapeDoesNothingWhenTheHostHasNoTape) {
  json manifest = withRegisters(R"([{ "bus": "cpu", "range": "$5000-$5000", "read": { "value": 1 } }])", R"([{ "name": "ticks", "bits": 8, "description": "x" }, { "name": "samples", "bits": 32, "description": "y" }])");
  manifest["tape"] = json::parse(R"({ "comparator": { "low": -1, "high": 1 }, "timer": { "samples": "samples", "ticks": "ticks", "scale": [1, 1] } })");
  Rig rig(manifest, "", makeImage(16384, 8192));
  EXPECT_NO_THROW(rig.chip.reset(true));
  EXPECT_EQ(rig.cpu(0x5000).value, 1);
}

TEST(MapperChipDeclaredErrors, ATableNeedsAScriptThatExportsItsFunction) {
  json manifest = withRegisters(R"([{ "bus": "cpu", "range": "$5000-$5000", "write": { "update": { "table": "t", "into": "acc" } } }])", R"([{ "name": "acc", "bits": 8, "description": "x" }])");
  manifest["tables"] = json::parse(R"([{ "id": "t", "index": [{ "from": "data", "shift": 0, "bits": 8 }], "bits": 8 }])");
  try {
    Rig rig(manifest, "", makeImage(16384, 0));
    FAIL() << "应当抛出异常";
  } catch (const ember::shared::CoreError& error) {
    EXPECT_EQ(error.code(), mapper_error::ERROR_INVALID_PACKAGE);
    EXPECT_NE(std::string(error.what()).find("没有 main.js"), std::string::npos) << error.what();
  }
  try {
    Rig rig(manifest, "export default { tables: {} };", makeImage(16384, 0));
    FAIL() << "应当抛出异常";
  } catch (const ember::shared::CoreError& error) {
    EXPECT_EQ(error.code(), mapper_error::ERROR_INVALID_PACKAGE);
    EXPECT_NE(std::string(error.what()).find("没有导出 tables.t"), std::string::npos) << error.what();
  }
  EXPECT_THROW(Rig(manifest, "export default { tables: 5 };", makeImage(16384, 0)), ember::shared::CoreError);
}

TEST(MapperChipDeclaredErrors, ATableFunctionThatReturnsAnIllegalValueOrThrowsStopsTheScript) {
  json manifest = withRegisters(R"([{ "bus": "cpu", "range": "$5000-$5000", "write": { "update": { "table": "t", "into": "acc" } } }])", R"([{ "name": "acc", "bits": 8, "description": "x" }])");
  manifest["tables"] = json::parse(R"([{ "id": "t", "index": [{ "from": "data", "shift": 0, "bits": 8 }], "bits": 8 }])");
  Rig negative(manifest, "export default { tables: { t() { return -1; } } };", makeImage(16384, 0));
  negative.cpuWrite(0x5000, 1);
  ASSERT_TRUE(negative.chip.failure().has_value());
  EXPECT_EQ(negative.chip.failure()->code, mapper_error::ERROR_SCRIPT_ERROR);
  EXPECT_EQ(negative.chip.state("acc"), 0U);

  Rig thrown(manifest, "export default { tables: { t() { throw new Error('boom'); } } };", makeImage(16384, 0));
  thrown.cpuWrite(0x5000, 1);
  ASSERT_TRUE(thrown.chip.failure().has_value());
  EXPECT_NE(thrown.chip.failure()->message.find("boom"), std::string::npos);
  EXPECT_NE(thrown.chip.failure()->message.find("tables.t"), std::string::npos);
}

/** 没有寄存器的最小清单，脚本只处理输入。 */
Rig inputRig(const std::string& script) { return Rig(json::parse(ember::test::minimalManifest()), script, makeImage(16384, 8192)); }

TEST(MapperChipInput, KeyEventsReachOnKeyAndTheScriptSetsPadButtonsByName) {
  Rig rig(json::parse(ember::test::minimalManifest()),
          R"(export default { onKey(ctx, event) {
            if (event.code === 'KeyA') ctx.pad(0).set('left', event.down);
            if (event.code === 'KeyB') ctx.pad(0).set('b', event.down);
            if (event.code === 'KeyP') ctx.pad(1).set('start', event.down);
            if (event.code === 'KeyR') ctx.pad(0).release();
          } };)",
          makeImage(16384, 8192));
  rig.chip.input(hw::KeyInput{"KeyA", true});
  EXPECT_EQ(rig.pads.masks[0], hw::pad_button::LEFT);
  rig.chip.input(hw::KeyInput{"KeyB", true});
  EXPECT_EQ(rig.pads.masks[0], hw::pad_button::LEFT | hw::pad_button::B);
  rig.chip.input(hw::KeyInput{"KeyP", true});
  EXPECT_EQ(rig.pads.masks[1], hw::pad_button::START);
  rig.chip.input(hw::KeyInput{"KeyA", false});
  EXPECT_EQ(rig.pads.masks[0], hw::pad_button::B);
  rig.chip.input(hw::KeyInput{"KeyR", true});
  EXPECT_EQ(rig.pads.masks[0], 0);
  EXPECT_EQ(rig.pads.masks[1], hw::pad_button::START);
  rig.chip.input(hw::KeyInput{"KeyQ", true});  // 脚本没有处理的键什么也不发生
  EXPECT_FALSE(rig.chip.failure().has_value());
}

TEST(MapperChipInput, ScriptsSeeWhichKeysAreHeldSoCombinationsWork) {
  Rig rig(json::parse(ember::test::minimalManifest()),
          R"(export default { onKey(ctx, event) {
            if (event.code === 'KeyZ' && ctx.keys.isDown('ShiftLeft')) ctx.pad(0).set('select', event.down);
          } };)",
          makeImage(16384, 8192));
  rig.chip.input(hw::KeyInput{"KeyZ", true});
  EXPECT_EQ(rig.pads.masks[0], 0);
  rig.chip.input(hw::KeyInput{"KeyZ", false});
  rig.chip.input(hw::KeyInput{"ShiftLeft", true});
  rig.chip.input(hw::KeyInput{"KeyZ", true});
  EXPECT_EQ(rig.pads.masks[0], hw::pad_button::SELECT);
  rig.chip.input(hw::KeyInput{"ShiftLeft", false});
  rig.chip.input(hw::KeyInput{"KeyZ", false});
  EXPECT_EQ(rig.pads.masks[0], hw::pad_button::SELECT);  // 松开时 Shift 已经松开，组合不成立，选择键保持按下是脚本自己的选择
}

TEST(MapperChipInput, MouseEventsCarryTheirFieldsToTheScript) {
  Rig rig(json::parse(ember::test::minimalManifest()),
          R"(export default {
            onMouse(ctx, event) { ctx.log.warn(JSON.stringify(event)); }
          };)",
          makeImage(16384, 8192));
  hw::MouseInput mouse;
  mouse.kind = hw::MouseKind::Wheel;
  mouse.x = 10.5;
  mouse.y = 20;
  mouse.dx = -1;
  mouse.dy = 2;
  mouse.buttons = 5;
  mouse.wheel = 3;
  rig.chip.input(mouse);
  ASSERT_EQ(rig.chip.scriptLog().size(), 1U);
  EXPECT_EQ(json::parse(rig.chip.scriptLog()[0]), json::parse(R"({"kind":"wheel","x":10.5,"y":20,"dx":-1,"dy":2,"buttons":5,"wheel":3})"));
  EXPECT_FALSE(rig.chip.failure().has_value());
}

TEST(MapperChipInput, EventsWithoutAHandlerAreIgnored) {
  Rig noScript(json::parse(ember::test::minimalManifest()), "", makeImage(16384, 8192));
  noScript.chip.input(hw::KeyInput{"KeyA", true});
  noScript.chip.input(hw::MouseInput{});
  EXPECT_EQ(noScript.pads.calls, 0);

  Rig onlyKeys = inputRig("export default { onKey() {} };");
  onlyKeys.chip.input(hw::MouseInput{});
  EXPECT_FALSE(onlyKeys.chip.failure().has_value());
}

TEST(MapperChipInput, BadPadCallsAreScriptErrorsAndStopTheScript) {
  Rig badButton = inputRig("export default { onKey(ctx, event) { ctx.pad(0).set('turbo', true); } };");
  badButton.chip.input(hw::KeyInput{"KeyA", true});
  ASSERT_TRUE(badButton.chip.failure().has_value());
  EXPECT_EQ(badButton.chip.failure()->code, mapper_error::ERROR_SCRIPT_ERROR);
  EXPECT_NE(badButton.chip.failure()->message.find("onKey"), std::string::npos);

  Rig badPort = inputRig("export default { onKey(ctx, event) { ctx.pad(2); } };");
  badPort.chip.input(hw::KeyInput{"KeyA", true});
  ASSERT_TRUE(badPort.chip.failure().has_value());
  EXPECT_EQ(badPort.chip.failure()->code, mapper_error::ERROR_SCRIPT_ERROR);

  Rig badValue = inputRig("export default { onKey(ctx, event) { ctx.pad(0).set('a', 1); } };");
  badValue.chip.input(hw::KeyInput{"KeyA", true});
  EXPECT_TRUE(badValue.chip.failure().has_value());

  // 出错之后不再调用脚本，复位清除错误。
  badButton.chip.input(hw::KeyInput{"KeyB", true});
  EXPECT_EQ(badButton.pads.calls, 0);
  badButton.chip.reset(false);
  EXPECT_FALSE(badButton.chip.failure().has_value());
}

}  // namespace
