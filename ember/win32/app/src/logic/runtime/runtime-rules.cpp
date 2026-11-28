/*
 * ------------------------------------------------------------------------
 * 名称：runtime-rules.cpp
 * 说明：运行模块纯规则的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "logic/runtime/runtime-rules.hpp"

#include <algorithm>

namespace ember::win32::logic {

namespace {

/** 主机型号的显示名；表里没有的型号显示型号标识本身。 */
struct HostName {
  const char* id;
  const char* name;
};
constexpr HostName kHostNames[] = {{"fc", "经典 FC"},     {"sb97", "小霸王 97"},       {"sb2000", "小霸王 2000"},          {"bbg1", "步步高 软驱一号"},
                                   {"bbg98", "步步高 98"}, {"pec9588", "金字塔 PEC-9588"}, {"pec586", "金字塔 PEC-586(686)"}, {"yuxing", "裕兴"}, {"cec1", "中华学习机 CEC-I"}};

constexpr const char* kUnsupported = "暂未支持";
constexpr const char* kNoRom = "未添加 ROM";
constexpr const char* kMachineMenuTitle = "选择机型";

std::string stringOf(const nlohmann::json& object, const char* key) {
  const auto found = object.find(key);
  return found != object.end() && found->is_string() ? found->get<std::string>() : std::string();
}

bool boolOf(const nlohmann::json& object, const char* key) {
  const auto found = object.find(key);
  return found != object.end() && found->is_boolean() && found->get<bool>();
}

}  // namespace

std::optional<RuntimeStatus> parseRuntimeStatus(const nlohmann::json& result) {
  if (!result.is_object() || !result.contains("power") || !result.contains("mode")) return std::nullopt;
  RuntimeStatus status;
  status.powerOn = stringOf(result, "power") == "on";
  status.running = stringOf(result, "mode") == "running";
  if (const auto machine = stringOf(result, "machine"); !machine.empty()) status.machine = machine;
  if (const auto found = result.find("cartridge"); found != result.end() && found->is_object()) status.cartridge = RuntimeCartridge{stringOf(*found, "path"), stringOf(*found, "pluginName")};
  status.mouse = boolOf(result, "mouse");
  if (const auto label = stringOf(result, "romLabel"); !label.empty()) status.romLabel = label;
  return status;
}

std::vector<MachineEntry> parseMachines(const nlohmann::json& result) {
  std::vector<MachineEntry> machines;
  const auto list = result.is_object() ? result.find("machines") : result.end();
  if (!result.is_object() || list == result.end() || !list->is_array()) return machines;
  for (const auto& item : *list) {
    if (!item.is_object()) continue;
    MachineEntry entry;
    entry.id = stringOf(item, "id");
    entry.name = stringOf(item, "name");
    entry.needsRom = boolOf(item, "needsRom");
    entry.supported = boolOf(item, "supported");
    entry.hasRom = item.contains("rom") && !item["rom"].is_null();
    if (entry.hasRom && item["rom"].is_object()) entry.romLabel = stringOf(item["rom"], "label");
    if (!entry.id.empty()) machines.push_back(std::move(entry));
  }
  return machines;
}

std::vector<RecentCartridge> parseRecent(const nlohmann::json& result) {
  std::vector<RecentCartridge> items;
  const auto list = result.is_object() ? result.find("items") : result.end();
  if (!result.is_object() || list == result.end() || !list->is_array()) return items;
  for (const auto& item : *list) {
    if (!item.is_object() || !item.contains("id") || !item["id"].is_number_integer()) continue;
    items.push_back({item["id"].get<std::int64_t>(), stringOf(item, "name")});
  }
  return items;
}

std::string fileNameOf(const std::string& path) {
  const std::size_t slash = path.find_last_of("\\/");
  return slash == std::string::npos ? path : path.substr(slash + 1);
}

contracts::MachineState machineState(const std::optional<RuntimeStatus>& status) {
  if (!status) return contracts::MachineState::Unknown;
  if (!status->machine) return contracts::MachineState::None;
  if (!status->powerOn) return contracts::MachineState::Off;
  return status->running ? contracts::MachineState::Running : contracts::MachineState::Paused;
}

std::optional<std::string> stateText(const std::optional<RuntimeStatus>& status) {
  switch (machineState(status)) {
    case contracts::MachineState::Unknown: return std::nullopt;
    case contracts::MachineState::None: return std::string("未开机");
    case contracts::MachineState::Off: return std::string("已断电");
    case contracts::MachineState::Paused: return std::string("已暂停");
    case contracts::MachineState::Running: return std::string("运行中");
  }
  return std::nullopt;
}

