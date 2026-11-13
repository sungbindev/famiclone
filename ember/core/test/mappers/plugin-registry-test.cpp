/*
 * ------------------------------------------------------------------------
 * 名称：plugin-registry-test.cpp
 * 说明：插件登记的测试：安装与列表、详情、按卡带描述匹配（条件数、冲突、停用）、主机型号与设备检查、安装时的脚本试加载、
 *       预装与用户目录的版本优先、卸载与预装只读、停用状态的保存、重新加载与不合格包的跳过、事件、按文件安装、创建卡带、并发使用。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：安装的包都在测试里现场生成；带目录的用例用临时目录。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <nlohmann/json.hpp>

#include "../support/plugin-test-support.hpp"
#include "contracts/mappers/mapper-error.hpp"
#include "infra/mappers/plugin-registry.hpp"
#include "shared/errors/core-error.hpp"

namespace {

using nlohmann::json;
using ember::infra::mappers::PluginRegistry;
using ember::infra::mappers::PluginRegistryOptions;
using ember::test::TempDir;
namespace api = ember::contracts::mappers;
namespace fs = std::filesystem;

/** 清单加可选的脚本，打成包。 */
std::vector<std::uint8_t> package(const json& manifest, const std::string& script = "") {
  std::vector<ember::test::PackageFile> files = {{"manifest.json", manifest.dump()}};
  if (!script.empty()) files.emplace_back("main.js", script);
  return ember::test::makeEmap(files);
}

/** 指定标识、mapper 号与版本的清单。 */
json manifestFor(const std::string& id, int mapper, const std::string& version = "1.0.0") {
  json manifest = json::parse(ember::test::minimalManifest(id, mapper));
  manifest["version"] = version;
  return manifest;
}

api::CartridgeDescription described(std::uint32_t mapper, std::uint32_t variant = 0) {
  api::CartridgeDescription description;
  description.mapper = mapper;
  description.variant = variant;
  return description;
}

/** 执行应当抛出 CoreError，错误码与说明里的片段符合。 */
template <typename Call>
void expectError(Call&& call, std::string_view code, const std::string& fragment = "") {
  try {
    call();
    FAIL() << "应当抛出异常：" << code;
  } catch (const ember::shared::CoreError& error) {
    EXPECT_EQ(error.code(), code);
    EXPECT_NE(std::string(error.what()).find(fragment), std::string::npos) << error.what();
  }
}

template <typename Result>
std::vector<std::string> idsOf(const Result& result) {
  std::vector<std::string> ids;
  for (const auto& plugin : result.plugins) ids.push_back(plugin.id);
  return ids;
}

TEST(PluginRegistry, InstallsAPluginAndListsItWithItsSummary) {
  PluginRegistry registry(ember::test::fcRegistryOptions());
  EXPECT_TRUE(registry.list().plugins.empty());
  const auto summary = registry.installPackage(ember::test::officialPluginPackage("nrom"));
  EXPECT_EQ(summary.id, "nrom");
  EXPECT_EQ(summary.version, "1.0.0");
  EXPECT_EQ(summary.source, "user");
  EXPECT_TRUE(summary.enabled);
  EXPECT_EQ(summary.host, std::vector<std::string>{"fc"});
  ASSERT_EQ(summary.match.size(), 1U);
  EXPECT_EQ(summary.match[0].mapper, 0);
  EXPECT_FALSE(summary.match[0].variant.has_value());
  const auto list = registry.list();
  ASSERT_EQ(list.plugins.size(), 1U);
  EXPECT_EQ(list.plugins[0].id, "nrom");
}

