/*
 * ------------------------------------------------------------------------
 * 名称：manifest-test.cpp
 * 说明：插件清单检查的测试：合格的清单与各字段的解析，以及缺字段、不认识的字段、类型与范围不对、窗口重叠与越界、存储区与窗口的搭配、寄存器、状态、设置项、设备连线等错误。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：错误用例都在合格清单的基础上改一处，保证每个用例只因这一处被拒绝；错误码为 mappers.invalidPackage，说明里指出出错的字段。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <string>

#include <nlohmann/json.hpp>

#include "../support/plugin-test-support.hpp"
#include "contracts/mappers/mapper-error.hpp"
#include "infra/mappers/manifest.hpp"
#include "shared/errors/core-error.hpp"

namespace {

using nlohmann::json;
using namespace ember::infra::mappers;

/** 合格的最小清单。 */
json base() { return json::parse(ember::test::minimalManifest()); }

/** 带齐全部可选部分的合格清单。 */
json full() {
  json manifest = base();
  manifest["description"] = "说明";
  manifest["match"] = json::parse(R"([{ "mapper": 171, "variant": 1 }, { "mapper": 172, "prgSize": 524288, "chrSize": 0, "crc32Prg": "A1b2C3d4", "crc32All": "00000001" }])");
  manifest["storage"].push_back(json::parse(R"({ "id": "ext-ram", "kind": "ram", "size": 32768, "battery": true })"));
  manifest["windows"].push_back(json::parse(R"({ "id": "ext", "bus": "cpu", "range": "$6000-$7FFF", "storage": "ext-ram", "size": 8192, "initial": 3, "access": "rw" })"));
  manifest["nametable"] = "vertical";
  manifest["registers"] = json::parse(R"([{ "bus": "cpu", "range": "$4100-$4107", "read": true, "write": true }, { "bus": "ppu", "range": "$1000-$1FFF", "write": true }])");
  manifest["wires"] = json::parse(R"([{ "bus": "cpu", "address": "$4200-$4207", "device": "fdc", "register": 0, "read": true, "write": true }])");
  manifest["uses"] = json::parse(R"(["fdc"])");
  manifest["irq"] = json::parse(R"([{ "from": "fdc.irq", "to": "cart" }])");
  manifest["state"] = json::parse(R"([{ "name": "prgBank", "bits": 8, "description": "PRG bank" }, { "name": "flags", "bits": 32, "description": "标志" }])");
  manifest["settings"] = json::parse(R"([
    { "key": "title", "label": "标题", "kind": "text", "default": "x" },
    { "key": "mode", "label": "模式", "kind": "choice", "default": "a", "options": [{ "value": "a", "label": "甲" }, { "value": "b", "label": "乙" }] },
    { "key": "fast", "label": "快速", "kind": "switch", "default": true, "description": "加速" }
  ])");
  return manifest;
}

/** 解析应当失败，且说明里含有指定片段。 */
void expectInvalid(const json& manifest, const std::string& fragment) {
  try {
    static_cast<void>(parseManifest(manifest.dump()));
    FAIL() << "应当抛出异常，期望的说明片段：" << fragment;
  } catch (const ember::shared::CoreError& error) {
    EXPECT_EQ(error.code(), ember::contracts::mappers::ERROR_INVALID_PACKAGE);
    EXPECT_NE(std::string(error.what()).find(fragment), std::string::npos) << error.what();
  }
}

TEST(Manifest, ParsesTheMinimalManifest) {
  const Manifest manifest = parseManifest(base().dump());
  EXPECT_EQ(manifest.id, "test-plugin");
  EXPECT_EQ(manifest.name, "测试插件");
  EXPECT_EQ(manifest.versionNumber, (Version{1, 0, 0}));
  EXPECT_EQ(manifest.apiVersion, 1U);
  EXPECT_EQ(manifest.host, std::vector<std::string>{"fc"});
  ASSERT_EQ(manifest.match.size(), 1U);
  EXPECT_EQ(manifest.match[0].mapper, 7U);
  ASSERT_EQ(manifest.storage.size(), 2U);
  EXPECT_FALSE(manifest.storage[0].ram);
  EXPECT_EQ(manifest.storage[0].from, StorageSource::FilePrg);
  ASSERT_TRUE(manifest.storage[1].fallback.has_value());
  EXPECT_EQ(manifest.storage[1].fallback->size, 8192U);
  ASSERT_EQ(manifest.windows.size(), 3U);
  EXPECT_EQ(manifest.windows[0].range.first, 0x8000U);
  EXPECT_EQ(manifest.windows[0].range.last, 0xBFFFU);
  EXPECT_EQ(manifest.windows[0].initialKind, InitialKind::First);
  EXPECT_EQ(manifest.windows[1].initialKind, InitialKind::Last);
  EXPECT_EQ(manifest.windows[2].initialKind, InitialKind::Bank);
  EXPECT_TRUE(manifest.windows[2].ppu);
  EXPECT_TRUE(manifest.windows[2].writable);
  EXPECT_EQ(manifest.nametable, NametableMode::Header);
  EXPECT_TRUE(manifest.registers.empty());
  EXPECT_FALSE(manifest.needsReadHandler());
  EXPECT_FALSE(manifest.needsWriteHandler());
}

