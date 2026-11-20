/*
 * ------------------------------------------------------------------------
 * 名称：mappers-logic-test.cpp
 * 说明：mapper 插件模块逻辑的测试：纯规则（摘要与详情的读取、匹配条件与主机型号的文字、搜索与来源筛选、统计、清单摘要）、控制器（读取、选中与详情、搜索与筛选、安装、卸载先询问、启用与停用、事件、核心重启、断开）与模块的登记项。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <algorithm>

#include "logic/mappers/mappers-controller.hpp"
#include "logic/mappers/mappers-module.hpp"
#include "logic/mappers/mappers-rules.hpp"
#include "support/fakes.hpp"

namespace ember::win32 {
namespace {

using contracts::CoreReply;
using contracts::MapperDetailLoad;
using contracts::MappersLoad;
using nlohmann::json;

json summary(const std::string& id, const std::string& name, const std::string& source, bool enabled, const json& match, const std::string& publisher = "Ember", const std::string& description = "") {
  json item = {{"id", id}, {"name", name}, {"version", "1.0.0"}, {"publisher", publisher}, {"host", json::array({"fc"})}, {"devices", json::array()}, {"match", match}, {"source", source}, {"enabled", enabled}};
  if (!description.empty()) item["description"] = description;
  return item;
}

json plugins() {
  return {{"plugins", json::array({summary("ember.mmc3", "MMC3", "bundled", true, json::array({{{"mapper", 4}}, {{"mapper", 118}}}), "Ember", "任天堂 MMC3"),
                                   summary("ember.nrom", "NROM", "bundled", false, json::array({{{"mapper", 0}}})),
                                   summary("user.cool", "Cool Mapper", "user", true, json::array({{{"mapper", 4}, {"variant", 1}}}), "某某", "自制")})}};
}

json detail() {
  return {{"plugin", {{"summary", summary("ember.mmc3", "MMC3", "bundled", true, json::array({{{"mapper", 4}, {"prgSize", 262144}, {"crc32Prg", "abcd1234"}}}))},
                      {"storage", json::array({{{"id", "prg"}, {"kind", "rom"}, {"from", "file.prg"}, {"battery", false}}, {{"id", "wram"}, {"kind", "ram"}, {"size", 8192}, {"battery", true}}})},
                      {"windows", json::array()},
                      {"nametable", "h"},
                      {"registers", json::array({{{"bus", "cpu"}, {"range", "$8000-$FFFF"}, {"read", false}, {"write", true}}})},
                      {"state", json::array({{{"name", "irq"}, {"bits", 8}, {"description", "计数器"}}})},
                      {"settings", json::array()}}}};
}

logic::MapperSummary parsed(const json& value) { return *logic::parseMapperSummary(value); }

TEST(MappersRules, ParsesSummariesAndSkipsBrokenOnes) {
  const auto list = logic::parseMapperList(plugins());
  ASSERT_TRUE(list.has_value());
  ASSERT_EQ(list->size(), 3U);
  EXPECT_EQ((*list)[2].id, "user.cool");
  EXPECT_EQ((*list)[2].match[0].variant, 1);
  auto broken = plugins();
  broken["plugins"].push_back(5);
  broken["plugins"].push_back({{"name", "no id"}});
  EXPECT_EQ(logic::parseMapperList(broken)->size(), 3U);
  EXPECT_FALSE(logic::parseMapperList(json::object()).has_value());
  EXPECT_FALSE(logic::parseMapperDetail(json::object()).has_value());
}

TEST(MappersRules, RuleTextListsMapperNumberAndWrittenConditions) {
  EXPECT_EQ(logic::ruleText({4, {}, {}, {}, {}, {}}), "mapper 4");
  EXPECT_EQ(logic::ruleText({4, 1, 262144, 131072, std::string("abcd1234"), std::string("0123abcd")}), "mapper 4，变体 1，PRG 256 KB，CHR 128 KB，PRG 校验 ABCD1234，整体校验 0123ABCD");
  EXPECT_EQ(logic::ruleText({4, {}, 512, {}, {}, {}}), "mapper 4，PRG 0.5 KB");
}

TEST(MappersRules, HostsAndSourcesUseDisplayNames) {
  EXPECT_EQ(logic::hostsText({"fc", "sb2000", "bbk10", "bbk20", "sb97", "other"}), "经典 FC、小霸王 2000、步步高软驱一号、步步高 98、小霸王 97、other");
  EXPECT_EQ(logic::sourceLabel("bundled"), "预装");
  EXPECT_EQ(logic::sourceLabel("user"), "用户安装");
  EXPECT_EQ(logic::matchesText({{4, {}, {}, {}, {}, {}}, {118, {}, {}, {}, {}, {}}}), "mapper 4；mapper 118");
}

TEST(MappersRules, SearchIgnoresCaseAndAllWhitespaceAndCoversNameIdPublisherDescription) {
  const auto list = *logic::parseMapperList(plugins());
  const auto ids = [](const std::vector<logic::MapperSummary>& items) {
    std::vector<std::string> result;
    for (const auto& item : items) result.push_back(item.id);
    return result;
  };
  EXPECT_EQ(ids(logic::filterMappers(list, "all", "")), (std::vector<std::string>{"ember.mmc3", "ember.nrom", "user.cool"}));
  EXPECT_EQ(ids(logic::filterMappers(list, "all", "mm c 3")), (std::vector<std::string>{"ember.mmc3"}));
  EXPECT_EQ(ids(logic::filterMappers(list, "all", "COOL")), (std::vector<std::string>{"user.cool"}));
  EXPECT_EQ(ids(logic::filterMappers(list, "all", "某某")), (std::vector<std::string>{"user.cool"}));
  EXPECT_EQ(ids(logic::filterMappers(list, "all", "任天堂　mmc3")), (std::vector<std::string>{"ember.mmc3"}));
  EXPECT_EQ(ids(logic::filterMappers(list, "bundled", "")), (std::vector<std::string>{"ember.mmc3", "ember.nrom"}));
  EXPECT_EQ(ids(logic::filterMappers(list, "user", "ember")), (std::vector<std::string>{}));
}

TEST(MappersRules, StatsCountPluginsEnabledAndDistinctMapperNumbersRegardlessOfEnabled) {
  const auto list = *logic::parseMapperList(plugins());
  // 4、118、0 三种 mapper（user.cool 的 4 与 MMC3 的重复）
  EXPECT_EQ(logic::statsText(list), "共 3 个插件（已启用 2 个），支持 3 种 mapper");
}

TEST(MappersRules, DetailFactsOmitEmptyOptionalItemsAndWriteNoneForEmptyLists) {
  const auto parsedDetail = logic::parseMapperDetail(detail());
  ASSERT_TRUE(parsedDetail.has_value());
  const auto facts = logic::detailFacts(*parsedDetail);
  std::vector<std::string> labels;
  for (const auto& fact : facts) labels.push_back(fact.label);
  // 没有说明、没有外设：不列
  EXPECT_EQ(labels, (std::vector<std::string>{"标识", "版本", "发布者", "匹配", "主机型号", "存储区", "寄存器范围", "状态声明"}));
  EXPECT_EQ(facts[3].lines, (std::vector<std::string>{"mapper 4，PRG 256 KB，PRG 校验 ABCD1234"}));
  EXPECT_EQ(facts[5].lines, (std::vector<std::string>{"prg：只读（来自 PRG）", "wram：可写 8192 字节，带电池"}));
  EXPECT_EQ(facts[6].lines, (std::vector<std::string>{"CPU $8000-$FFFF（写）"}));
  EXPECT_EQ(facts[7].lines, (std::vector<std::string>{"irq（8 位）：计数器"}));

  logic::MapperDetail empty = *parsedDetail;
  empty.storage.clear();
  empty.registers.clear();
  empty.state.clear();
  empty.summary.description = "说明";
  empty.summary.devices = {"mouse"};
  const auto emptyFacts = logic::detailFacts(empty);
  EXPECT_EQ(emptyFacts[3].label, "说明");
  EXPECT_EQ(emptyFacts[6].label, "外设");
  EXPECT_EQ(emptyFacts[6].lines, (std::vector<std::string>{"鼠标"}));
  EXPECT_EQ(emptyFacts[7].lines, (std::vector<std::string>{"无"}));
  EXPECT_EQ(emptyFacts[8].lines, (std::vector<std::string>{"无"}));
  EXPECT_EQ(emptyFacts[9].lines, (std::vector<std::string>{"无"}));
}

struct Fixture {
  test::FakeCoreGateway core;
  test::FakeBroker broker{core};
  test::FakeShellApi shell;
  test::FakeConfirmation confirmation;
  logic::MappersController controller{broker, core, confirmation};
  std::function<void()> detach;
  std::function<void()> unsubscribe;
  int changes{0};

  Fixture() {
    reply("mappers.list", plugins());
    reply("mappers.info", detail());
    unsubscribe = controller.subscribe([this] { ++changes; });
  }

  void attach() { detach = controller.attach(shell); }
  void reply(const std::string& name, const json& result) {
    core.replies[name] = [result](const json&) { return CoreReply::success(result); };
  }
  void fail(const std::string& name, const std::string& message) {
    core.replies[name] = [message](const json&) { return CoreReply::failure("mappers.notFound", message); };
  }
  [[nodiscard]] const test::FakeCoreGateway::Call* last(const std::string& name) const {
    for (auto it = core.calls.rbegin(); it != core.calls.rend(); ++it) {
      if (it->name == name) return &*it;
    }
    return nullptr;
  }
};

TEST(MappersController, LoadsOnAttachAndShowsRowsAndStats) {
  Fixture f;
  EXPECT_EQ(f.controller.view().load, MappersLoad::Loading);
  f.attach();
  const auto view = f.controller.view();
  EXPECT_EQ(view.load, MappersLoad::Ready);
  ASSERT_EQ(view.rows.size(), 3U);
  EXPECT_EQ(view.rows[0].name, "MMC3");
  EXPECT_EQ(view.rows[0].match, "mapper 4；mapper 118");
  EXPECT_EQ(view.rows[0].source, "预装");
  EXPECT_TRUE(view.rows[0].bundled);
  EXPECT_TRUE(view.rows[1].bundled);
  EXPECT_FALSE(view.rows[2].bundled);
  EXPECT_EQ(view.stats, "共 3 个插件（已启用 2 个），支持 3 种 mapper");
}

TEST(MappersController, FailedListShowsTheCoreMessage) {
  Fixture f;
  f.fail("mappers.list", "读取失败了");
  f.attach();
  EXPECT_EQ(f.controller.view().load, MappersLoad::Failed);
  EXPECT_EQ(f.controller.view().message, "读取失败了");
  EXPECT_TRUE(f.controller.view().rows.empty());
  EXPECT_TRUE(f.controller.view().stats.empty());
}

TEST(MappersController, SearchAndSourceFilterTheRowsButNotTheStats) {
  Fixture f;
  f.attach();
  f.controller.setSearch("cool");
  auto view = f.controller.view();
  ASSERT_EQ(view.rows.size(), 1U);
  EXPECT_EQ(view.rows[0].id, "user.cool");
  EXPECT_EQ(view.stats, "共 3 个插件（已启用 2 个），支持 3 种 mapper");
  f.controller.setSearch("");
  f.controller.setSource("bundled");
  EXPECT_EQ(f.controller.view().rows.size(), 2U);
  EXPECT_EQ(f.controller.view().source, "bundled");
}

TEST(MappersController, SelectingReadsTheDetailAndSelectingAgainClearsIt) {
  Fixture f;
  f.attach();
  f.controller.toggleSelect("ember.mmc3");
  EXPECT_EQ(f.last("mappers.info")->params, (json{{"id", "ember.mmc3"}}));
  auto view = f.controller.view();
  EXPECT_EQ(view.selectedId, "ember.mmc3");
  EXPECT_EQ(view.detailLoad, MapperDetailLoad::Ready);
  EXPECT_FALSE(view.facts.empty());
  f.controller.toggleSelect("ember.mmc3");
  view = f.controller.view();
  EXPECT_TRUE(view.selectedId.empty());
  EXPECT_EQ(view.detailLoad, MapperDetailLoad::None);
}

TEST(MappersController, SelectedPluginStaysVisibleInDetailWhenFilteredOut) {
  Fixture f;
  f.attach();
  f.controller.toggleSelect("ember.mmc3");
  f.controller.setSearch("cool");
  const auto view = f.controller.view();
  EXPECT_EQ(view.rows.size(), 1U);
  EXPECT_EQ(view.selectedId, "ember.mmc3");
  EXPECT_EQ(view.selected.name, "MMC3");
}

TEST(MappersController, DetailFailureIsShownWithTheMessage) {
  Fixture f;
  f.attach();
  f.fail("mappers.info", "找不到插件");
  f.controller.toggleSelect("ember.mmc3");
  const auto view = f.controller.view();
  EXPECT_EQ(view.detailLoad, MapperDetailLoad::Failed);
  EXPECT_EQ(view.detailMessage, "找不到插件");
}

TEST(MappersController, InstallAddsAndSelectsThePluginAndNotifies) {
  Fixture f;
  f.attach();
  f.reply("mappers.openPackage", {{"canceled", false}, {"plugin", summary("user.new", "New", "user", true, json::array({{{"mapper", 5}}}))}});
  f.controller.install();
  const auto view = f.controller.view();
  ASSERT_EQ(view.rows.size(), 4U);
  EXPECT_EQ(view.selectedId, "user.new");
  ASSERT_EQ(f.shell.notices.size(), 1U);
  EXPECT_EQ(f.shell.notices[0].first, "已安装插件：New 1.0.0");
  EXPECT_EQ(f.shell.notices[0].second, contracts::NotificationLevel::Success);
}

TEST(MappersController, InstallReplacesPluginWithTheSameIdCancelIsSilentAndFailureIsNotified) {
  Fixture f;
  f.attach();
  f.reply("mappers.openPackage", {{"canceled", false}, {"plugin", summary("user.cool", "Cool 2", "user", true, json::array({{{"mapper", 4}}}))}});
  f.controller.install();
  EXPECT_EQ(f.controller.view().rows.size(), 3U);
  EXPECT_EQ(f.controller.view().rows[2].name, "Cool 2");
  f.reply("mappers.openPackage", {{"canceled", true}});
  const std::size_t notices = f.shell.notices.size();
  f.controller.install();
  EXPECT_EQ(f.shell.notices.size(), notices);
  f.fail("mappers.openPackage", "插件包不合格");
  f.controller.install();
  ASSERT_EQ(f.shell.notices.size(), notices + 1);
  EXPECT_EQ(f.shell.notices.back().first, "插件包不合格");
  EXPECT_EQ(f.shell.notices.back().second, contracts::NotificationLevel::Error);
}

TEST(MappersController, UninstallAsksFirstThenRemovesAndNotifies) {
  Fixture f;
  f.attach();
  f.controller.toggleSelect("user.cool");
  f.confirmation.answer = false;
  f.controller.uninstall("user.cool");
  ASSERT_EQ(f.confirmation.asked.size(), 1U);
  EXPECT_EQ(f.confirmation.asked[0], "卸载“Cool Mapper”？卸载后使用它的卡带将不能运行，可以重新安装插件包。");
  EXPECT_EQ(f.last("mappers.uninstall"), nullptr);
  EXPECT_EQ(f.controller.view().rows.size(), 3U);

  f.confirmation.answer = true;
  f.controller.uninstall("user.cool");
  EXPECT_EQ(f.last("mappers.uninstall")->params, (json{{"id", "user.cool"}}));
  const auto view = f.controller.view();
  EXPECT_EQ(view.rows.size(), 2U);
  EXPECT_TRUE(view.selectedId.empty());
  EXPECT_EQ(f.shell.notices.back().first, "已卸载插件：Cool Mapper");
}

TEST(MappersController, UninstallFailureIsNotifiedAndKeepsThePlugin) {
  Fixture f;
  f.attach();
  f.fail("mappers.uninstall", "预装插件不能卸载");
  f.controller.uninstall("user.cool");
  EXPECT_EQ(f.controller.view().rows.size(), 3U);
  ASSERT_EQ(f.shell.notices.size(), 1U);
  EXPECT_EQ(f.shell.notices[0].first, "预装插件不能卸载");
  EXPECT_EQ(f.shell.notices[0].second, contracts::NotificationLevel::Error);
}

TEST(MappersController, SetEnabledUsesTheSummaryTheCoreReturnsAndFailureIsNotified) {
  Fixture f;
  f.attach();
  f.reply("mappers.setEnabled", {{"plugin", summary("ember.nrom", "NROM", "bundled", true, json::array({{{"mapper", 0}}}))}});
  f.controller.setEnabled("ember.nrom", true);
  EXPECT_EQ(f.last("mappers.setEnabled")->params, (json{{"id", "ember.nrom"}, {"enabled", true}}));
  EXPECT_TRUE(f.controller.view().rows[1].enabled);
  EXPECT_TRUE(f.shell.notices.empty());
  f.fail("mappers.setEnabled", "找不到插件");
  f.controller.setEnabled("ember.nrom", false);
  EXPECT_TRUE(f.controller.view().rows[1].enabled);
  ASSERT_EQ(f.shell.notices.size(), 1U);
}

TEST(MappersController, ChangedEventReloadsTheListAndTheSelectedDetailAndCoreRestartClears) {
  Fixture f;
  f.attach();
  f.controller.toggleSelect("ember.mmc3");
  const std::size_t infoCalls = std::count_if(f.core.calls.begin(), f.core.calls.end(), [](const auto& call) { return call.name == "mappers.info"; });
  f.reply("mappers.list", {{"plugins", json::array({summary("ember.mmc3", "MMC3", "bundled", true, json::array({{{"mapper", 4}}}))})}});
  f.core.emitEvent("mappers.changed", {{"id", nullptr}});
  EXPECT_EQ(f.controller.view().rows.size(), 1U);
  EXPECT_EQ(std::count_if(f.core.calls.begin(), f.core.calls.end(), [](const auto& call) { return call.name == "mappers.info"; }), static_cast<std::ptrdiff_t>(infoCalls + 1));
  f.core.setState(contracts::CoreState::Starting);
  EXPECT_EQ(f.controller.view().load, MappersLoad::Loading);
  EXPECT_TRUE(f.controller.view().rows.empty());
  f.detach();
  EXPECT_EQ(f.core.subscriberCount(), 0U);
}

TEST(MappersController, SelectedPluginThatNoLongerExistsIsDropped) {
  Fixture f;
  f.attach();
  f.controller.toggleSelect("user.cool");
  f.reply("mappers.list", {{"plugins", json::array({summary("ember.mmc3", "MMC3", "bundled", true, json::array({{{"mapper", 4}}}))})}});
  f.core.emitEvent("mappers.changed", {{"id", "user.cool"}});
  EXPECT_TRUE(f.controller.view().selectedId.empty());
}

TEST(MappersModule, RegistersTheDialogTheCommandAndTheSettingsMenuItemAndAttaches) {
  test::FakeCoreGateway core;
  test::FakeBroker broker{core};
  test::FakeShellApi shell;
  test::FakeConfirmation confirmation;
  logic::MappersController controller(broker, core, confirmation);
  const auto module = logic::createMappersModule(controller);
  EXPECT_EQ(module.name, "mappers");
  ASSERT_EQ(module.dialogs.size(), 1U);
  EXPECT_EQ(module.dialogs[0].id, "mappers.plugins");
  EXPECT_EQ(module.dialogs[0].title, "mapper 插件");
  EXPECT_EQ(module.dialogs[0].width, 0);
  EXPECT_EQ(module.dialogs[0].height, 560);
  EXPECT_TRUE(module.dialogs[0].fitMinWidth);
  EXPECT_FALSE(module.dialogs[0].fitMinHeight);
  EXPECT_EQ(module.dialogs[0].minHeight, 360);
  ASSERT_EQ(module.commands.size(), 1U);
  EXPECT_EQ(module.commands[0].id, "mappers.open");
  EXPECT_TRUE(module.commands[0].defaultShortcut.empty());
  module.commands[0].run(shell);
  EXPECT_EQ(shell.dialogs, (std::vector<std::string>{"mappers.plugins"}));
  ASSERT_EQ(module.menuItems.size(), 1U);
  EXPECT_EQ(module.menuItems[0].menu, "settings");
  EXPECT_FALSE(module.menuItems[0].separatorBefore);
  const auto detach = module.attach(shell);
  EXPECT_EQ(core.subscriberCount(), 2U);
  detach();
  EXPECT_EQ(core.subscriberCount(), 0U);
}

}  // namespace
}  // namespace ember::win32
