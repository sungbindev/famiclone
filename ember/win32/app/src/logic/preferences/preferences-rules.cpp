/*
 * ------------------------------------------------------------------------
 * 名称：preferences-rules.cpp
 * 说明：首选项模块纯规则的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-20
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "logic/preferences/preferences-rules.hpp"

#include <algorithm>
#include <stdexcept>

namespace ember::win32::logic {

namespace {

/** 用户可见的文字。 */
constexpr const char* kNotInteger = "请输入整数。";
constexpr const char* kFailed = "操作失败。";

/** JavaScript 的安全整数范围：设置项没有登记上下限时，范围说明用它们。 */
constexpr std::int64_t kSafeMinimum = -9007199254740991;
constexpr std::int64_t kSafeMaximum = 9007199254740991;

std::string stringField(const nlohmann::json& object, const char* name) {
  const auto found = object.find(name);
  return found != object.end() && found->is_string() ? found->get<std::string>() : std::string();
}

bool boolField(const nlohmann::json& object, const char* name) {
  const auto found = object.find(name);
  return found != object.end() && found->is_boolean() && found->get<bool>();
}

std::optional<std::int64_t> integerField(const nlohmann::json& object, const char* name) {
  const auto found = object.find(name);
  if (found == object.end() || !found->is_number_integer()) return std::nullopt;
  return found->get<std::int64_t>();
}

std::optional<SettingKind> kindOf(const std::string& kind) {
  if (kind == "choice") return SettingKind::Choice;
  if (kind == "switch") return SettingKind::Switch;
  if (kind == "integer") return SettingKind::Integer;
  if (kind == "text") return SettingKind::Text;
  return std::nullopt;
}

contracts::PreferenceControl controlOf(SettingKind kind) {
  switch (kind) {
    case SettingKind::Choice: return contracts::PreferenceControl::Choice;
    case SettingKind::Switch: return contracts::PreferenceControl::Switch;
    case SettingKind::Integer: return contracts::PreferenceControl::Integer;
    case SettingKind::Text: return contracts::PreferenceControl::Text;
  }
  return contracts::PreferenceControl::Text;
}

std::optional<Setting> parseSetting(const nlohmann::json& item) {
  if (!item.is_object()) return std::nullopt;
  const auto kind = kindOf(stringField(item, "kind"));
  if (!kind || stringField(item, "id").empty() || !item.contains("value") || !item.contains("defaultValue")) return std::nullopt;
  Setting setting;
  setting.id = stringField(item, "id");
  setting.group = stringField(item, "group");
  setting.label = stringField(item, "label");
  setting.description = stringField(item, "description");
  setting.kind = *kind;
  setting.minimum = integerField(item, "minimum");
  setting.maximum = integerField(item, "maximum");
  setting.directoryPicker = stringField(item, "picker") == "directory";
  setting.hidden = boolField(item, "hidden");
  setting.defaultValue = item["defaultValue"];
  setting.value = item["value"];
  const auto options = item.find("options");
  if (options != item.end() && options->is_array()) {
    for (const auto& option : *options) {
      if (option.is_object()) setting.options.push_back({stringField(option, "value"), stringField(option, "label")});
    }
  }
  return setting;
}

contracts::PreferenceRow rowOf(const Setting& setting) {
  contracts::PreferenceRow row;
  row.id = setting.id;
  row.label = setting.label;
  row.description = setting.description;
  row.control = controlOf(setting.kind);
  row.options = setting.options;
  row.text = valueText(setting.value);
  row.checked = setting.value.is_boolean() && setting.value.get<bool>();
  row.changed = isChanged(setting);
  row.directoryPicker = setting.kind == SettingKind::Text && setting.directoryPicker;
  return row;
}

}  // namespace

std::optional<SettingListing> parseSettingListing(const nlohmann::json& result) {
  if (!result.is_object()) return std::nullopt;
  const auto groups = result.find("groups");
  const auto settings = result.find("settings");
  if (groups == result.end() || !groups->is_array() || settings == result.end() || !settings->is_array()) return std::nullopt;
  SettingListing listing;
  for (const auto& group : *groups) {
    if (group.is_object() && !stringField(group, "id").empty()) listing.groups.push_back({stringField(group, "id"), stringField(group, "label")});
  }
  for (const auto& item : *settings) {
    if (auto setting = parseSetting(item)) listing.settings.push_back(std::move(*setting));
  }
  return listing;
}

void applyValue(std::vector<Setting>& settings, const std::string& id, const nlohmann::json& value) {
  for (Setting& setting : settings) {
    if (setting.id == id) setting.value = value;
  }
}

bool isChanged(const Setting& setting) { return setting.value != setting.defaultValue; }

std::string valueText(const nlohmann::json& value) {
  if (value.is_string()) return value.get<std::string>();
  if (value.is_null()) return "null";
  return value.dump();
}

IntegerParse parseInteger(const std::string& text, std::optional<std::int64_t> minimum, std::optional<std::int64_t> maximum) {
  const auto first = text.find_first_not_of(" \t\r\n");
  const auto last = text.find_last_not_of(" \t\r\n");
  const std::string trimmed = first == std::string::npos ? std::string() : text.substr(first, last - first + 1);
  const std::size_t digitsFrom = !trimmed.empty() && trimmed[0] == '-' ? 1 : 0;
  const bool digits = trimmed.size() > digitsFrom && std::all_of(trimmed.begin() + static_cast<std::ptrdiff_t>(digitsFrom), trimmed.end(), [](char c) { return c >= '0' && c <= '9'; });
  if (!digits) return {false, 0, kNotInteger};
  const std::string range = "请输入 " + std::to_string(minimum.value_or(kSafeMinimum)) + " 至 " + std::to_string(maximum.value_or(kSafeMaximum)) + " 之间的整数。";
  std::int64_t value = 0;
  try {
    value = std::stoll(trimmed);
  } catch (const std::out_of_range&) {
    // 比 64 位整数还大：一定超出任何登记的范围
    return {false, 0, range};
  }
  if ((minimum && value < *minimum) || (maximum && value > *maximum)) return {false, 0, range};
  return {true, value, {}};
}

std::vector<contracts::PreferenceGroupView> buildGroupViews(const SettingListing& listing) {
  std::vector<contracts::PreferenceGroupView> groups;
  for (const SettingGroup& group : listing.groups) {
    contracts::PreferenceGroupView view;
    view.id = group.id;
    view.label = group.label;
    for (const Setting& setting : listing.settings) {
      if (setting.group == group.id && !setting.hidden) view.rows.push_back(rowOf(setting));
    }
    if (!view.rows.empty()) groups.push_back(std::move(view));
  }
  return groups;
}

std::string errorText(const contracts::CoreError& error) { return error.message.empty() ? std::string(kFailed) : error.message; }

}  // namespace ember::win32::logic
