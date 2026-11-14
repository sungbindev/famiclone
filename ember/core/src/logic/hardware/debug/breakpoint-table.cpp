/*
 * ------------------------------------------------------------------------
 * 名称：breakpoint-table.cpp
 * 说明：断点表的实现，见 breakpoint-table.hpp。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

#include "logic/hardware/debug/breakpoint-table.hpp"

#include <algorithm>
#include <string>

#include "contracts/hardware/hardware-error.hpp"
#include "shared/errors/core-error.hpp"

namespace ember::logic::hardware {

namespace hw = contracts::hardware;

hw::Breakpoint BreakpointTable::add(hw::BreakpointKind kind, std::uint16_t first, std::uint16_t last) {
  if (first > last) throw shared::CoreError(hw::ERROR_INVALID_BREAKPOINT, "断点地址范围的起点大于终点");
  if (entries_.size() >= MAX_BREAKPOINTS) throw shared::CoreError(hw::ERROR_BREAKPOINT_LIMIT, "断点最多 " + std::to_string(MAX_BREAKPOINTS) + " 个");
  hw::Breakpoint entry;
  entry.id = nextId_++;
  entry.kind = kind;
  entry.first = first;
  entry.last = last;
  entries_.push_back(entry);
  if (kind == hw::BreakpointKind::Execute) ++executeCount_;
  else ++accessCount_;
  return entry;
}

bool BreakpointTable::remove(std::uint32_t id) {
  const auto found = std::find_if(entries_.begin(), entries_.end(), [id](const hw::Breakpoint& entry) { return entry.id == id; });
  if (found == entries_.end()) return false;
  if (found->kind == hw::BreakpointKind::Execute) --executeCount_;
  else --accessCount_;
  entries_.erase(found);
  return true;
}

std::size_t BreakpointTable::clear() {
  const std::size_t removed = entries_.size();
  entries_.clear();
  executeCount_ = 0;
  accessCount_ = 0;
  pending_.reset();
  return removed;
}

std::vector<hw::Breakpoint> BreakpointTable::list() const { return entries_; }

bool BreakpointTable::hitsExecute(std::uint16_t pc) {
  for (hw::Breakpoint& entry : entries_) {
    if (entry.kind != hw::BreakpointKind::Execute || pc < entry.first || pc > entry.last) continue;
    ++entry.hits;
    pending_ = hw::BreakpointHit{entry.id, entry.kind, pc, 0};
    return true;
  }
  return false;
}

std::optional<hw::BreakpointHit> BreakpointTable::take() {
  auto hit = pending_;
  pending_.reset();
  return hit;
}

void BreakpointTable::onAccess(hw::AccessKind kind, std::uint16_t address, std::uint8_t value) {
  if (pending_.has_value()) return;
  const hw::BreakpointKind wanted = kind == hw::AccessKind::Read ? hw::BreakpointKind::Read : hw::BreakpointKind::Write;
  for (hw::Breakpoint& entry : entries_) {
    if (entry.kind != wanted || address < entry.first || address > entry.last) continue;
    ++entry.hits;
    pending_ = hw::BreakpointHit{entry.id, entry.kind, address, value};
    return;
  }
}

}  // namespace ember::logic::hardware