TEST(Manifest, ParsesAManifestWithEveryOptionalPart) {
  const Manifest manifest = parseManifest(full().dump());
  EXPECT_EQ(manifest.description, "说明");
  ASSERT_EQ(manifest.match.size(), 2U);
  EXPECT_EQ(manifest.match[0].variant, 1U);
  EXPECT_EQ(manifest.match[0].conditionCount(), 2);
  EXPECT_EQ(manifest.match[1].conditionCount(), 5);
  EXPECT_EQ(manifest.match[1].crc32Prg, 0xA1B2C3D4U);
  EXPECT_EQ(manifest.match[1].crc32All, 1U);
  EXPECT_TRUE(manifest.storage[2].ram);
  EXPECT_TRUE(manifest.storage[2].battery);
  EXPECT_EQ(manifest.windows[3].initialBank, 3U);
  EXPECT_EQ(manifest.nametable, NametableMode::Vertical);
  ASSERT_EQ(manifest.registers.size(), 2U);
  EXPECT_TRUE(manifest.registers[0].read && manifest.registers[0].write);
  EXPECT_TRUE(manifest.registers[1].ppu && !manifest.registers[1].read);
  EXPECT_TRUE(manifest.needsReadHandler());
  EXPECT_TRUE(manifest.needsWriteHandler());
  ASSERT_EQ(manifest.wires.size(), 1U);
  EXPECT_EQ(manifest.wires[0].device, "fdc");
  EXPECT_EQ(manifest.wires[0].address.first, 0x4200U);
  EXPECT_EQ(manifest.uses, std::vector<std::string>{"fdc"});
  EXPECT_EQ(manifest.irq[0].from, "fdc.irq");
  ASSERT_EQ(manifest.state.size(), 2U);
  EXPECT_EQ(manifest.state[1].bits, 32U);
  ASSERT_EQ(manifest.settings.size(), 3U);
  EXPECT_EQ(manifest.settings[1].kind, SettingKind::Choice);
  EXPECT_EQ(manifest.settings[1].options.size(), 2U);
  EXPECT_TRUE(manifest.settings[2].switchDefault);
}

TEST(Manifest, MatchRulesRequireEveryWrittenConditionAndCountThem) {
  ember::contracts::mappers::CartridgeDescription description;
  description.mapper = 171;
  description.variant = 1;
  description.prgSize = 524288;
  description.chrSize = 0;
  description.crc32Prg = 0x1234;
  MatchRule rule;
  rule.mapper = 171;
  EXPECT_TRUE(rule.matches(description));
  EXPECT_EQ(rule.conditionCount(), 1);
  rule.variant = 1;
  rule.prgSize = 524288;
  EXPECT_TRUE(rule.matches(description));
  EXPECT_EQ(rule.conditionCount(), 3);
  rule.chrSize = 8192;
  EXPECT_FALSE(rule.matches(description));
  rule.chrSize = 0;
  rule.crc32Prg = 0x1235;
  EXPECT_FALSE(rule.matches(description));
  rule.crc32Prg = 0x1234;
  EXPECT_TRUE(rule.matches(description));
  rule.mapper = 3;
  EXPECT_FALSE(rule.matches(description));
}

TEST(Manifest, FormatsRanges) {
  EXPECT_EQ(formatRange(AddressRange{0x8000, 0x9FFF}), "$8000-$9FFF");
  EXPECT_EQ(formatRange(AddressRange{0x4100, 0x4100}), "$4100");
  EXPECT_EQ(formatRange(AddressRange{0x0, 0x1FFF}), "$0000-$1FFF");
}

TEST(ManifestErrors, RejectsInputThatIsNotAJsonObject) {
  try {
    static_cast<void>(parseManifest("{"));
    FAIL();
  } catch (const ember::shared::CoreError& error) {
    EXPECT_EQ(error.code(), ember::contracts::mappers::ERROR_INVALID_PACKAGE);
    EXPECT_NE(std::string(error.what()).find("JSON"), std::string::npos);
  }
  expectInvalid(json::array(), "应为对象");
}

