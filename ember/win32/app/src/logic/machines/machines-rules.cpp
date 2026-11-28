/*
 * ------------------------------------------------------------------------
 * 名称：machines-rules.cpp
 * 说明：机型模块纯规则的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "logic/machines/machines-rules.hpp"

#include <cmath>

namespace ember::win32::logic {

namespace {

/** 用户可见的文字。 */
constexpr const char* kFailed = "操作失败。";
constexpr const char* kNoRom = "未添加";
constexpr const char* kUnsupported = "暂未支持";

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

}  // namespace

std::optional<Machine> parseRomMachine(const nlohmann::json& machine) {
  if (!machine.is_object() || stringField(machine, "id").empty()) return std::nullopt;
  Machine result;
  result.id = stringField(machine, "id");
  result.name = stringField(machine, "name");
  result.needsRom = boolField(machine, "needsRom");
  result.supported = boolField(machine, "supported");
  const auto rom = machine.find("rom");
  if (rom != machine.end() && rom->is_object()) {
    result.hasRom = true;
    result.romFileName = stringField(*rom, "fileName");
    result.romSize = integerField(*rom, "size").value_or(0);
  }
  return result;
}

std::optional<std::vector<Machine>> parseRomMachines(const nlohmann::json& result) {
  const auto machines = result.is_object() ? result.find("machines") : result.end();
  if (machines == result.end() || !machines->is_array()) return std::nullopt;
  std::vector<Machine> list;
  for (const auto& item : *machines) {
    if (auto machine = parseRomMachine(item)) list.push_back(std::move(*machine));
  }
  return list;
}

std::string romStatusText(const Machine& machine) {
  if (!machine.supported) return kUnsupported;
  if (!machine.hasRom) return kNoRom;
  const auto kilobytes = static_cast<std::int64_t>(std::floor(static_cast<double>(machine.romSize) / 1024.0 + 0.5));
  return machine.romFileName + "（" + std::to_string(kilobytes) + " KB）";
}

std::vector<contracts::RomRow> buildRomRows(const std::vector<Machine>& machines) {
  std::vector<contracts::RomRow> rows;
  for (const Machine& machine : machines) {
    if (machine.needsRom) rows.push_back({machine.id, machine.name, romStatusText(machine), machine.hasRom, machine.supported});
  }
  return rows;
}

std::optional<RomImport> parseRomImport(const nlohmann::json& result) {
  const auto added = result.is_object() ? result.find("added") : result.end();
  const auto skipped = result.is_object() ? result.find("skipped") : result.end();
  if (added == result.end() || !added->is_array() || skipped == result.end() || !skipped->is_array()) return std::nullopt;
  RomImport summary;
  summary.added = static_cast<int>(added->size());
  for (const auto& item : *skipped) {
    if (!item.is_object()) continue;
    if (stringField(item, "reason") == "unrecognized") ++summary.unrecognized;
    else ++summary.kept;
  }
  return summary;
}

namespace {

/** 添加结果通知的后缀：已有 ROM 未改动与认不出的文件数（没有对应情况时不写）。 */
std::string importSuffix(const RomImport& import) {
  std::string text;
  if (import.kept > 0) text += "；" + std::to_string(import.kept) + " 个文件的机型已有 ROM，未改动";
  if (import.unrecognized > 0) text += "；" + std::to_string(import.unrecognized) + " 个文件认不出是哪个机型的 ROM，已跳过";
  return text;
}

}  // namespace

std::pair<std::string, contracts::NotificationLevel> romImportNotice(const RomImport& import) {
  std::string text;
  if (import.added > 0) text = "已从目录添加 " + std::to_string(import.added) + " 个机型的 ROM";
  else text = "目录里没有可添加的 ROM";
  text += importSuffix(import);
  return {text, import.added > 0 ? contracts::NotificationLevel::Success : contracts::NotificationLevel::Warning};
}

std::pair<std::string, contracts::NotificationLevel> romDownloadNotice(const RomImport& import) {
  std::string text;
  if (import.added > 0) text = "已下载并添加 " + std::to_string(import.added) + " 个机型的 ROM";
  else text = "ROM 包里没有可添加的 ROM";
  text += importSuffix(import);
  return {text, import.added > 0 ? contracts::NotificationLevel::Success : contracts::NotificationLevel::Warning};
}

std::string machineErrorText(const contracts::CoreError& error) { return error.message.empty() ? std::string(kFailed) : error.message; }

}  // namespace ember::win32::logic
