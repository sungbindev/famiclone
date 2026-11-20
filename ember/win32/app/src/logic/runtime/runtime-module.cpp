/*
 * ------------------------------------------------------------------------
 * 名称：runtime-module.cpp
 * 说明：运行模块登记项的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "logic/runtime/runtime-module.hpp"

namespace ember::win32::logic {

namespace {

using contracts::ShellApi;
using contracts::ShellCommandDefinition;
using contracts::ShellMenuItemDefinition;
using contracts::ShellViewState;

/** 子菜单的标识。 */
constexpr const char* kMachineSubmenuId = "runtime.machine";
constexpr const char* kRecentSubmenuId = "runtime.recent";

ShellCommandDefinition makeCommand(const char* id, std::function<std::string()> title, const char* shortcut, std::function<void(ShellApi&)> run) {
  ShellCommandDefinition command;
  command.id = id;
  command.title = std::move(title);
  command.defaultShortcut = shortcut;
  command.run = std::move(run);
  return command;
}

std::function<std::string()> fixedTitle(const char* text) {
  return [text] { return std::string(text); };
}

ShellMenuItemDefinition commandItem(const char* menu, const char* command, bool separatorBefore = false) {
  ShellMenuItemDefinition item;
  item.menu = menu;
  item.command = command;
  item.separatorBefore = separatorBefore;
  return item;
}

}  // namespace

