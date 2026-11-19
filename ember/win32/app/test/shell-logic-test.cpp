/*
 * ------------------------------------------------------------------------
 * 名称：shell-logic-test.cpp
 * 说明：外壳逻辑的测试：快捷键的写法与检查、登记表的汇总与检查、外壳控制器（命令、菜单、面板、自动收缩、状态项、拖入文件）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：行为规则见 docs/05-界面.md 第 2、5、6 节。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include "logic/shell/shell-controller.hpp"
#include "logic/shell/shell-module.hpp"
#include "logic/shell/shell-registry-builder.hpp"
#include "shared/shortcut/shortcut.hpp"
#include "support/fakes.hpp"

namespace ember::win32 {
namespace {

using contracts::ShellApi;
using contracts::ShellCommandDefinition;
using contracts::ShellModule;

const std::vector<char> kMnemonics = {'F', 'M', 'S', 'H'};

ShellCommandDefinition command(const std::string& id, const std::string& shortcut = "", std::function<void(ShellApi&)> run = {}) {
  ShellCommandDefinition result;
  result.id = id;
  result.title = [id] { return id; };
  result.defaultShortcut = shortcut;
  result.run = run ? std::move(run) : [](ShellApi&) {};
  return result;
}

contracts::ShellMenuItemDefinition menuItem(const std::string& menu, const std::string& commandId, bool separator = false) {
  contracts::ShellMenuItemDefinition item;
  item.menu = menu;
  item.command = commandId;
  item.separatorBefore = separator;
  return item;
}

std::string registryError(std::vector<ShellModule> modules) {
  try {
    (void)logic::buildShellRegistry(std::move(modules));
  } catch (const std::runtime_error& error) {
    return error.what();
  }
  return {};
}

TEST(Shortcut, WritesModifiersInCtrlShiftAltOrder) {
  EXPECT_EQ(shared::shortcutFromKey('K', shared::kShortcutAlt | shared::kShortcutCtrl | shared::kShortcutShift), "Ctrl+Shift+Alt+K");
  EXPECT_EQ(shared::shortcutFromKey(0x78, 0), "F9");
  EXPECT_EQ(shared::shortcutFromKey(0xbc, shared::kShortcutCtrl), "Ctrl+,");
  EXPECT_EQ(shared::shortcutFromKey(0x1b, 0), "Esc");
  EXPECT_EQ(shared::shortcutFromKey(0x26, 0), "Up");
  EXPECT_FALSE(shared::shortcutFromKey(0x10, 0).has_value());
  EXPECT_FALSE(shared::shortcutFromKey(0x5b, 0).has_value());
}

TEST(Shortcut, AcceptsTheDefaultShortcutsOfEmber) {
  for (const char* shortcut : {"Ctrl+O", "F9", "F8", "F7", "Ctrl+F7", "F11", "Ctrl+,", "Alt+Z", "Shift+F7"}) EXPECT_FALSE(shared::checkDefaultShortcut(shortcut, kMnemonics).has_value()) << shortcut;
}

TEST(Shortcut, RejectsBadWritingReservedMnemonicAndMissingModifier) {
  EXPECT_TRUE(shared::checkDefaultShortcut("ctrl+o", kMnemonics).has_value());
  EXPECT_TRUE(shared::checkDefaultShortcut("Shift+Ctrl+O", kMnemonics).has_value());
  EXPECT_TRUE(shared::checkDefaultShortcut("Ctrl+", kMnemonics).has_value());
  EXPECT_TRUE(shared::checkDefaultShortcut("F25", kMnemonics).has_value());
  EXPECT_TRUE(shared::checkDefaultShortcut("Ctrl+C", kMnemonics).has_value());
  EXPECT_TRUE(shared::checkDefaultShortcut("Alt+F4", kMnemonics).has_value());
  EXPECT_TRUE(shared::checkDefaultShortcut("Alt+F", kMnemonics).has_value());
  EXPECT_TRUE(shared::checkDefaultShortcut("K", kMnemonics).has_value());
  EXPECT_TRUE(shared::checkDefaultShortcut("Shift+K", kMnemonics).has_value());
}

TEST(ShellRegistry, CollectsEverythingAndKeepsTheShellsMenuItemsLast) {
  ShellModule runtime;
  runtime.name = "runtime";
  runtime.hasScreen = true;
  runtime.commands.push_back(command("runtime.open", "Ctrl+O"));
  runtime.menuItems.push_back(menuItem("file", "runtime.open"));
  const auto registry = logic::buildShellRegistry({logic::createShellModule(), runtime});
  EXPECT_EQ(registry.screenOwner, "runtime");
  EXPECT_EQ(registry.commands.size(), 4U);
  ASSERT_EQ(registry.menuItems.size(), 3U);
  EXPECT_EQ(registry.menuItems[0].command, "runtime.open");
  EXPECT_EQ(registry.menuItems[1].command, "shell.toggleAlwaysOnTop");
  EXPECT_EQ(registry.shortcuts.at("F11"), "shell.toggleFullScreen");
}

TEST(ShellRegistry, ReportsEveryProblemAtOnce) {
  ShellModule bad;
  bad.name = "Bad";
  ShellModule one;
  one.name = "one";
  one.hasScreen = true;
  one.commands.push_back(command("two.open"));
  one.commands.push_back(command("one.a", "F9"));
  one.commands.push_back(command("one.a"));
  one.commands.push_back(command("one.b", "F9"));
  one.commands.push_back(command("one.c", "Ctrl+C"));
  one.menuItems.push_back(menuItem("file", "one.missing"));
  one.menuItems.push_back(menuItem("nowhere", "one.a"));
  ShellModule two;
  two.name = "two";
  two.hasScreen = true;
  two.menuItems.push_back(menuItem("file", "one.a"));
  const std::string error = registryError({bad, one, two});
  EXPECT_NE(error.find("模块名“Bad”不合格或重复"), std::string::npos);
  EXPECT_NE(error.find("命令“two.open”的标识必须以“one.”开头"), std::string::npos);
  EXPECT_NE(error.find("命令“one.a”重复登记"), std::string::npos);
  EXPECT_NE(error.find("显示区只能有一个"), std::string::npos);
  EXPECT_NE(error.find("菜单项引用的命令不存在：one.missing"), std::string::npos);
  EXPECT_NE(error.find("放在不存在的菜单里：nowhere"), std::string::npos);
  EXPECT_NE(error.find("菜单项只能使用本模块的命令：one.a"), std::string::npos);
  EXPECT_NE(error.find("默认快捷键 F9 重复"), std::string::npos);
  EXPECT_NE(error.find("默认快捷键 Ctrl+C 不可用"), std::string::npos);
}

TEST(ShellRegistry, FileHandlersNeedValidAndUniqueExtensions) {
  ShellModule a;
  a.name = "a";
  a.fileHandlers.push_back({"a.nes", {"nes"}, [](const contracts::DroppedFile&) {}});
  a.fileHandlers.push_back({"a.bad", {".img"}, [](const contracts::DroppedFile&) {}});
  a.fileHandlers.push_back({"a.none", {}, [](const contracts::DroppedFile&) {}});
  ShellModule b;
  b.name = "b";
  b.fileHandlers.push_back({"b.nes", {"nes", "bin"}, [](const contracts::DroppedFile&) {}});
  const std::string error = registryError({a, b});
  EXPECT_NE(error.find("文件处理项“a.bad”的扩展名必须是小写字母与数字"), std::string::npos);
  EXPECT_NE(error.find("文件处理项“a.none”的扩展名必须是小写字母与数字"), std::string::npos);
  EXPECT_NE(error.find("“b.nes”与“a.nes”都认领了扩展名 nes"), std::string::npos);
}

TEST(ShellRegistry, SubmenusAndTheirIdsFollowTheSameRules) {
  ShellModule m;
  m.name = "m";
  contracts::ShellMenuItemDefinition item;
  item.menu = "file";
  item.submenu = contracts::ShellSubmenuDefinition{"other.recent", "最近打开", [] { return std::vector<contracts::ShellSubmenuEntry>{}; }};
  m.menuItems.push_back(item);
  EXPECT_NE(registryError({m}).find("子菜单“other.recent”的标识必须以“m.”开头"), std::string::npos);
}

/** 带外壳自己的登记项与一个示例模块的控制器。 */
struct Fixture {
  test::FakeShellState state;
  test::FakeDroppedFiles dropped;
  test::FakeShellWindow window;
  test::FakeShellDialogs dialogs;
  test::FakeNotifier notifier;
  int runs{0};
  int opened{0};
  std::string lastDropped;
  bool powered{false};
  std::unique_ptr<logic::ShellController> controller;

