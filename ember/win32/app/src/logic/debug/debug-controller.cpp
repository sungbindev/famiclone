/*
 * ------------------------------------------------------------------------
 * 名称：debug-controller.cpp
 * 说明：调试服务状态控制器的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "logic/debug/debug-controller.hpp"

namespace ember::win32::logic {

DebugController::DebugController(contracts::RequestBroker& broker, contracts::CoreGateway& core) : broker_(broker), core_(core) {}

DebugController::~DebugController() {
  *alive_ = false;
  for (const auto& stop : stops_) stop();
}

std::function<void()> DebugController::attach(contracts::ShellApi& shell) {
  const auto stopAll = [this] {
    for (const auto& stop : stops_) stop();
    stops_.clear();
    shell_ = nullptr;
    ++generation_;
    ++request_;
  };
  stopAll();
  shell_ = &shell;
  stops_.push_back(core_.onEvent([this](const std::string& name, const nlohmann::json&) {
    if (name == "debug.statusChanged") load();
  }));
  stops_.push_back(core_.onStatusChange([this](const contracts::CoreStatus& status) {
    if (status.state == contracts::CoreState::Running) {
      load();
      return;
    }
    ++request_;
    setState(std::nullopt);
  }));
  load();
  return stopAll;
}

void DebugController::load() {
  if (shell_ == nullptr) return;
  const auto current = ++request_;
  const auto alive = alive_;
  const auto generation = generation_;
  broker_.request("debug.status", nlohmann::json::object(), [this, alive, generation, current](contracts::CoreReply reply) {
    if (!*alive || generation != generation_ || shell_ == nullptr || current != request_) return;
    setState(reply.ok ? parseDebugStatus(reply.result) : std::nullopt);
  });
}

void DebugController::setState(std::optional<DebugState> next) {
  if (next == state_) return;
  state_ = std::move(next);
  if (shell_ != nullptr) shell_->refreshCommands();
}

}  // namespace ember::win32::logic
