/*
 * ------------------------------------------------------------------------
 * 名称：debug-controller.hpp
 * 说明：调试服务状态的控制器：核心就绪后读取 debug.status，收到 debug.statusChanged 或核心重启后重新读取，供系统栏的“调试模式”项显示。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：状态事件里没有出错原因，所以每次变化都重新请求 debug.status；读取失败或核心不可用时回到“还没读到”；状态变化时让外壳重画状态栏。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

#include "contracts/core-gateway/core-gateway.hpp"
#include "contracts/shell/request-broker.hpp"
#include "contracts/shell/shell-registry.hpp"
#include "logic/debug/debug-rules.hpp"

namespace ember::win32::logic {

/** 调试服务状态的控制器。 */
class DebugController final {
 public:
  /**
   * @param broker 请求中转。
   * @param core 核心网关：订阅核心事件与核心运行状态。
   */
  DebugController(contracts::RequestBroker& broker, contracts::CoreGateway& core);
  ~DebugController();
  DebugController(const DebugController&) = delete;
  DebugController& operator=(const DebugController&) = delete;

  /**
   * 接上外壳：订阅核心事件与核心运行状态，并读取初始状态。
   * @returns 断开的函数。
   */
  std::function<void()> attach(contracts::ShellApi& shell);

  /** 当前状态；还没读到时没有值。 */
  [[nodiscard]] const std::optional<DebugState>& state() const { return state_; }

 private:
  void load();
  void setState(std::optional<DebugState> next);

  contracts::RequestBroker& broker_;
  contracts::CoreGateway& core_;
  contracts::ShellApi* shell_{nullptr};
  std::vector<std::function<void()>> stops_;
  std::shared_ptr<bool> alive_{std::make_shared<bool>(true)};
  /** 每次接上外壳加一：上一次接上时发出的请求，回复到了也不处理。 */
  std::uint64_t generation_{0};
  /** 最近一次读取的序号：较早发出的读取结果晚到时丢弃。 */
  std::uint64_t request_{0};
  std::optional<DebugState> state_;
};

}  // namespace ember::win32::logic