  explicit Fixture(std::vector<std::string> autoCollapseOff = {}) {
    state.state.layout.autoCollapseOffPanels = std::move(autoCollapseOff);
    ShellModule sample;
    sample.name = "sample";
    sample.dialogs.push_back({"sample.settings", "设置", 400, 300, 0, 0});
    ShellCommandDefinition power = command("sample.power", "F9", [this](ShellApi&) { ++runs; });
    power.title = [this] { return powered ? std::string("断电") : std::string("开机"); };
    power.enabled = [](const contracts::ShellViewState& view) { return !view.fullScreen; };
    sample.commands.push_back(power);
    sample.commands.push_back(command("sample.ctrl", "Ctrl+O", [this](ShellApi&) { ++runs; }));
    sample.menuItems.push_back(menuItem("machine", "sample.power"));
    sample.menuItems.push_back(menuItem("machine", "sample.ctrl", true));
    contracts::ShellMenuItemDefinition recent;
    recent.menu = "file";
    recent.submenu = contracts::ShellSubmenuDefinition{"sample.recent", "最近打开", [this] {
      std::vector<contracts::ShellSubmenuEntry> entries;
      contracts::ShellSubmenuEntry entry;
      entry.id = "1";
      entry.label = "game.nes";
      entry.run = [this](ShellApi&) { ++opened; };
      entries.push_back(entry);
      return entries;
    }};
    sample.menuItems.push_back(recent);
    contracts::ShellMenuItemDefinition empty;
    empty.menu = "file";
    empty.submenu = contracts::ShellSubmenuDefinition{"sample.empty", "没有的", [] { return std::vector<contracts::ShellSubmenuEntry>{}; }};
    sample.menuItems.push_back(empty);
    contracts::ShellPanelDefinition panel;
    panel.id = "sample.panel";
    panel.title = "运行";
    panel.autoCollapse = true;
    panel.look = [this] {
      contracts::ShellPanelLook look;
      look.icon = powered ? contracts::ShellIcon::PlayerPause : contracts::ShellIcon::PlayerPlay;
      look.hint = powered ? "运行：运行中" : "";
      return look;
    };
    sample.bottomPanels.push_back(panel);
    contracts::ShellPanelDefinition plain;
    plain.id = "sample.plain";
    plain.title = "普通";
    sample.bottomPanels.push_back(plain);
    contracts::ShellStatusItemDefinition host;
    host.id = "sample.host";
    host.view = [this]() -> std::optional<contracts::ShellStatusItemView> {
      if (!powered) return std::nullopt;
      return contracts::ShellStatusItemView{"经典 FC", "主机型号", contracts::ShellTone::Normal};
    };
    sample.statusItems.push_back(host);
    sample.fileHandlers.push_back({"sample.cartridge", {"nes"}, [this](const contracts::DroppedFile& file) { lastDropped = file.name; }});
    sample.attach = [this](ShellApi&) { return std::function<void()>([this] { ++detached; }); };
    controller = std::make_unique<logic::ShellController>(logic::buildShellRegistry({logic::createShellModule(), sample}), window, dialogs, notifier, state, dropped);
  }