TEST(PluginRegistry, InfoReturnsTheManifestDetails) {
  PluginRegistry registry(ember::test::fcRegistryOptions());
  static_cast<void>(registry.installPackage(ember::test::officialPluginPackage("nrom")));
  const auto detail = registry.info("nrom").plugin;
  EXPECT_EQ(detail.summary.id, "nrom");
  ASSERT_EQ(detail.storage.size(), 3U);
  EXPECT_EQ(detail.storage[0].id, "prg");
  EXPECT_EQ(detail.storage[0].kind, "rom");
  EXPECT_EQ(detail.storage[0].from, "file.prg");
  EXPECT_EQ(detail.storage[1].fallbackSize, 8192);
  EXPECT_EQ(detail.storage[2].kind, "ram");
  EXPECT_EQ(detail.storage[2].size, 8192);
  ASSERT_EQ(detail.windows.size(), 4U);
  EXPECT_EQ(detail.windows[1].id, "prg-low");
  EXPECT_EQ(detail.windows[1].range, "$8000-$BFFF");
  EXPECT_EQ(detail.windows[1].initial, "first");
  EXPECT_EQ(detail.windows[1].access, "r");
  EXPECT_EQ(detail.windows[2].initial, "last");
  EXPECT_EQ(detail.windows[3].bus, "ppu");
  EXPECT_EQ(detail.windows[3].access, "rw");
  EXPECT_EQ(detail.nametable, "header");
  EXPECT_TRUE(detail.registers.empty());
  expectError([&] { static_cast<void>(registry.info("missing")); }, api::ERROR_NOT_FOUND, "missing");
}

TEST(PluginRegistry, InfoShowsRegistersStateSettingsAndWiresOfARichManifest) {
  PluginRegistryOptions options = ember::test::fcRegistryOptions();
  options.hosts.hosts["fc"] = {"fdc"};
  PluginRegistry registry(options);
  json manifest = manifestFor("rich-plugin", 9);
  manifest["registers"] = json::parse(R"([{ "bus": "cpu", "range": "$5000-$5003", "write": true }])");
  manifest["wires"] = json::parse(R"([{ "bus": "cpu", "address": "$4200-$4207", "device": "fdc", "register": 0, "read": true, "write": true }])");
  manifest["uses"] = json::parse(R"(["fdc"])");
  manifest["state"] = json::parse(R"([{ "name": "bank", "bits": 6, "description": "bank 号" }])");
  manifest["settings"] = json::parse(R"([{ "key": "mode", "label": "模式", "kind": "choice", "default": "a", "options": [{ "value": "a", "label": "甲" }] }, { "key": "on", "label": "开", "kind": "switch", "default": true }])");
  manifest["match"] = json::parse(R"([{ "mapper": 9, "variant": 1, "crc32Prg": "0000ABCD" }])");
  static_cast<void>(registry.installPackage(package(manifest, "export default { write() {} };")));
  const auto detail = registry.info("rich-plugin").plugin;
  ASSERT_EQ(detail.registers.size(), 1U);
  EXPECT_EQ(detail.registers[0].range, "$5000-$5003");
  EXPECT_TRUE(detail.registers[0].write);
  EXPECT_FALSE(detail.registers[0].read);
  ASSERT_EQ(detail.wires.size(), 1U);
  EXPECT_EQ(detail.wires[0].address, "$4200-$4207");
  EXPECT_EQ(detail.wires[0].device, "fdc");
  EXPECT_EQ(detail.uses, std::vector<std::string>{"fdc"});
  ASSERT_EQ(detail.state.size(), 1U);
  EXPECT_EQ(detail.state[0].bits, 6);
  ASSERT_EQ(detail.settings.size(), 2U);
  EXPECT_EQ(detail.settings[0].kind, "choice");
  EXPECT_EQ(detail.settings[0].defaultValue, json("a"));
  EXPECT_EQ(detail.settings[0].options->size(), 1U);
  EXPECT_EQ(detail.settings[1].defaultValue, json(true));
  EXPECT_EQ(detail.summary.match[0].crc32Prg, "0000ABCD");
}

