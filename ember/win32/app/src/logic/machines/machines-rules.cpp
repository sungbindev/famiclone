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
constexpr const char* kNoBios = "未添加";
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

std::optional<Machine> parseBiosMachine(const nlohmann::json& machine) {
  if (!machine.is_object() || stringField(machine, "id").empty()) return std::nullopt;
  Machine result;
  result.id = stringField(machine, "id");
  result.name = stringField(machine, "name");
  result.needsBios = boolField(machine, "needsBios");
  result.supported = boolField(machine, "supported");
  const auto bios = machine.find("bios");
  if (bios != machine.end() && bios->is_object()) {
    result.hasBios = true;
    result.biosFileName = stringField(*bios, "fileName");
    result.biosSize = integerField(*bios, "size").value_or(0);
  }
  return result;
}

std::optional<std::vector<Machine>> parseBiosMachines(const nlohmann::json& result) {
  const auto machines = result.is_object() ? result.find("machines") : result.end();
  if (machines == result.end() || !machines->is_array()) return std::nullopt;
  std::vector<Machine> list;
  for (const auto& item : *machines) {
    if (auto machine = parseBiosMachine(item)) list.push_back(std::move(*machine));
  }
  return list;
}

std::string biosStatusText(const Machine& machine) {
  if (!machine.supported) return kUnsupported;
  if (!machine.hasBios) return kNoBios;
  const auto kilobytes = static_cast<std::int64_t>(std::floor(static_cast<double>(machine.biosSize) / 1024.0 + 0.5));
  return machine.biosFileName + "（" + std::to_string(kilobytes) + " KB）";
}

std::vector<contracts::BiosRow> buildBiosRows(const std::vector<Machine>& machines) {
  std::vector<contracts::BiosRow> rows;
  for (const Machine& machine : machines) {
    if (machine.needsBios) rows.push_back({machine.id, machine.name, biosStatusText(machine), machine.hasBios, machine.supported});
  }
  return rows;
}

std::string machineErrorText(const contracts::CoreError& error) { return error.message.empty() ? std::string(kFailed) : error.message; }

}  // namespace ember::win32::logic
