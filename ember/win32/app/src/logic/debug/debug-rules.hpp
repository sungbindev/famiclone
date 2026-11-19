/*
 * ------------------------------------------------------------------------
 * 名称：debug-rules.hpp
 * 说明：调试服务模块用到的纯规则：从 debug.status 的结果读出服务状态，得出系统栏右侧“调试模式”项显示的内容。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：规则同 Electron 外壳的 DebugStatusItem.tsx（见 docs/05-界面.md 第 6 节）：调试服务开启时显示绿色文字“调试模式”，没有开启或还没读到状态时不显示；端口、连接数放在悬停提示里
 *       （如“调试服务：ws://127.0.0.1:5688，2 个连接”）；开启了但没有监听（如端口被占用）时文字仍是“调试模式”，用警告色，提示里写原因。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <optional>
#include <string>

#include <nlohmann/json.hpp>

#include "contracts/shell/shell-registry.hpp"

namespace ember::win32::logic {

/** 调试服务的状态（debug.status 的结果）。 */
struct DebugState {
  bool enabled{false};
  bool listening{false};
  int port{0};
  /** 当前的连接数。 */
  int connections{0};
  /** 开启了但没有监听时的原因。 */
  std::string error;
  bool operator==(const DebugState&) const = default;
};

/** 从 debug.status 的结果读出服务状态；格式不对时没有值。 */
[[nodiscard]] std::optional<DebugState> parseDebugStatus(const nlohmann::json& result);

/** 系统栏右侧“调试模式”项此刻的内容；还没读到状态或没有开启时没有值（不显示）。 */
[[nodiscard]] std::optional<contracts::ShellStatusItemView> debugItemView(const std::optional<DebugState>& state);

}  // namespace ember::win32::logic