TEST(PluginRegistryMatch, FindsThePluginByMapperNumberAndChoosesTheFirstExistingHost) {
  PluginRegistry registry(ember::test::fcRegistryOptions());
  json manifest = manifestFor("multi-host", 4);
  manifest["host"] = json::parse(R"(["sb2000", "fc"])");
  static_cast<void>(registry.installPackage(package(manifest)));
  static_cast<void>(registry.installPackage(ember::test::officialPluginPackage("nrom")));
  const auto match = registry.match(described(4));
  EXPECT_EQ(match.pluginId, "multi-host");
  EXPECT_EQ(match.host, "fc");
  EXPECT_EQ(match.version, "1.0.0");
  EXPECT_EQ(registry.match(described(0)).pluginId, "nrom");
}

TEST(PluginRegistryMatch, ReportsNoMatchWithTheMapperAndVariant) {
  PluginRegistry registry(ember::test::fcRegistryOptions());
  static_cast<void>(registry.installPackage(ember::test::officialPluginPackage("nrom")));
  expectError([&] { static_cast<void>(registry.match(described(5, 2))); }, api::ERROR_NO_MATCH, "mapper 号 5、变体 2");
}

TEST(PluginRegistryMatch, PluginsWithTheSameNumberOfConditionsConflict) {
  PluginRegistry registry(ember::test::fcRegistryOptions());
  static_cast<void>(registry.installPackage(package(manifestFor("plug-a", 7))));
  static_cast<void>(registry.installPackage(package(manifestFor("plug-b", 7))));
  expectError([&] { static_cast<void>(registry.match(described(7))); }, api::ERROR_CONFLICT, "plug-a、plug-b");
}

TEST(PluginRegistryMatch, MoreWrittenConditionsWinAndDisablingResolvesAConflict) {
  PluginRegistry registry(ember::test::fcRegistryOptions());
  static_cast<void>(registry.installPackage(package(manifestFor("plug-a", 7))));
  static_cast<void>(registry.installPackage(package(manifestFor("plug-b", 7))));
  json specific = manifestFor("plug-c", 7);
  specific["match"] = json::parse(R"([{ "mapper": 7, "variant": 1 }])");
  static_cast<void>(registry.installPackage(package(specific)));
  EXPECT_EQ(registry.match(described(7, 1)).pluginId, "plug-c");  // 两项条件胜过一项
  expectError([&] { static_cast<void>(registry.match(described(7, 0))); }, api::ERROR_CONFLICT);

  EXPECT_FALSE(registry.setEnabled("plug-b", false).plugin.enabled);
  EXPECT_EQ(registry.match(described(7, 0)).pluginId, "plug-a");
  static_cast<void>(registry.setEnabled("plug-a", false));
  expectError([&] { static_cast<void>(registry.match(described(7, 0))); }, api::ERROR_NO_MATCH, "已被停用");
  EXPECT_TRUE(registry.setEnabled("plug-a", true).plugin.enabled);
  EXPECT_EQ(registry.match(described(7, 0)).pluginId, "plug-a");
}

TEST(PluginRegistryMatch, TheBestConditionOfAMultiRulePluginCounts) {
  PluginRegistry registry(ember::test::fcRegistryOptions());
  json many = manifestFor("many-rules", 7);
  many["match"] = json::parse(R"([{ "mapper": 7 }, { "mapper": 7, "prgSize": 16384, "chrSize": 8192 }])");
  static_cast<void>(registry.installPackage(package(many)));
  static_cast<void>(registry.installPackage(package(manifestFor("plain-rule", 7))));
  api::CartridgeDescription description = described(7);
  description.prgSize = 16384;
  description.chrSize = 8192;
  EXPECT_EQ(registry.match(description).pluginId, "many-rules");  // 3 项条件胜过 1 项
  description.chrSize = 0;
  expectError([&] { static_cast<void>(registry.match(description)); }, api::ERROR_CONFLICT);
}

