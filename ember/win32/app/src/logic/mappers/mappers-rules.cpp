/*
 * ------------------------------------------------------------------------
 * 名称：mappers-rules.cpp
 * 说明：mapper 插件对话框纯规则的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "logic/mappers/mappers-rules.hpp"

#include <algorithm>
#include <set>

namespace ember::win32::logic {

namespace {

using nlohmann::json;

/** 用户可见的文字。 */
constexpr const char* kSourceBundled = "预装";
constexpr const char* kSourceUser = "用户安装";
constexpr const char* kSourceDevelopment = "开发目录";
constexpr const char* kNone = "无";
constexpr const char* kLabelId = "标识";
constexpr const char* kLabelVersion = "版本";
constexpr const char* kLabelPublisher = "发布者";
constexpr const char* kLabelDescription = "说明";
constexpr const char* kLabelMatch = "匹配";
constexpr const char* kLabelHost = "主机型号";
constexpr const char* kLabelDevices = "外设";
constexpr const char* kLabelStorage = "存储区";
constexpr const char* kLabelRegisters = "寄存器范围";
constexpr const char* kLabelState = "状态声明";
constexpr const char* kDeviceMouse = "鼠标";

std::string stringField(const json& object, const char* name) {
  const auto found = object.find(name);
  return found != object.end() && found->is_string() ? found->get<std::string>() : std::string();
}

bool boolField(const json& object, const char* name) {
  const auto found = object.find(name);
  return found != object.end() && found->is_boolean() && found->get<bool>();
}

std::optional<std::int64_t> integerField(const json& object, const char* name) {
  const auto found = object.find(name);
  if (found == object.end() || !found->is_number_integer()) return std::nullopt;
  return found->get<std::int64_t>();
}

std::vector<std::string> stringList(const json& object, const char* name) {
  std::vector<std::string> list;
  const auto found = object.find(name);
  if (found == object.end() || !found->is_array()) return list;
  for (const auto& item : *found) {
    if (item.is_string()) list.push_back(item.get<std::string>());
  }
  return list;
}

std::optional<MapperRule> parseRule(const json& value) {
  if (!value.is_object()) return std::nullopt;
  const auto mapper = integerField(value, "mapper");
  if (!mapper) return std::nullopt;
  MapperRule rule;
  rule.mapper = *mapper;
  rule.variant = integerField(value, "variant");
  rule.prgSize = integerField(value, "prgSize");
  rule.chrSize = integerField(value, "chrSize");
  if (const auto found = value.find("crc32Prg"); found != value.end() && found->is_string()) rule.crc32Prg = found->get<std::string>();
  if (const auto found = value.find("crc32All"); found != value.end() && found->is_string()) rule.crc32All = found->get<std::string>();
  return rule;
}

/** 字节数换成 KB 的文字：整除时是整数，否则保留小数。 */
std::string kilobytes(std::int64_t bytes) {
  if (bytes % 1024 == 0) return std::to_string(bytes / 1024);
  std::string text = std::to_string(static_cast<double>(bytes) / 1024.0);
  while (!text.empty() && text.back() == '0') text.pop_back();
  if (!text.empty() && text.back() == '.') text.pop_back();
  return text;
}

std::string upperCase(std::string text) {
  for (char& c : text) {
    if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
  }
  return text;
}

/** 去掉全部空白并转小写，用于搜索比较（全角空格也算空白）。 */
std::string squash(const std::string& text) {
  std::string result;
  for (std::size_t i = 0; i < text.size(); ++i) {
    const unsigned char c = static_cast<unsigned char>(text[i]);
    if (c == ' ' || (c >= '\t' && c <= '\r')) continue;
    if (c == 0xE3 && i + 2 < text.size() && static_cast<unsigned char>(text[i + 1]) == 0x80 && static_cast<unsigned char>(text[i + 2]) == 0x80) {
      i += 2;
      continue;
    }
    result += c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : static_cast<char>(c);
  }
  return result;
}

std::string storageText(const MapperStorage& item) {
  if (item.kind == "rom") return item.id + "：只读（" + (item.from == "file.prg" ? "来自 PRG" : "来自 CHR") + "）";
  return item.id + "：可写 " + std::to_string(item.size) + " 字节" + (item.battery ? "，带电池" : "");
}

