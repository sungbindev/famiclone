/*
 * ------------------------------------------------------------------------
 * 名称：machines-rules.hpp
 * 说明：机型模块用到的纯规则：从核心回复读出机型清单，机型的 BIOS 状态文字，“机型与 BIOS”对话框的行（只有需要 BIOS 的机型），错误说明的读取。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：规则见 docs/05-界面.md 第 4.7 节；不依赖界面与请求，便于单独测试。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "contracts/core-gateway/core-gateway.hpp"
#include "contracts/machines/machines-model.hpp"

namespace ember::win32::logic {

/** 核心登记的一个机型（machines.list 的一项）。 */
struct Machine {
  std::string id;
  std::string name;
  bool needsBios{false};
  bool supported{false};
  bool hasBios{false};
  std::string biosFileName;
  std::int64_t biosSize{0};
  bool operator==(const Machine&) const = default;
};

/** 从 machines.list 的结果读出机型清单，保持核心给出的顺序；格式不对时没有值。 */
[[nodiscard]] std::optional<std::vector<Machine>> parseBiosMachines(const nlohmann::json& result);
/** 从 machines.openBios 等结果里的 machine 读出一个机型；格式不对时没有值。 */
[[nodiscard]] std::optional<Machine> parseBiosMachine(const nlohmann::json& machine);

/** 一个机型的 BIOS 状态文字：暂未支持、未添加，或文件名与大小（KB，四舍五入）。 */
[[nodiscard]] std::string biosStatusText(const Machine& machine);
/** “机型与 BIOS”对话框的行：只有需要 BIOS 的机型。 */
[[nodiscard]] std::vector<contracts::BiosRow> buildBiosRows(const std::vector<Machine>& machines);

/** 从请求失败的原因里取出给用户看的说明；没有说明时为“操作失败。”。 */
[[nodiscard]] std::string machineErrorText(const contracts::CoreError& error);

}  // namespace ember::win32::logic