contracts::ShellModule createRuntimeModule(RuntimeController& controller) {
  contracts::ShellModule module;
  module.name = "runtime";
  module.hasScreen = true;

  // 运行面板：按钮图标实时显示运行状态——运行中是绿色三角形，暂停是暂停图标，其余（未开机、已断电）是常色三角形
  contracts::ShellPanelDefinition panel;
  panel.id = kRuntimePanelId;
  panel.title = "运行";
  panel.icon = contracts::ShellIcon::PlayerPlay;
  panel.autoCollapse = true;
  panel.look = [&controller] {
    contracts::ShellPanelLook look;
    const auto state = machineState(controller.status());
    const auto text = stateText(controller.status());
    look.hint = text ? "运行：" + *text : std::string("运行");
    if (state == contracts::MachineState::Running) {
      look.icon = contracts::ShellIcon::PlayerPlay;
      look.tone = contracts::ShellTone::Success;
    } else if (state == contracts::MachineState::Paused) {
      look.icon = contracts::ShellIcon::PlayerPause;
    }
    return look;
  };
  module.bottomPanels.push_back(std::move(panel));

  auto makeEnabled = [&controller](bool contracts::RuntimeControls::*field) {
    return [&controller, field](const ShellViewState&) { return controller.availability().*field; };
  };

  module.commands.push_back(makeCommand("runtime.openCartridge", fixedTitle("打开卡带"), "Ctrl+O", [&controller](ShellApi&) { controller.openCartridge(); }));
  auto eject = makeCommand("runtime.ejectCartridge", fixedTitle("关闭卡带"), "", [&controller](ShellApi&) { controller.ejectCartridge(); });
  eject.enabled = makeEnabled(&contracts::RuntimeControls::eject);
  module.commands.push_back(std::move(eject));
  auto power = makeCommand("runtime.powerToggle", [&controller] { return powerToggleText(controller.status()); }, "F9", [&controller](ShellApi&) { controller.togglePower(); });
  power.enabled = makeEnabled(&contracts::RuntimeControls::powerToggle);
  module.commands.push_back(std::move(power));
  auto pause = makeCommand("runtime.pauseToggle", [&controller] { return pauseToggleText(controller.status()); }, "F8", [&controller](ShellApi&) { controller.togglePause(); });
  pause.enabled = makeEnabled(&contracts::RuntimeControls::pauseToggle);
  module.commands.push_back(std::move(pause));
  auto softReset = makeCommand("runtime.softReset", fixedTitle("软复位"), "F7", [&controller](ShellApi&) { controller.softReset(); });
  softReset.enabled = makeEnabled(&contracts::RuntimeControls::softReset);
  module.commands.push_back(std::move(softReset));
  auto hardReset = makeCommand("runtime.hardReset", fixedTitle("硬复位"), "Ctrl+F7", [&controller](ShellApi&) { controller.hardReset(); });
  hardReset.enabled = makeEnabled(&contracts::RuntimeControls::hardReset);
  module.commands.push_back(std::move(hardReset));
  auto mute = makeCommand("runtime.muteToggle", fixedTitle("静音"), "", [&controller](ShellApi&) { controller.toggleMute(); });
  mute.checked = [&controller](const ShellViewState&) { return controller.muted(); };
  module.commands.push_back(std::move(mute));
  auto familyKeyboard = makeCommand("runtime.familyKeyboardToggle", fixedTitle("接入 Family BASIC 键盘"), "", [&controller](ShellApi&) { controller.toggleFamilyKeyboard(); });
  familyKeyboard.checked = [&controller](const ShellViewState&) { return controller.familyKeyboard(); };
  module.commands.push_back(std::move(familyKeyboard));

  // “选择机型”的子菜单项：已选的打对勾，点一项记住并开机；程序里有实现且已添加 BIOS 才可选，否则置灰并在右侧注明原因
  ShellMenuItemDefinition machineMenu;
  machineMenu.menu = "machine";
  machineMenu.submenu = contracts::ShellSubmenuDefinition{kMachineSubmenuId, "选择机型", [&controller] {
    std::vector<contracts::ShellSubmenuEntry> entries;
    for (const auto& choice : machineChoices(controller.machines())) {
      contracts::ShellSubmenuEntry entry;
      entry.id = choice.id;
      entry.label = choice.label;
      entry.checked = choice.id == controller.selectedMachine();
      entry.hint = choice.hint;
      entry.disabled = !choice.hint.empty();
      entry.run = [&controller, id = choice.id](ShellApi&) { controller.selectAndBootMachine(id); };
      entries.push_back(std::move(entry));
    }
    return entries;
  }};
  machineMenu.submenu->titleText = [&controller] { return machineMenuTitle(controller.machines(), controller.selectedMachine()); };
  module.menuItems.push_back(std::move(machineMenu));
  module.menuItems.push_back(commandItem("file", "runtime.openCartridge"));
  // “最近打开”的子菜单项：最近打开的卡带，选中即重新打开
  ShellMenuItemDefinition recentMenu;
  recentMenu.menu = "file";
  recentMenu.submenu = contracts::ShellSubmenuDefinition{kRecentSubmenuId, "最近打开", [&controller] {
    std::vector<contracts::ShellSubmenuEntry> entries;
    for (const auto& item : controller.recent()) {
      contracts::ShellSubmenuEntry entry;
      entry.id = std::to_string(item.id);
      entry.label = item.name;
      entry.run = [&controller, id = item.id](ShellApi&) { controller.openRecent(id); };
      entries.push_back(std::move(entry));
    }
    return entries;
  }};
  module.menuItems.push_back(std::move(recentMenu));
  module.menuItems.push_back(commandItem("file", "runtime.ejectCartridge"));
  module.menuItems.push_back(commandItem("machine", "runtime.powerToggle", true));
  module.menuItems.push_back(commandItem("machine", "runtime.pauseToggle"));
  module.menuItems.push_back(commandItem("machine", "runtime.softReset", true));
  module.menuItems.push_back(commandItem("machine", "runtime.hardReset"));
  module.menuItems.push_back(commandItem("machine", "runtime.muteToggle", true));
  module.menuItems.push_back(commandItem("machine", "runtime.familyKeyboardToggle"));

  // 系统栏左侧的项：主机型号、卡带名、mapper 插件；没有信息的项不显示
  const auto statusItem = [](const char* id, const char* title, std::function<std::optional<std::string>()> text) {
    contracts::ShellStatusItemDefinition item;
    item.id = id;
    item.view = [text = std::move(text), title]() -> std::optional<contracts::ShellStatusItemView> {
      const auto shown = text();
      if (!shown) return std::nullopt;
      return contracts::ShellStatusItemView{*shown, title, contracts::ShellTone::Normal};
    };
    return item;
  };
  module.statusItems.push_back(statusItem("runtime.host", "主机型号", [&controller] { return hostText(controller.status(), controller.selectedMachine()); }));
  module.statusItems.push_back(statusItem("runtime.cartridge", "卡带", [&controller] { return cartridgeText(controller.status()); }));
  module.statusItems.push_back(statusItem("runtime.plugin", "mapper 插件", [&controller] { return pluginText(controller.status()); }));

  module.fileHandlers.push_back({"runtime.cartridge", {"nes"}, [&controller](const contracts::DroppedFile& file) { controller.openDroppedCartridge(file.id); }});
  module.attach = [&controller](ShellApi& shell) { return controller.attach(shell); };
  return module;
}

}  // namespace ember::win32::logic