TEST(ManifestErrors, RejectsAMissingRequiredField) {
  for (const char* key : {"id", "name", "version", "apiVersion", "publisher", "host", "match"}) {
    json manifest = base();
    manifest.erase(key);
    expectInvalid(manifest, std::string("manifest.") + key + " 缺少");
  }
}

TEST(ManifestErrors, RejectsUnknownFieldsAtEveryLevel) {
  json manifest = base();
  manifest["extra"] = 1;
  expectInvalid(manifest, "manifest.extra 是不认识的字段");
  manifest = base();
  manifest["windows"][0]["extra"] = 1;
  expectInvalid(manifest, "windows[0].extra");
  manifest = base();
  manifest["match"][0]["extra"] = 1;
  expectInvalid(manifest, "match[0].extra");
  manifest = base();
  manifest["storage"][1]["fallback"]["extra"] = 1;
  expectInvalid(manifest, "fallback.extra");
}

TEST(ManifestErrors, RejectsBadBasicFields) {
  json manifest = base();
  manifest["id"] = "Bad_Id";
  expectInvalid(manifest, "manifest.id");
  manifest["id"] = "ab";
  expectInvalid(manifest, "manifest.id");
  manifest["id"] = "-abc";
  expectInvalid(manifest, "manifest.id");
  manifest = base();
  manifest["name"] = std::string(41, 'x');
  expectInvalid(manifest, "manifest.name");
  for (const char* version : {"1.0", "1.0.0.0", "a.b.c", "01.0.0", "1..0", "1.0.", ""}) {
    manifest = base();
    manifest["version"] = version;
    expectInvalid(manifest, "manifest.version");
  }
  manifest = base();
  manifest["apiVersion"] = 2;
  expectInvalid(manifest, "只支持插件接口版本 1");
  manifest = base();
  manifest["apiVersion"] = "1";
  expectInvalid(manifest, "manifest.apiVersion");
  manifest = base();
  manifest["host"] = json::array();
  expectInvalid(manifest, "manifest.host 至少 1 项");
  manifest = base();
  manifest["host"] = json::array({"fc", "fc"});
  expectInvalid(manifest, "host 里的型号重复");
  manifest = base();
  manifest["host"] = json::array({"FC"});
  expectInvalid(manifest, "manifest.host[0]");
  manifest = base();
  manifest["match"] = json::array();
  expectInvalid(manifest, "manifest.match 至少 1 项");
}

TEST(ManifestErrors, RejectsBadMatchRules) {
  json manifest = base();
  manifest["match"] = json::parse(R"([{ "variant": 1 }])");
  expectInvalid(manifest, "match[0].mapper 缺少");
  manifest["match"] = json::parse(R"([{ "mapper": 5000 }])");
  expectInvalid(manifest, "match[0].mapper");
  manifest["match"] = json::parse(R"([{ "mapper": 1, "crc32Prg": "XYZ12345" }])");
  expectInvalid(manifest, "8 位十六进制");
  manifest["match"] = json::parse(R"([{ "mapper": 1, "crc32All": "1234" }])");
  expectInvalid(manifest, "crc32All");
  manifest["match"] = json::parse(R"([{ "mapper": 1, "variant": -1 }])");
  expectInvalid(manifest, "variant");
}

TEST(ManifestErrors, RejectsBadAddressRanges) {
  for (const char* range : {"8000-9FFF", "$A000-$8000", "$80000-$9FFF", "$8000-", "-$9FFF", "$GGGG-$HHHH", "$8000 - $9FFF", ""}) {
    json manifest = base();
    manifest["windows"][0]["range"] = range;
    expectInvalid(manifest, "windows[0].range");
  }
}

TEST(ManifestErrors, RejectsRangesOutsideTheBusAreas) {
  json manifest = base();
  manifest["windows"][0]["range"] = "$4000-$7FFF";  // 低于卡带的起点 $4020
  expectInvalid(manifest, "$4020-$FFFF");
  manifest = base();
  manifest["windows"][2]["range"] = "$0000-$3FFF";  // PPU 侧只能到 $1FFF
  manifest["windows"][2]["size"] = 16384;
  expectInvalid(manifest, "$0000-$1FFF");
  manifest = base();
  manifest["registers"] = json::parse(R"([{ "bus": "cpu", "range": "$2000-$2007", "write": true }])");
  expectInvalid(manifest, "registers[0].range");
}