TEST(PluginRegistryInstall, RejectsPluginsForUnknownHostsAndDevices) {
  PluginRegistry registry(ember::test::fcRegistryOptions());
  json unknownHost = manifestFor("odd-host", 7);
  unknownHost["host"] = json::parse(R"(["sb2000"])");
  expectError([&] { static_cast<void>(registry.installPackage(package(unknownHost))); }, api::ERROR_HOST_UNKNOWN, "sb2000");

  json wired = manifestFor("wired", 7);
  wired["wires"] = json::parse(R"([{ "bus": "cpu", "address": "$4200", "device": "fdc", "register": 0, "read": true }])");
  expectError([&] { static_cast<void>(registry.installPackage(package(wired))); }, api::ERROR_DEVICE_UNKNOWN, "fdc");
  json uses = manifestFor("uses-dev", 7);
  uses["uses"] = json::parse(R"(["printer"])");
  expectError([&] { static_cast<void>(registry.installPackage(package(uses))); }, api::ERROR_DEVICE_UNKNOWN, "printer");
  json irq = manifestFor("irq-dev", 7);
  irq["irq"] = json::parse(R"([{ "from": "timer.irq", "to": "cart" }])");
  expectError([&] { static_cast<void>(registry.installPackage(package(irq))); }, api::ERROR_DEVICE_UNKNOWN, "timer");
  EXPECT_TRUE(registry.list().plugins.empty());

  // 主机型号上有这个设备时通过。
  PluginRegistryOptions options = ember::test::fcRegistryOptions();
  options.hosts.hosts["fc"] = {"fdc"};
  PluginRegistry withDevice(options);
  EXPECT_NO_THROW(static_cast<void>(withDevice.installPackage(package(wired))));
}

TEST(PluginRegistryInstall, DevicesMustExistOnEveryExistingHostOfTheList) {
  PluginRegistryOptions options = ember::test::fcRegistryOptions();
  options.hosts.hosts["sb2000"] = {"fdc"};
  PluginRegistry registry(options);
  json manifest = manifestFor("two-hosts", 7);
  manifest["host"] = json::parse(R"(["sb2000", "fc"])");
  manifest["wires"] = json::parse(R"([{ "bus": "cpu", "address": "$4200", "device": "fdc", "register": 0, "read": true }])");
  expectError([&] { static_cast<void>(registry.installPackage(package(manifest))); }, api::ERROR_DEVICE_UNKNOWN, "主机型号 fc");
  manifest["host"] = json::parse(R"(["sb2000"])");
  EXPECT_NO_THROW(static_cast<void>(registry.installPackage(package(manifest))));
}

TEST(PluginRegistryInstall, RejectsBadPackagesAndTriesTheScriptBeforeAccepting) {
  PluginRegistryOptions options = ember::test::fcRegistryOptions();
  options.limits.callTimeout = std::chrono::milliseconds(30);
  PluginRegistry registry(options);
  expectError([&] { static_cast<void>(registry.installPackage(std::vector<std::uint8_t>(64, 1))); }, api::ERROR_INVALID_PACKAGE);

  json bad = manifestFor("bad-api", 7);
  bad["apiVersion"] = 2;
  expectError([&] { static_cast<void>(registry.installPackage(package(bad))); }, api::ERROR_INVALID_PACKAGE, "apiVersion");
  json unknown = manifestFor("unknown-field", 7);
  unknown["surprise"] = true;
  expectError([&] { static_cast<void>(registry.installPackage(package(unknown))); }, api::ERROR_INVALID_PACKAGE, "surprise");

  json withRegisters = manifestFor("needs-write", 7);
  withRegisters["registers"] = json::parse(R"([{ "bus": "cpu", "range": "$8000-$FFFF", "write": true }])");
  expectError([&] { static_cast<void>(registry.installPackage(package(withRegisters, "export default { read() {} };"))); }, api::ERROR_INVALID_PACKAGE, "没有导出 write");
  expectError([&] { static_cast<void>(registry.installPackage(package(withRegisters))); }, api::ERROR_INVALID_PACKAGE, "没有 main.js");
  expectError([&] { static_cast<void>(registry.installPackage(package(withRegisters, "export default {"))); }, api::ERROR_INVALID_PACKAGE, "needs-write");
  expectError([&] { static_cast<void>(registry.installPackage(package(withRegisters, "for (;;) {}\nexport default { write() {} };"))); }, api::ERROR_LIMIT_EXCEEDED, "超过 30 毫秒");
  EXPECT_TRUE(registry.list().plugins.empty());
  EXPECT_NO_THROW(static_cast<void>(registry.installPackage(package(withRegisters, "export default { write() {} };"))));
}

