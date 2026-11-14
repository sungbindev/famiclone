/*
 * ------------------------------------------------------------------------
 * 名称：probe-hub.hpp
 * 说明：总线探针的分发：把总线访问同时交给断点表（读写断点）与记录表（I/O 记录）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：调试访问（单步、调试写内存）期间可以分别暂停断点与记录，被暂停的访问不触发。
 * ------------------------------------------------------------------------
 */

#pragma once

#include "contracts/hardware/access-probe.hpp"
#include "logic/hardware/debug/breakpoint-table.hpp"
#include "logic/hardware/debug/recorder-table.hpp"

namespace ember::logic::hardware {

/** 探针分发。 */
class ProbeHub final : public contracts::hardware::AccessProbe {
 public:
  ProbeHub(BreakpointTable& breakpoints, RecorderTable& recorders) : breakpoints_(breakpoints), recorders_(recorders) {}

  /** 要不要装到总线上。 */
  [[nodiscard]] bool needed() const { return breakpoints_.hasAccess() || recorders_.recordsIo(); }

  /** 暂停断点与记录；返回暂停之前的状态，用 restore 恢复。 */
  struct Suspension {
    bool breakpoints;
    bool recording;
  };
  Suspension suspend(bool breakpoints, bool recording) {
    const Suspension before{breakpointsSuspended_, recordingSuspended_};
    breakpointsSuspended_ = breakpointsSuspended_ || breakpoints;
    recordingSuspended_ = recordingSuspended_ || recording;
    return before;
  }
  void restore(const Suspension& before) {
    breakpointsSuspended_ = before.breakpoints;
    recordingSuspended_ = before.recording;
  }

  void onAccess(contracts::hardware::AccessKind kind, std::uint16_t address, std::uint8_t value) override {
    if (!breakpointsSuspended_ && breakpoints_.hasAccess()) breakpoints_.onAccess(kind, address, value);
    if (!recordingSuspended_ && recorders_.recordsIo()) recorders_.onIo(kind, address, value);
  }

 private:
  BreakpointTable& breakpoints_;
  RecorderTable& recorders_;
  bool breakpointsSuspended_{false};
  bool recordingSuspended_{false};
};

}  // namespace ember::logic::hardware