  int detached{0};
};

TEST(ShellController, MenusShowNamesShortcutsStatesAndHideEmptyMenus) {
  Fixture f;
  const auto menus = f.controller->menus();
  // 帮助菜单没有项，不出现
  ASSERT_EQ(menus.size(), 3U);
  EXPECT_EQ(menus[0].id, "file");
  EXPECT_EQ(menus[1].id, "machine");
  EXPECT_EQ(menus[2].id, "settings");
  ASSERT_EQ(menus[1].items.size(), 2U);
  EXPECT_EQ(menus[1].items[0].label, "开机");
  EXPECT_EQ(menus[1].items[0].shortcut, "F9");
  EXPECT_FALSE(menus[1].items[0].separatorBefore);
  EXPECT_TRUE(menus[1].items[1].separatorBefore);
  // 文件菜单：最近打开子菜单、置灰的空子菜单、退出（前面有分隔线）
  ASSERT_EQ(menus[0].items.size(), 3U);
  EXPECT_EQ(menus[0].items[0].items.size(), 1U);
  EXPECT_EQ(menus[0].items[0].items[0].id, "sample.recent.1");
  EXPECT_TRUE(menus[0].items[1].disabled);
  EXPECT_TRUE(menus[0].items[1].items.empty());
  EXPECT_EQ(menus[0].items[2].id, "shell.quit");
  EXPECT_TRUE(menus[0].items[2].separatorBefore);
  // 设置菜单的“窗口始终置顶”是勾选项，菜单里第一项不画分隔线
  ASSERT_EQ(menus[2].items.size(), 1U);
  EXPECT_FALSE(menus[2].items[0].separatorBefore);
  ASSERT_TRUE(menus[2].items[0].checked.has_value());
  EXPECT_FALSE(*menus[2].items[0].checked);
  f.powered = true;
  EXPECT_EQ(f.controller->menus()[1].items[0].label, "断电");
}