TEST(ManifestErrors, RejectsWindowsThatDoNotFitTheirRangeOrStorage) {
  json manifest = base();
  manifest["windows"][0]["size"] = 8192;
  expectInvalid(manifest, "必须等于地址范围的大小 16384");
  manifest = base();
  manifest["windows"][0]["storage"] = "nothing";
  expectInvalid(manifest, "引用了不存在的存储区：nothing");
  manifest = base();
  manifest["windows"][0]["access"] = "x";
  expectInvalid(manifest, "windows[0].access");
  manifest = base();
  manifest["windows"][0]["initial"] = "middle";
  expectInvalid(manifest, "windows[0].initial");
  manifest = base();
  manifest["windows"][0]["initial"] = -1;
  expectInvalid(manifest, "windows[0].initial");
  manifest = base();
  manifest["windows"][0]["bus"] = "usb";
  expectInvalid(manifest, "windows[0].bus");
}

TEST(ManifestErrors, RejectsOverlappingWindowsOnTheSameBus) {
  json manifest = base();
  manifest["windows"][1]["range"] = "$A000-$DFFF";
  expectInvalid(manifest, "prg-low 与 prg-high 的地址范围重叠");
  manifest = base();
  manifest["windows"].push_back(json::parse(R"({ "id": "chr2", "bus": "ppu", "range": "$0000-$0FFF", "storage": "chr", "size": 4096, "initial": 0, "access": "r" })"));
  expectInvalid(manifest, "chr 与 chr2 的地址范围重叠");
  manifest = base();
  manifest["windows"][2]["range"] = "$0000-$1FFF";
  manifest["windows"][0]["bus"] = "ppu";
  manifest["windows"][0]["range"] = "$0000-$1FFF";
  manifest["windows"][0]["size"] = 8192;
  expectInvalid(manifest, "重叠");
}

TEST(ManifestErrors, RejectsDuplicateIds) {
  json manifest = base();
  manifest["storage"][1]["id"] = "prg";
  expectInvalid(manifest, "storage 里的标识重复：prg");
  manifest = base();
  manifest["windows"][1]["id"] = "prg-low";
  expectInvalid(manifest, "windows 里的标识重复：prg-low");
}

TEST(ManifestErrors, ARomWindowCannotBeWritableUnlessTheStorageHasARamFallback) {
  json manifest = base();
  manifest["windows"][0]["access"] = "rw";
  expectInvalid(manifest, "窗口 prg-low 指向 ROM，access 只能是 r");
  // chr 有 RAM 后备，可以是 rw（文件里没有 CHR 时它是 RAM，有 CHR 时写入被 ROM 拒绝）。
  EXPECT_NO_THROW(static_cast<void>(parseManifest(base().dump())));
}

TEST(ManifestErrors, RejectsBadStorageDeclarations) {
  json manifest = base();
  manifest["storage"][0]["from"] = "file.misc";
  expectInvalid(manifest, "storage[0].from");
  manifest = base();
  manifest["storage"][0].erase("from");
  expectInvalid(manifest, "storage[0].from 缺少");
  manifest = base();
  manifest["storage"][0]["size"] = 16;
  expectInvalid(manifest, "size 只用于 ram");
  manifest = base();
  manifest["storage"][0]["battery"] = true;
  expectInvalid(manifest, "battery 只用于 ram");
  manifest = base();
  manifest["storage"].push_back(json::parse(R"({ "id": "ram1", "kind": "ram" })"));
  expectInvalid(manifest, "storage[2].size 缺少");
  manifest["storage"][2]["size"] = 0;
  expectInvalid(manifest, "storage[2].size");
  manifest["storage"][2]["size"] = 20000000;
  expectInvalid(manifest, "storage[2].size");
  manifest["storage"][2]["size"] = 1024;
  manifest["storage"][2]["from"] = "file.prg";
  expectInvalid(manifest, "from 只用于 rom");
  manifest["storage"][2].erase("from");
  manifest["storage"][2]["fallback"] = json::parse(R"({ "kind": "ram", "size": 16 })");
  expectInvalid(manifest, "fallback 只用于 rom");
  manifest = base();
  manifest["storage"][1]["fallback"]["kind"] = "rom";
  expectInvalid(manifest, "fallback.kind");
  manifest = base();
  manifest["storage"][1]["fallback"].erase("size");
  expectInvalid(manifest, "fallback.size 缺少");
}