TEST(PluginRegistryInstall, TheHigherVersionWinsAndTheUserWinsATie) {
  const TempDir root;
  const fs::path bundled = root.path() / "bundled";
  const fs::path user = root.path() / "user";
  root.write("bundled/ver-test.emap", package(manifestFor("ver-test", 7, "1.0.0")));
  PluginRegistryOptions options = ember::test::fcRegistryOptions();
  options.bundledDirectory = bundled;
  options.userDirectory = user;
  PluginRegistry registry(options);
  ASSERT_EQ(registry.list().plugins.size(), 1U);
  EXPECT_EQ(registry.list().plugins[0].source, "bundled");

  EXPECT_EQ(registry.installPackage(package(manifestFor("ver-test", 7, "2.0.0"))).version, "2.0.0");
  EXPECT_EQ(registry.list().plugins[0].version, "2.0.0");
  EXPECT_EQ(registry.list().plugins[0].source, "user");
  registry.uninstall("ver-test");
  EXPECT_EQ(registry.list().plugins[0].version, "1.0.0");
  EXPECT_EQ(registry.list().plugins[0].source, "bundled");

  static_cast<void>(registry.installPackage(package(manifestFor("ver-test", 7, "0.9.0"))));
  EXPECT_EQ(registry.list().plugins[0].source, "bundled");  // 预装的版本更高
  static_cast<void>(registry.installPackage(package(manifestFor("ver-test", 7, "1.0.0"))));
  EXPECT_EQ(registry.list().plugins[0].source, "user");  // 版本相同时用户安装的优先
  static_cast<void>(registry.installPackage(package(manifestFor("ver-test", 7, "10.0.0"))));
  EXPECT_EQ(registry.list().plugins[0].version, "10.0.0");  // 版本按数字比较，不按文字
  EXPECT_EQ(registry.list().plugins.size(), 1U);
  EXPECT_EQ(registry.info("ver-test").plugin.summary.version, "10.0.0");
}

TEST(PluginRegistryInstall, SavesTheUserPackageUnderItsIdAndKeepsItAcrossRestarts) {
  const TempDir root;
  PluginRegistryOptions options = ember::test::fcRegistryOptions();
  options.userDirectory = root.path() / "plugins";
  {
    PluginRegistry registry(options);
    static_cast<void>(registry.installPackage(ember::test::officialPluginPackage("nrom")));
    EXPECT_TRUE(fs::is_regular_file(options.userDirectory / "nrom.emap"));
  }
  PluginRegistry restarted(options);
  EXPECT_EQ(idsOf(restarted.list()), std::vector<std::string>{"nrom"});
  EXPECT_EQ(restarted.list().plugins[0].source, "user");
}

TEST(PluginRegistryManage, UninstallRemovesUserPluginsAndRefusesBundledOnes) {
  const TempDir root;
  root.write("bundled/nrom.emap", ember::test::officialPluginPackage("nrom"));
  PluginRegistryOptions options = ember::test::fcRegistryOptions();
  options.bundledDirectory = root.path() / "bundled";
  options.userDirectory = root.path() / "user";
  PluginRegistry registry(options);
  static_cast<void>(registry.installPackage(package(manifestFor("mine", 7))));
  ASSERT_TRUE(fs::exists(options.userDirectory / "mine.emap"));
  registry.uninstall("mine");
  EXPECT_FALSE(fs::exists(options.userDirectory / "mine.emap"));
  EXPECT_EQ(idsOf(registry.list()), std::vector<std::string>{"nrom"});
  expectError([&] { registry.uninstall("nrom"); }, api::ERROR_BUNDLED_READONLY, "只能停用");
  expectError([&] { registry.uninstall("missing"); }, api::ERROR_NOT_FOUND, "missing");
  // 预装的插件可以停用。
  EXPECT_FALSE(registry.setEnabled("nrom", false).plugin.enabled);
  expectError([&] { static_cast<void>(registry.setEnabled("missing", true)); }, api::ERROR_NOT_FOUND);
}

