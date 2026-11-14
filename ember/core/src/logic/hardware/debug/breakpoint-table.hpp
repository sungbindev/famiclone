/*
 * ------------------------------------------------------------------------
 * 名称：breakpoint-table.hpp
 * 说明：断点表：执行、读、写三种断点，每个带地址范围与命中计数；作为总线访问探针记录读写命中，执行断点由主机在指令之前查询。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：同一次运行里只记录第一个命中，取走（take）之后才记录下一个；各主机型号共用。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "contracts/hardware/access-probe.hpp"
#include "contracts/hardware/machine-debugger.hpp"

namespace ember::logic::hardware {

/** 断点表。 */
class BreakpointTable final : public contracts::hardware::AccessProbe {
 public:
  /** 断点数量上限。 */
  static constexpr std::size_t MAX_BREAKPOINTS = 256;

  /** 添加断点，规则见 MachineDebugger::addBreakpoint。 */
  contracts::hardware::Breakpoint add(contracts::hardware::BreakpointKind kind, std::uint16_t first, std::uint16_t last);

  /** 删除断点；没有这个编号时返回 false。 */
  bool remove(std::uint32_t id);

  /** 删除全部断点，返回删除的个数；同时丢弃没有取走的命中。 */
  std::size_t clear();

  /** 全部断点，按编号升序。 */
  [[nodiscard]] std::vector<contracts::hardware::Breakpoint> list() const;

  /** 有没有执行断点。 */
  [[nodiscard]] bool hasExecute() const { return executeCount_ > 0; }

  /** 有没有读写断点（需要在总线上安装探针）。 */
  [[nodiscard]] bool hasAccess() const { return accessCount_ > 0; }

  /** 有没有没取走的命中。 */
  [[nodiscard]] bool hasPending() const { return pending_.has_value(); }

  /**
   * 查询执行断点：pc 落在某个执行断点的范围里就记录命中。
   * @return 命中返回 true。
   */
  bool hitsExecute(std::uint16_t pc);

  /** 取走命中；没有时为空。 */
  std::optional<contracts::hardware::BreakpointHit> take();

  /** 丢弃没有取走的命中。 */
  void discardPending() { pending_.reset(); }

  void onAccess(contracts::hardware::AccessKind kind, std::uint16_t address, std::uint8_t value) override;

 private:
  std::vector<contracts::hardware::Breakpoint> entries_;
  std::uint32_t nextId_{1};
  std::size_t executeCount_{0};
  std::size_t accessCount_{0};
  std::optional<contracts::hardware::BreakpointHit> pending_;
};

}  // namespace ember::logic::hardware