TEST(ManifestErrors, RejectsBadRegisterDeclarations) {
  json manifest = base();
  manifest["registers"] = json::parse(R"([{ "bus": "cpu", "range": "$8000-$FFFF" }])");
  expectInvalid(manifest, "read 与 write 至少要有一个为 true");
  manifest["registers"] = json::parse(R"([{ "bus": "cpu", "range": "$8000-$8FFF", "write": true }, { "bus": "cpu", "range": "$8800-$9FFF", "read": true }])");
  expectInvalid(manifest, "registers 的第 0 项与第 1 项的地址范围重叠");
  manifest["registers"] = json::parse(R"([{ "bus": "cpu", "range": "$8000-$8FFF", "write": 1 }])");
  expectInvalid(manifest, "registers[0].write");
  // 寄存器范围可以与窗口重叠。
  manifest["registers"] = json::parse(R"([{ "bus": "cpu", "range": "$8000-$FFFF", "write": true }])");
  EXPECT_NO_THROW(static_cast<void>(parseManifest(manifest.dump())));
}

TEST(ManifestErrors, RejectsBadStateDeclarations) {
  json manifest = base();
  manifest["state"] = json::parse(R"([{ "name": "a", "bits": 0, "description": "x" }])");
  expectInvalid(manifest, "state[0].bits");
  manifest["state"] = json::parse(R"([{ "name": "a", "bits": 33, "description": "x" }])");
  expectInvalid(manifest, "state[0].bits");
  manifest["state"] = json::parse(R"([{ "name": "Bad", "bits": 8, "description": "x" }])");
  expectInvalid(manifest, "state[0].name");
  manifest["state"] = json::parse(R"([{ "name": "a_b", "bits": 8, "description": "x" }])");
  expectInvalid(manifest, "state[0].name");
  manifest["state"] = json::parse(R"([{ "name": "a", "bits": 8 }])");
  expectInvalid(manifest, "state[0].description 缺少");
  manifest["state"] = json::parse(R"([{ "name": "a", "bits": 8, "description": "x" }, { "name": "a", "bits": 4, "description": "y" }])");
  expectInvalid(manifest, "state 里的名称重复：a");
}

TEST(ManifestErrors, RejectsBadSettings) {
  json manifest = base();
  manifest["settings"] = json::parse(R"([{ "key": "mode", "label": "模式", "kind": "choice", "default": "z", "options": [{ "value": "a", "label": "甲" }] }])");
  expectInvalid(manifest, "必须是 options 里的一个取值");
  manifest["settings"] = json::parse(R"([{ "key": "mode", "label": "模式", "kind": "choice", "default": "a" }])");
  expectInvalid(manifest, "settings[0].options 缺少");
  manifest["settings"] = json::parse(R"([{ "key": "mode", "label": "模式", "kind": "text", "default": "a", "options": [] }])");
  expectInvalid(manifest, "options 只用于 choice");
  manifest["settings"] = json::parse(R"([{ "key": "fast", "label": "快速", "kind": "switch", "default": "yes" }])");
  expectInvalid(manifest, "settings[0].default");
  manifest["settings"] = json::parse(R"([{ "key": "fast", "label": "快速", "kind": "slider", "default": 1 }])");
  expectInvalid(manifest, "settings[0].kind");
  manifest["settings"] = json::parse(R"([{ "key": "a", "label": "甲", "kind": "text", "default": "" }, { "key": "a", "label": "乙", "kind": "text", "default": "" }])");
  expectInvalid(manifest, "settings 里的键重复：a");
}

TEST(ManifestErrors, RejectsBadWiresAndIrqWires) {
  json manifest = base();
  manifest["wires"] = json::parse(R"([{ "bus": "cpu", "address": "$4200", "device": "fdc", "register": 256, "read": true }])");
  expectInvalid(manifest, "wires[0].register");
  manifest["wires"] = json::parse(R"([{ "bus": "cpu", "address": "$4200", "device": "FDC", "register": 0, "read": true }])");
  expectInvalid(manifest, "wires[0].device");
  manifest["wires"] = json::parse(R"([{ "bus": "cpu", "address": "$4200", "device": "fdc", "register": 0 }])");
  expectInvalid(manifest, "read 与 write 至少要有一个为 true");
  manifest = base();
  manifest["irq"] = json::parse(R"([{ "from": "fdc", "to": "cart" }])");
  expectInvalid(manifest, "irq[0].from 应写成 设备标识.引脚");
  manifest["irq"] = json::parse(R"([{ "from": "fdc.irq", "to": "cpu" }])");
  expectInvalid(manifest, "irq[0].to");
}

TEST(ManifestErrors, RejectsABadNametableAndTooManyItems) {
  json manifest = base();
  manifest["nametable"] = "diagonal";
  expectInvalid(manifest, "manifest.nametable");
  manifest = base();
  for (int index = 0; index < 100; ++index) manifest["windows"].push_back(manifest["windows"][0]);
  expectInvalid(manifest, "manifest.windows 最多 64 项");
}

}  // namespace