std::string registerText(const MapperRegister& item) {
  const char* access = item.read && item.write ? "读写" : (item.read ? "读" : "写");
  return std::string(item.bus == "cpu" ? "CPU" : "PPU") + " " + item.range + "（" + access + "）";
}

std::string stateText(const MapperState& item) { return item.name + "（" + std::to_string(item.bits) + " 位）" + (item.description.empty() ? std::string() : "：" + item.description); }

/** 没有内容时写“无”。 */
std::vector<std::string> linesOrNone(std::vector<std::string> lines) {
  if (lines.empty()) lines.push_back(kNone);
  return lines;
}

}  // namespace

std::optional<MapperSummary> parseMapperSummary(const json& value) {
  if (!value.is_object() || stringField(value, "id").empty()) return std::nullopt;
  MapperSummary plugin;
  plugin.id = stringField(value, "id");
  plugin.name = stringField(value, "name");
  plugin.version = stringField(value, "version");
  plugin.publisher = stringField(value, "publisher");
  plugin.description = stringField(value, "description");
  plugin.host = stringList(value, "host");
  plugin.devices = stringList(value, "devices");
  plugin.source = stringField(value, "source");
  plugin.enabled = boolField(value, "enabled");
  const auto match = value.find("match");
  if (match != value.end() && match->is_array()) {
    for (const auto& item : *match) {
      if (auto rule = parseRule(item)) plugin.match.push_back(std::move(*rule));
    }
  }
  return plugin;
}

std::optional<std::vector<MapperSummary>> parseMapperList(const json& result) {
  const auto plugins = result.is_object() ? result.find("plugins") : result.end();
  if (plugins == result.end() || !plugins->is_array()) return std::nullopt;
  std::vector<MapperSummary> list;
  for (const auto& item : *plugins) {
    if (auto plugin = parseMapperSummary(item)) list.push_back(std::move(*plugin));
  }
  return list;
}

std::optional<MapperDetail> parseMapperDetail(const json& result) {
  const auto plugin = result.is_object() ? result.find("plugin") : result.end();
  if (plugin == result.end() || !plugin->is_object()) return std::nullopt;
  const auto summary = plugin->find("summary");
  if (summary == plugin->end()) return std::nullopt;
  auto parsed = parseMapperSummary(*summary);
  if (!parsed) return std::nullopt;
  MapperDetail detail;
  detail.summary = std::move(*parsed);
  if (const auto storage = plugin->find("storage"); storage != plugin->end() && storage->is_array()) {
    for (const auto& item : *storage) {
      if (!item.is_object()) continue;
      detail.storage.push_back({stringField(item, "id"), stringField(item, "kind"), stringField(item, "from"), integerField(item, "size").value_or(0), boolField(item, "battery")});
    }
  }
  if (const auto registers = plugin->find("registers"); registers != plugin->end() && registers->is_array()) {
    for (const auto& item : *registers) {
      if (item.is_object()) detail.registers.push_back({stringField(item, "bus"), stringField(item, "range"), boolField(item, "read"), boolField(item, "write")});
    }
  }
  if (const auto state = plugin->find("state"); state != plugin->end() && state->is_array()) {
    for (const auto& item : *state) {
      if (item.is_object()) detail.state.push_back({stringField(item, "name"), integerField(item, "bits").value_or(0), stringField(item, "description")});
    }
  }
  return detail;
}

std::string sourceLabel(const std::string& source) {
  if (source == "bundled") return kSourceBundled;
  return source == "development" ? kSourceDevelopment : kSourceUser;
}

std::string hostLabel(const std::string& host) {
  if (host == "fc") return "经典 FC";
  if (host == "sb97") return "小霸王 97";
  if (host == "sb2000") return "小霸王 2000";
  if (host == "bbg1") return "步步高 软驱一号";
  if (host == "bbg98") return "步步高 98";
  return host;
}

std::string ruleText(const MapperRule& rule) {
  std::vector<std::string> parts{"mapper " + std::to_string(rule.mapper)};
  if (rule.variant) parts.push_back("变体 " + std::to_string(*rule.variant));
  if (rule.prgSize) parts.push_back("PRG " + kilobytes(*rule.prgSize) + " KB");
  if (rule.chrSize) parts.push_back("CHR " + kilobytes(*rule.chrSize) + " KB");
  if (rule.crc32Prg) parts.push_back("PRG 校验 " + upperCase(*rule.crc32Prg));
  if (rule.crc32All) parts.push_back("整体校验 " + upperCase(*rule.crc32All));
  std::string text;
  for (std::size_t i = 0; i < parts.size(); ++i) text += (i == 0 ? "" : "，") + parts[i];
  return text;
}

