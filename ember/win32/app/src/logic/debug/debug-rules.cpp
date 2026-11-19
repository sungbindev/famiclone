/*
 * ------------------------------------------------------------------------
 * 名称：debug-rules.cpp
 * 说明：调试服务模块纯规则的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "logic/debug/debug-rules.hpp"

namespace ember::win32::logic {

std::optional<DebugState> parseDebugStatus(const nlohmann::json& result) {
  if (!result.is_object() || !result.contains("enabled") || !result["enabled"].is_boolean()) return std::nullopt;
  DebugState state;
  state.enabled = result["enabled"].get<bool>();
  state.listening = result.contains("listening") && result["listening"].is_boolean() && result["listening"].get<bool>();
  state.port = result.contains("port") && result["port"].is_number_integer() ? result["port"].get<int>() : 0;
  state.connections = result.contains("connections") && result["connections"].is_array() ? static_cast<int>(result["connections"].size()) : 0;
  if (result.contains("error") && result["error"].is_string()) state.error = result["error"].get<std::string>();
  return state;
}

std::optional<contracts::ShellStatusItemView> debugItemView(const std::optional<DebugState>& state) {
  if (!state || !state->enabled) return std::nullopt;
  if (!state->listening) {
    return contracts::ShellStatusItemView{"调试模式", state->error.empty() ? "调试服务没有监听" : "调试服务没有监听：" + state->error, contracts::ShellTone::Warning};
  }
  std::string title = "调试服务：ws://127.0.0.1:" + std::to_string(state->port);
  if (state->connections > 0) title += "，" + std::to_string(state->connections) + " 个连接";
  return contracts::ShellStatusItemView{"调试模式", std::move(title), contracts::ShellTone::Success};
}

}  // namespace ember::win32::logic