TEST(ShellController, SelectingMenuItemsRunsCommandsAndSubmenuEntries) {
  Fixture f;
  f.controller->selectMenuItem("sample.power");
  EXPECT_EQ(f.runs, 1);
  f.controller->selectMenuItem("sample.recent.1");
  EXPECT_EQ(f.opened, 1);
  f.controller->selectMenuItem("sample.recent.nope");
  EXPECT_EQ(f.opened, 1);
  f.controller->selectMenuItem("shell.quit");
  EXPECT_EQ(f.window.quits, 1);
}

TEST(ShellController, DisabledCommandsDoNotRunButTheirShortcutIsStillClaimed) {
  Fixture f;
  f.controller->setWindowState(true, false);
  const auto menus = f.controller->menus();
  EXPECT_TRUE(menus[1].items[0].disabled);
  f.controller->selectMenuItem("sample.power");
  EXPECT_TRUE(f.controller->runShortcut("F9", false));
  EXPECT_EQ(f.runs, 0);
  f.controller->setWindowState(false, false);
  EXPECT_TRUE(f.controller->runShortcut("F9", false));
  EXPECT_EQ(f.runs, 1);
}

TEST(ShellController, ShortcutsWithoutCtrlOrAltAreLeftToTextControls) {
  Fixture f;
  EXPECT_FALSE(f.controller->runShortcut("F9", true));
  EXPECT_EQ(f.runs, 0);
  EXPECT_TRUE(f.controller->runShortcut("Ctrl+O", true));
  EXPECT_EQ(f.runs, 1);
  EXPECT_FALSE(f.controller->runShortcut("Ctrl+K", false));
}