std::optional<std::string> hostText(const std::optional<RuntimeStatus>& status, const std::string& selectedMachine) {
  if (!status) return std::nullopt;
  const std::string machine = status->machine.value_or(selectedMachine);
  if (machine == kClassicMachine) return std::nullopt;
  for (const auto& host : kHostNames) {
    if (machine == host.id) return std::string(host.name);
  }
  return machine;
}

std::optional<std::string> romVersionText(const std::optional<RuntimeStatus>& status, const std::string& selectedMachine, const std::vector<MachineEntry>& machines) {
  if (!status) return std::nullopt;
  if (status->machine) return *status->machine == kClassicMachine ? std::nullopt : status->romLabel;
  if (selectedMachine == kClassicMachine) return std::nullopt;
  const auto info = std::find_if(machines.begin(), machines.end(), [&selectedMachine](const MachineEntry& entry) { return entry.id == selectedMachine; });
  if (info == machines.end() || !info->hasRom || info->romLabel.empty()) return std::nullopt;
  return info->romLabel;
}

std::optional<std::string> cartridgeText(const std::optional<RuntimeStatus>& status) {
  if (!status || !status->cartridge) return std::nullopt;
  return fileNameOf(status->cartridge->path);
}

std::optional<std::string> pluginText(const std::optional<RuntimeStatus>& status) {
  if (!status || !status->cartridge) return std::nullopt;
  return status->cartridge->pluginName;
}

std::string powerToggleText(const std::optional<RuntimeStatus>& status) { return status && status->powerOn ? "断电" : "开机"; }

std::string pauseToggleText(const std::optional<RuntimeStatus>& status) { return machineState(status) == contracts::MachineState::Running ? "暂停" : "继续"; }

PowerToggleAction powerToggleAction(const std::optional<RuntimeStatus>& status, const std::string& selectedMachine, const std::vector<MachineEntry>& machines) {
  if (!status) return PowerToggleAction::None;
  if (status->powerOn) return PowerToggleAction::PowerOff;
  // 经典 FC 装着卡带时，断电后能再开机。
  if (selectedMachine == kClassicMachine) return status->machine == std::optional<std::string>(kClassicMachine) ? PowerToggleAction::PowerOn : PowerToggleAction::None;
  const auto info = std::find_if(machines.begin(), machines.end(), [&selectedMachine](const MachineEntry& entry) { return entry.id == selectedMachine; });
  return info != machines.end() && info->supported && info->hasRom ? PowerToggleAction::Boot : PowerToggleAction::None;
}

contracts::RuntimeControls controlAvailability(const std::optional<RuntimeStatus>& status, const std::string& selectedMachine, const std::vector<MachineEntry>& machines) {
  const auto state = machineState(status);
  const bool powered = state == contracts::MachineState::Paused || state == contracts::MachineState::Running;
  contracts::RuntimeControls controls;
  controls.powerToggle = powerToggleAction(status, selectedMachine, machines) != PowerToggleAction::None;
  controls.pauseToggle = powered;
  controls.softReset = powered;
  controls.hardReset = powered;
  controls.eject = status && status->cartridge.has_value();
  return controls;
}

bool capturesMouse(bool enabled, const std::optional<RuntimeStatus>& status) { return enabled && status && status->mouse; }

bool exclusiveKeyboard(bool familyKeyboard, const std::optional<RuntimeStatus>& status) {
  return familyKeyboard || (status && status->machine && *status->machine != kClassicMachine);
}

std::vector<MachineChoice> machineChoices(const std::vector<MachineEntry>& machines) {
  struct Choice {
    const char* id;
    const char* label;
    /** 品牌的第一个机型，上面加分隔线。 */
    bool separatorBefore;
  };
  static constexpr Choice kChoices[] = {{"sb97", "小霸王 97", false},  {"sb2000", "小霸王 2000", false},          {"bbg1", "步步高 软驱一号", true},
                                        {"bbg98", "步步高 98", false}, {"pec586", "金字塔 PEC-586(686)", true}, {"pec9588", "金字塔 PEC-9588", false}, {"yuxing", "裕兴", true},
                                        {"cec1", "中华学习机 CEC-I", true}};
  std::vector<MachineChoice> result;
  for (const auto& choice : kChoices) {
    const auto info = std::find_if(machines.begin(), machines.end(), [&choice](const MachineEntry& entry) { return entry.id == choice.id; });
    std::string hint;
    if (info == machines.end() || !info->supported) hint = kUnsupported;
    else if (!info->hasRom) hint = kNoRom;
    result.push_back({choice.id, choice.label, hint, choice.separatorBefore});
  }
  return result;
}

std::string machineMenuTitle(const std::vector<MachineEntry>& machines, const std::string& selectedMachine) {
  for (const auto& choice : machineChoices(machines)) {
    if (choice.id == selectedMachine) return choice.label;
  }
  return kMachineMenuTitle;
}

}  // namespace ember::win32::logic