TEST(PluginRegistryManage, TheDisabledStateIsSavedAndForgottenWhenThePluginIsUninstalled) {
  const TempDir root;
  PluginRegistryOptions options = ember::test::fcRegistryOptions();
  options.userDirectory = root.path() / "user";
  {
    PluginRegistry registry(options);
    static_cast<void>(registry.installPackage(package(manifestFor("keeper", 7))));
    static_cast<void>(registry.installPackage(package(manifestFor("dropped", 8))));
    static_cast<void>(registry.setEnabled("keeper", false));
    static_cast<void>(registry.setEnabled("dropped", false));
    registry.uninstall("dropped");
  }
  PluginRegistry restarted(options);
  ASSERT_EQ(restarted.list().plugins.size(), 1U);
  EXPECT_FALSE(restarted.list().plugins[0].enabled);
  // 同标识的插件再次安装时，不受已卸载插件的停用状态影响。
  EXPECT_TRUE(restarted.installPackage(package(manifestFor("dropped", 8))).enabled);
}

TEST(PluginRegistryManage, ReloadReadsTheDirectoriesAgainAndSkipsBadPackagesWithALogEntry) {
  const TempDir root;
  auto errors = std::make_shared<ember::test::RecordingErrorLog>();
  PluginRegistryOptions options = ember::test::fcRegistryOptions();
  options.bundledDirectory = root.path() / "bundled";
  options.userDirectory = root.path() / "user";
  options.errors = errors;
  PluginRegistry registry(options);
  EXPECT_TRUE(registry.list().plugins.empty());

  root.write("bundled/nrom.emap", ember::test::officialPluginPackage("nrom"));
  root.write("bundled/broken.emap", std::vector<std::uint8_t>(80, 7));
  root.write("bundled/notes.txt", std::vector<std::uint8_t>{'h', 'i'});
  root.write("user/UPPER.EMAP", package(manifestFor("upper-case", 7)));
  const auto reloaded = registry.reload();
  EXPECT_EQ(idsOf(reloaded), (std::vector<std::string>{"nrom", "upper-case"}));
  ASSERT_EQ(errors->entries.size(), 1U);
  EXPECT_EQ(errors->entries[0].module, "mappers");
  EXPECT_EQ(errors->entries[0].code, api::ERROR_INVALID_PACKAGE);
  EXPECT_EQ(errors->entries[0].context["file"], "broken.emap");

  fs::remove(options.bundledDirectory / "nrom.emap");
  EXPECT_EQ(idsOf(registry.reload()), std::vector<std::string>{"upper-case"});
}

TEST(PluginRegistryManage, InstalledPluginsStayAfterReloadWhenThereIsNoUserDirectory) {
  PluginRegistry registry(ember::test::fcRegistryOptions());
  static_cast<void>(registry.installPackage(ember::test::officialPluginPackage("nrom")));
  EXPECT_EQ(idsOf(registry.reload()), std::vector<std::string>{"nrom"});
}

TEST(PluginRegistryManage, PublishesAChangedEventForEveryChange) {
  auto events = std::make_shared<ember::test::RecordingEventSink>();
  PluginRegistryOptions options = ember::test::fcRegistryOptions();
  options.events = events;
  PluginRegistry registry(options);
  static_cast<void>(registry.installPackage(package(manifestFor("evented", 7))));
  static_cast<void>(registry.setEnabled("evented", false));
  registry.uninstall("evented");
  static_cast<void>(registry.reload());
  ASSERT_EQ(events->events.size(), 4U);
  for (const auto& event : events->events) EXPECT_EQ(event.first, "mappers.changed");
  EXPECT_EQ(events->events[0].second, json({{"id", "evented"}}));
  EXPECT_EQ(events->events[1].second, json({{"id", "evented"}}));
  EXPECT_EQ(events->events[2].second, json({{"id", "evented"}}));
  EXPECT_EQ(events->events[3].second, json({{"id", nullptr}}));
}