TEST(ShellController, KeysAreWrittenAsShortcutsThroughTheModel) {
  Fixture f;
  // 0x78 是 F9，0x4F 是 O，修饰键位 Ctrl=2
  EXPECT_EQ(f.controller->shortcutFromKey(0x78, 0), "F9");
  EXPECT_EQ(f.controller->shortcutFromKey(0x4F, 2), "Ctrl+O");
  // 修饰键本身不是快捷键
  EXPECT_FALSE(f.controller->shortcutFromKey(0x11, 2).has_value());
}

TEST(ShellController, AltMnemonicOpensAMenuThatHasItems) {
  Fixture f;
  EXPECT_EQ(f.controller->menuForShortcut("Alt+F"), "file");
  EXPECT_EQ(f.controller->menuForShortcut("Alt+M"), "machine");
  // 帮助菜单没有项，Alt+H 不打开
  EXPECT_FALSE(f.controller->menuForShortcut("Alt+H").has_value());
  EXPECT_FALSE(f.controller->menuForShortcut("Alt+Z").has_value());
}

TEST(ShellController, FullScreenAndTopmostGoThroughTheWindowAndFollowItsState) {
  Fixture f;
  f.controller->runCommand("shell.toggleFullScreen");
  f.controller->runCommand("shell.toggleAlwaysOnTop");
  EXPECT_EQ(f.window.fullScreenToggles, 1);
  EXPECT_EQ(f.window.topmostToggles, 1);
  int changes = 0;
  f.controller->setOnChanged([&changes] { ++changes; });
  f.controller->setWindowState(true, true);
  EXPECT_TRUE(f.controller->fullScreen());
  EXPECT_TRUE(*f.controller->menus()[2].items[0].checked);
  f.controller->setWindowState(true, true);
  EXPECT_EQ(changes, 1);
}

TEST(ShellController, PanelButtonTogglesOpensSwitchesAndCloses) {
  Fixture f;
  EXPECT_FALSE(f.controller->openPanel().has_value());
  f.controller->togglePanel("sample.panel");
  ASSERT_TRUE(f.controller->openPanel().has_value());
  EXPECT_EQ(f.controller->openPanel()->id, "sample.panel");
  auto buttons = f.controller->panelButtons();
  ASSERT_EQ(buttons.size(), 2U);
  EXPECT_TRUE(buttons[0].pressed);
  EXPECT_FALSE(buttons[1].pressed);
  f.controller->togglePanel("sample.plain");
  EXPECT_EQ(f.controller->openPanel()->id, "sample.plain");
  f.controller->togglePanel("sample.plain");
  EXPECT_FALSE(f.controller->openPanel().has_value());
  EXPECT_FALSE(f.controller->panelButtons()[1].pressed);
  f.controller->togglePanel("sample.nope");
  EXPECT_FALSE(f.controller->openPanel().has_value());
  f.controller->togglePanel("sample.panel");
  f.controller->closePanel();
  EXPECT_FALSE(f.controller->openPanel().has_value());
}

TEST(ShellController, PanelButtonLookFollowsTheModuleState) {
  Fixture f;
  EXPECT_EQ(f.controller->panelButtons()[0].icon, contracts::ShellIcon::PlayerPlay);
  EXPECT_EQ(f.controller->panelButtons()[0].hint, "运行");
  f.powered = true;
  EXPECT_EQ(f.controller->panelButtons()[0].icon, contracts::ShellIcon::PlayerPause);
  EXPECT_EQ(f.controller->panelButtons()[0].hint, "运行：运行中");
}

TEST(ShellController, ClickingOutsideCollapsesOnlyPanelsWithAutoCollapseOn) {
  Fixture f;
  f.controller->togglePanel("sample.panel");
  ASSERT_TRUE(f.controller->openPanel().has_value());
  EXPECT_TRUE(f.controller->openPanel()->autoCollapse);
  EXPECT_TRUE(f.controller->openPanel()->autoCollapseOn);
  f.controller->pressedOutsidePanel();
  EXPECT_FALSE(f.controller->openPanel().has_value());
  // 没有登记自动收缩的面板点外面不收起
  f.controller->togglePanel("sample.plain");
  f.controller->pressedOutsidePanel();
  EXPECT_TRUE(f.controller->openPanel().has_value());
}

