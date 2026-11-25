/*
 * ------------------------------------------------------------------------
 * 名称：shell-state.cpp
 * 说明：界面状态的解析与序列化。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "shared/shell-state/shell-state.hpp"

#include <algorithm>
#include <cctype>

#include "shared/shortcut/shortcut.hpp"

namespace ember::win32::shared {

namespace {

constexpr std::size_t kMaxMachineLength = 32;
constexpr std::size_t kMaxVersionLength = 64;
constexpr std::size_t kMaxPanelIdLength = 64;
constexpr std::size_t kMaxAutoCollapsePanels = 16;
constexpr std::size_t kMaxKeybindings = 128;
constexpr std::size_t kMaxPathLength = 1024;

bool isLower(char c) { return c >= 'a' && c <= 'z'; }
bool isAlpha(char c) { return isLower(c) || (c >= 'A' && c <= 'Z'); }
bool isDigit(char c) { return c >= '0' && c <= '9'; }

/** 机型标识：小写字母开头，字母与数字，最长 32。 */
bool isMachineId(const std::string& text) {
  if (text.empty() || text.size() > kMaxMachineLength || !isLower(text[0])) return false;
  return std::all_of(text.begin(), text.end(), [](char c) { return isLower(c) || isDigit(c); });
}

/** 应用版本：数字开头，只含字母、数字与 . + -，最长 64。 */
bool isVersion(const std::string& text) {
  if (text.empty() || text.size() > kMaxVersionLength || !isDigit(text[0])) return false;
  return std::all_of(text.begin(), text.end(), [](char c) { return isAlpha(c) || isDigit(c) || c == '.' || c == '+' || c == '-'; });
}

/** 登记项标识（面板、命令）：模块名加点加名字，如 runtime.panel。 */
bool isRegisteredId(const std::string& text) {
  if (text.empty() || text.size() > kMaxPanelIdLength) return false;
  const std::size_t dot = text.find('.');
  if (dot == std::string::npos || dot == 0 || text.find('.', dot + 1) != std::string::npos) return false;
  const std::string module = text.substr(0, dot);
  const std::string name = text.substr(dot + 1);
  if (!isLower(module[0]) || !std::all_of(module.begin(), module.end(), [](char c) { return isLower(c) || isDigit(c) || c == '-'; })) return false;
  return !name.empty() && isAlpha(name[0]) && std::all_of(name.begin(), name.end(), [](char c) { return isAlpha(c) || isDigit(c) || c == '-'; });
}

/** 目录路径：非空、不超过上限、不含控制字符。 */
bool isDirectoryPath(const std::string& text) {
  return !text.empty() && text.size() <= kMaxPathLength && std::none_of(text.begin(), text.end(), [](char c) { return static_cast<unsigned char>(c) < 0x20; });
}

/** 合格的面板标识清单：长度有限、每项写法合格、没有重复；不合格时为空。 */
std::optional<std::vector<std::string>> parsePanelList(const nlohmann::json& raw) {
  if (!raw.is_array() || raw.size() > kMaxAutoCollapsePanels) return std::nullopt;
  std::vector<std::string> result;
  for (const nlohmann::json& item : raw) {
    if (!item.is_string()) return std::nullopt;
    std::string id = item.get<std::string>();
    if (!isRegisteredId(id) || std::find(result.begin(), result.end(), id) != result.end()) return std::nullopt;
    result.push_back(std::move(id));
  }
  return result;
}

/** 用户修改的快捷键：不合格的项单独丢弃，其余照常保留。 */
std::map<std::string, std::string> parseKeybindings(const nlohmann::json& raw) {
  std::map<std::string, std::string> result;
  if (!raw.is_object()) return result;
  for (const auto& [id, shortcut] : raw.items()) {
    if (result.size() >= kMaxKeybindings) break;
    if (!isRegisteredId(id) || !shortcut.is_string()) continue;
    const std::string text = shortcut.get<std::string>();
    if (text.empty() || isValidShortcutText(text)) result.emplace(id, text);
  }
  return result;
}

}  // namespace

ShellState parseShellState(const nlohmann::json& raw) {
  ShellState state;
  if (!raw.is_object()) return state;
  if (raw.contains("layout") && raw["layout"].is_object()) {
    const nlohmann::json& layout = raw["layout"];
    if (layout.contains("selectedMachine") && layout["selectedMachine"].is_string() && isMachineId(layout["selectedMachine"].get<std::string>())) state.layout.selectedMachine = layout["selectedMachine"].get<std::string>();
    if (layout.contains("autoCollapseOffPanels")) {
      if (auto panels = parsePanelList(layout["autoCollapseOffPanels"])) state.layout.autoCollapseOffPanels = std::move(*panels);
    }
    if (layout.contains("alwaysOnTop") && layout["alwaysOnTop"].is_boolean()) state.layout.alwaysOnTop = layout["alwaysOnTop"].get<bool>();
    if (layout.contains("fileManagerOpen") && layout["fileManagerOpen"].is_boolean()) state.layout.fileManagerOpen = layout["fileManagerOpen"].get<bool>();
    if (layout.contains("keybindings")) state.layout.keybindings = parseKeybindings(layout["keybindings"]);
  }
  if (raw.contains("releaseNotesVersion") && raw["releaseNotesVersion"].is_string() && isVersion(raw["releaseNotesVersion"].get<std::string>())) {
    state.releaseNotesVersion = raw["releaseNotesVersion"].get<std::string>();
  }
  if (raw.contains("fileManagerRoot") && raw["fileManagerRoot"].is_string() && isDirectoryPath(raw["fileManagerRoot"].get<std::string>())) state.fileManagerRoot = raw["fileManagerRoot"].get<std::string>();
  return state;
}

nlohmann::json toJson(const ShellState& state) {
  nlohmann::json layout{{"selectedMachine", state.layout.selectedMachine ? nlohmann::json(*state.layout.selectedMachine) : nlohmann::json(nullptr)},
                        {"autoCollapseOffPanels", state.layout.autoCollapseOffPanels},
                        {"alwaysOnTop", state.layout.alwaysOnTop},
                        {"fileManagerOpen", state.layout.fileManagerOpen},
                        {"keybindings", state.layout.keybindings}};
  return {{"layout", layout},
          {"releaseNotesVersion", state.releaseNotesVersion ? nlohmann::json(*state.releaseNotesVersion) : nlohmann::json(nullptr)},
          {"fileManagerRoot", state.fileManagerRoot ? nlohmann::json(*state.fileManagerRoot) : nlohmann::json(nullptr)}};
}

}  // namespace ember::win32::shared