TEST(PluginRegistryManage, InstallFromAFileChecksTheExtensionAndTheSize) {
  const TempDir root;
  PluginRegistry registry(ember::test::fcRegistryOptions());
  const auto text = [](const fs::path& path) { return path.string(); };
  const fs::path good = root.write("good.emap", ember::test::officialPluginPackage("nrom"));
  EXPECT_EQ(registry.install(text(good)).plugin.id, "nrom");

  const fs::path wrongExtension = root.write("plugin.zip", ember::test::officialPluginPackage("nrom"));
  expectError([&] { static_cast<void>(registry.install(text(wrongExtension))); }, api::ERROR_INVALID_PACKAGE, ".emap");
  expectError([&] { static_cast<void>(registry.install(text(root.path() / "missing.emap"))); }, api::ERROR_INVALID_PACKAGE, "missing.emap");
  const fs::path huge = root.write("huge.emap", std::vector<std::uint8_t>(3U * 1024U * 1024U, 0));
  expectError([&] { static_cast<void>(registry.install(text(huge))); }, api::ERROR_INVALID_PACKAGE, "上限");
  const fs::path garbage = root.write("garbage.emap", std::vector<std::uint8_t>(50, 3));
  expectError([&] { static_cast<void>(registry.install(text(garbage))); }, api::ERROR_INVALID_PACKAGE, "zip");
}

TEST(PluginRegistryCreate, CreatesACartridgeFromTheMatchedPluginUntilItIsRemovedOrDisabled) {
  PluginRegistry registry(ember::test::fcRegistryOptions());
  static_cast<void>(registry.installPackage(ember::test::officialPluginPackage("nrom")));
  api::CartridgeImage image;
  image.description = described(0);
  image.prg.assign(16384, 0x5A);
  ember::test::RecordingIrq irq;
  const auto match = registry.match(image.description);
  const auto cartridge = registry.create(match, image, irq);
  EXPECT_EQ(cartridge->pluginId(), "nrom");
  EXPECT_EQ(cartridge->cpuSide().read(0x8000).value, 0x5A);

  static_cast<void>(registry.setEnabled("nrom", false));
  expectError([&] { static_cast<void>(registry.create(match, image, irq)); }, api::ERROR_NOT_FOUND, "nrom");
  static_cast<void>(registry.setEnabled("nrom", true));
  EXPECT_NE(registry.create(match, image, irq), nullptr);
  registry.uninstall("nrom");
  expectError([&] { static_cast<void>(registry.create(match, image, irq)); }, api::ERROR_NOT_FOUND);
  // 已创建的卡带不受卸载影响：它持有插件。
  EXPECT_EQ(cartridge->cpuSide().read(0xFFFF).value, 0x5A);
}

TEST(PluginRegistryConcurrency, ListMatchInstallAndCreateCanRunTogether) {
  PluginRegistry registry(ember::test::fcRegistryOptions());
  static_cast<void>(registry.installPackage(ember::test::officialPluginPackage("nrom")));
  const auto extra = package(manifestFor("churn", 9));
  std::atomic<bool> stop{false};
  std::atomic<int> failures{0};
  std::vector<std::thread> readers;
  for (int index = 0; index < 3; ++index) {
    readers.emplace_back([&] {
      api::CartridgeImage image;
      image.description = described(0);
      image.prg.assign(16384, 1);
      ember::test::RecordingIrq irq;
      while (!stop) {
        try {
          static_cast<void>(registry.list());
          const auto match = registry.match(image.description);
          static_cast<void>(registry.create(match, image, irq));
        } catch (...) {
          ++failures;
        }
      }
    });
  }
  for (int round = 0; round < 40; ++round) {
    static_cast<void>(registry.installPackage(extra));
    registry.uninstall("churn");
  }
  stop = true;
  for (auto& reader : readers) reader.join();
  EXPECT_EQ(failures.load(), 0);
}

}  // namespace