TEST(ShellController, TurningAutoCollapseOffIsSavedAndKeptNextTime) {
  Fixture f;
  f.controller->setPanelAutoCollapse("sample.panel", false);
  EXPECT_EQ(f.state.layoutUpdates, 1);
  EXPECT_EQ(f.state.state.layout.autoCollapseOffPanels, std::vector<std::string>{"sample.panel"});
  f.controller->togglePanel("sample.panel");
  EXPECT_FALSE(f.controller->openPanel()->autoCollapseOn);
  f.controller->pressedOutsidePanel();
  EXPECT_TRUE(f.controller->openPanel().has_value());
  // 同一个值再设一次不重复保存
  f.controller->setPanelAutoCollapse("sample.panel", false);
  EXPECT_EQ(f.state.layoutUpdates, 1);
  f.controller->setPanelAutoCollapse("sample.panel", true);
  EXPECT_TRUE(f.state.state.layout.autoCollapseOffPanels.empty());
  // 新的控制器从保存的状态读出
  Fixture next({"sample.panel"});
  next.controller->togglePanel("sample.panel");
  EXPECT_FALSE(next.controller->openPanel()->autoCollapseOn);
  EXPECT_EQ(next.state.layoutUpdates, 0);
}

TEST(ShellController, StatusItemsAreOnlyThereWhenTheyHaveInformation) {
  Fixture f;
  EXPECT_TRUE(f.controller->statusItems(false).empty());
  f.powered = true;
  const auto items = f.controller->statusItems(false);
  ASSERT_EQ(items.size(), 1U);
  EXPECT_EQ(items[0].text, "经典 FC");
  EXPECT_EQ(items[0].title, "主机型号");
  EXPECT_FALSE(items[0].clickable);
  EXPECT_TRUE(f.controller->statusItems(true).empty());
}

TEST(ShellController, DroppedFilesGoToTheHandlerOfTheirExtensionOrAreExplained) {
  Fixture f;
  f.controller->dropFiles({"C:\\roms\\Game.NES", "C:\\roms\\notes.txt", "C:\\roms\\noext", "C:\\roms\\unreadable.nes"});
  EXPECT_EQ(f.lastDropped, "Game.NES");
  ASSERT_EQ(f.notifier.notices.size(), 2U);
  EXPECT_EQ(f.notifier.notices[0].first, "不支持拖入的文件：notes.txt。可以拖入 NES 卡带、软盘镜像或声音文件");
  EXPECT_EQ(f.notifier.notices[1].first, "不支持拖入的文件：noext。可以拖入 NES 卡带、软盘镜像或声音文件");
  EXPECT_EQ(f.notifier.notices[0].second, contracts::NotificationLevel::Error);
  // 全部读不了时才说“无法读取”
  Fixture other;
  other.controller->dropFiles({"C:\\roms\\unreadable.nes"});
  ASSERT_EQ(other.notifier.notices.size(), 1U);
  EXPECT_EQ(other.notifier.notices[0].first, "无法读取拖入的文件，请从文件管理器里拖入磁盘上的文件");
}

TEST(ShellController, DialogsOpenOnlyWhenRegistered) {
  Fixture f;
  f.controller->openDialog("sample.settings");
  f.controller->openDialog("sample.nope");
  EXPECT_EQ(f.dialogs.opened, std::vector<std::string>{"sample.settings"});
}

TEST(ShellController, ModulesAttachOnceAndDetachWhenTheControllerEnds) {
  Fixture f;
  f.controller->attachModules();
  f.controller->attachModules();
  EXPECT_EQ(f.detached, 0);
  f.controller->detachModules();
  EXPECT_EQ(f.detached, 1);
  f.controller->attachModules();
  f.controller.reset();
  EXPECT_EQ(f.detached, 2);
}

}  // namespace
}  // namespace ember::win32
