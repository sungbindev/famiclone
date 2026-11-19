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

namespace ember::win32::shared {

namespace {

constexpr long long kMaxCoordinate = 100000;
constexpr std::size_t kMaxMachineLength = 32;
constexpr std::size_t kMaxVersionLength = 64;
constexpr std::size_t kMaxPanelIdLength = 64;
constexpr std::size_t kMaxAutoCollapsePanels = 16;

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

/** 面板标识：模块名加点加面板名，如 runtime.panel。 */
bool isPanelId(const std::string& text) {
  if (text.empty() || text.size() > kMaxPanelIdLength) return false;
  const std::size_t dot = text.find('.');
  if (dot == std::string::npos || dot == 0 || text.find('.', dot + 1) != std::string::npos) return false;
  const std::string module = text.substr(0, dot);
  const std::string name = text.substr(dot + 1);
  if (!isLower(module[0]) || !std::all_of(module.begin(), module.end(), [](char c) { return isLower(c) || isDigit(c) || c == '-'; })) return false;
  return !name.empty() && isAlpha(name[0]) && std::all_of(name.begin(), name.end(), [](char c) { return isAlpha(c) || isDigit(c) || c == '-'; });
}

std::optional<int> parseCoordinate(const nlohmann::json& value) {
  if (!value.is_number_integer()) return std::nullopt;
  const long long number = value.get<long long>();
  if (number < -kMaxCoordinate || number > kMaxCoordinate) return std::nullopt;
  return static_cast<int>(number);
}

/** 窗口位置：任一项不合格时整个位置不用。 */
std::optional<WindowPlacement> parseWindow(const nlohmann::json& raw) {
  if (!raw.is_object() || !raw.contains("x") || !raw.contains("y")) return std::nullopt;
  const auto x = parseCoordinate(raw["x"]);
  const auto y = parseCoordinate(raw["y"]);
  if (!x || !y) return std::nullopt;
  return WindowPlacement{*x, *y};
}

/** 合格的面板标识清单：长度有限、每项写法合格、没有重复；不合格时为空。 */
std::optional<std::vector<std::string>> parsePanelList(const nlohmann::json& raw) {
  if (!raw.is_array() || raw.size() > kMaxAutoCollapsePanels) return std::nullopt;
  std::vector<std::string> result;
  for (const nlohmann::json& item : raw) {
    if (!item.is_string()) return std::nullopt;
    std::string id = item.get<std::string>();
    if (!isPanelId(id) || std::find(result.begin(), result.end(), id) != result.end()) return std::nullopt;
    result.push_back(std::move(id));
  }
  return result;
}

}  // namespace

ShellState parseShellState(const nlohmann::json& raw) {
  ShellState state;
  if (!raw.is_object()) return state;
  if (raw.contains("window")) state.window = parseWindow(raw["window"]);
  if (raw.contains("layout") && raw["layout"].is_object()) {
    const nlohmann::json& layout = raw["layout"];
    if (layout.contains("selectedMachine") && layout["selectedMachine"].is_string() && isMachineId(layout["selectedMachine"].get<std::string>())) state.layout.selectedMachine = layout["selectedMachine"].get<std::string>();
    if (layout.contains("autoCollapseOffPanels")) {
      if (auto panels = parsePanelList(layout["autoCollapseOffPanels"])) state.layout.autoCollapseOffPanels = std::move(*panels);
    }
  }
  if (raw.contains("releaseNotesVersion") && raw["releaseNotesVersion"].is_string() && isVersion(raw["releaseNotesVersion"].get<std::string>())) {
    state.releaseNotesVersion = raw["releaseNotesVersion"].get<std::string>();
  }
  return state;
}

nlohmann::json toJson(const ShellState& state) {
  nlohmann::json layout{{"selectedMachine", state.layout.selectedMachine ? nlohmann::json(*state.layout.selectedMachine) : nlohmann::json(nullptr)},
                        {"autoCollapseOffPanels", state.layout.autoCollapseOffPanels}};
  return {{"window", state.window ? nlohmann::json{{"x", state.window->x}, {"y", state.window->y}} : nlohmann::json(nullptr)},
          {"layout", layout},
          {"releaseNotesVersion", state.releaseNotesVersion ? nlohmann::json(*state.releaseNotesVersion) : nlohmann::json(nullptr)}};
}

}  // namespace ember::win32::shared