std::string matchesText(const std::vector<MapperRule>& rules) {
  std::string text;
  for (std::size_t i = 0; i < rules.size(); ++i) text += (i == 0 ? "" : "；") + ruleText(rules[i]);
  return text;
}

std::string hostsText(const std::vector<std::string>& hosts) {
  std::string text;
  for (std::size_t i = 0; i < hosts.size(); ++i) text += (i == 0 ? "" : "、") + hostLabel(hosts[i]);
  return text;
}

std::vector<MapperSummary> filterMappers(const std::vector<MapperSummary>& plugins, const std::string& source, const std::string& search) {
  const std::string needle = squash(search);
  std::vector<MapperSummary> result;
  for (const MapperSummary& plugin : plugins) {
    if (source != contracts::kMapperSourceAll && plugin.source != source) continue;
    if (needle.empty() || squash(plugin.name).find(needle) != std::string::npos || squash(plugin.id).find(needle) != std::string::npos || squash(plugin.publisher).find(needle) != std::string::npos ||
        squash(plugin.description).find(needle) != std::string::npos) {
      result.push_back(plugin);
    }
  }
  return result;
}

std::string statsText(const std::vector<MapperSummary>& plugins) {
  std::set<std::int64_t> numbers;
  std::size_t enabled = 0;
  for (const MapperSummary& plugin : plugins) {
    for (const MapperRule& rule : plugin.match) numbers.insert(rule.mapper);
    if (plugin.enabled) ++enabled;
  }
  return "共 " + std::to_string(plugins.size()) + " 个插件（已启用 " + std::to_string(enabled) + " 个），支持 " + std::to_string(numbers.size()) + " 种 mapper";
}

contracts::MapperRow toRow(const MapperSummary& plugin) {
  contracts::MapperRow row;
  row.id = plugin.id;
  row.name = plugin.name;
  row.description = plugin.description;
  row.version = plugin.version;
  row.match = matchesText(plugin.match);
  row.hosts = hostsText(plugin.host);
  row.source = sourceLabel(plugin.source);
  row.enabled = plugin.enabled;
  row.bundled = plugin.source == "bundled";
  row.development = plugin.source == "development";
  return row;
}

std::vector<contracts::MapperFact> detailFacts(const MapperDetail& detail) {
  const MapperSummary& summary = detail.summary;
  std::vector<contracts::MapperFact> facts;
  facts.push_back({kLabelId, {summary.id}});
  facts.push_back({kLabelVersion, {summary.version}});
  facts.push_back({kLabelPublisher, {summary.publisher}});
  if (!summary.description.empty()) facts.push_back({kLabelDescription, {summary.description}});
  std::vector<std::string> match;
  for (const MapperRule& rule : summary.match) match.push_back(ruleText(rule));
  facts.push_back({kLabelMatch, match});
  facts.push_back({kLabelHost, {hostsText(summary.host)}});
  if (!summary.devices.empty()) {
    std::string devices;
    for (std::size_t i = 0; i < summary.devices.size(); ++i) devices += (i == 0 ? "" : "、") + (summary.devices[i] == "mouse" ? std::string(kDeviceMouse) : summary.devices[i]);
    facts.push_back({kLabelDevices, {devices}});
  }
  std::vector<std::string> storage;
  for (const MapperStorage& item : detail.storage) storage.push_back(storageText(item));
  facts.push_back({kLabelStorage, linesOrNone(std::move(storage))});
  std::vector<std::string> registers;
  for (const MapperRegister& item : detail.registers) registers.push_back(registerText(item));
  facts.push_back({kLabelRegisters, linesOrNone(std::move(registers))});
  std::vector<std::string> state;
  for (const MapperState& item : detail.state) state.push_back(stateText(item));
  facts.push_back({kLabelState, linesOrNone(std::move(state))});
  return facts;
}

std::string mapperErrorText(const contracts::CoreError& error, const std::string& fallback) { return error.message.empty() ? fallback : error.message; }

}  // namespace ember::win32::logic
