/*
 * ------------------------------------------------------------------------
 * 名称：inspector-service.hpp
 * 说明：inspector 契约：检查器服务：列出内存空间、读写内存、解析总线地址、读写模块状态。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：可在任意线程调用，实际访问由模拟线程完成；错误码见 inspector-error.hpp。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "contracts/hardware/machine-debugger.hpp"

namespace ember::contracts::inspector {

/** 一个模块的状态。 */
struct StateReport {
  std::string module;
  bool writable{false};
  std::vector<hardware::StateField> fields;
};

/** 一个监视中的状态项。 */
struct WatchEntry {
  /** 状态路径：<模块>.<状态项>。 */
  std::string path;
  std::int64_t value{0};
};

/** 一个时刻的完整快照。 */
struct SnapshotReport {
  /** 自上电以来的 CPU 周期数。 */
  std::uint64_t cycles{0};
  std::vector<StateReport> states;
  std::vector<hardware::Breakpoint> breakpoints;
  std::vector<WatchEntry> watches;
};

/** 一项记录的结果。 */
struct RecordingReport {
  /** 保存的文件的完整路径。 */
  std::string file;
  /** 写入的行数。 */
  std::size_t lines{0};
  /** 因为行数满了而少记了。 */
  bool truncated{false};
};

/** 检查器服务。 */
class InspectorService {
 public:
  virtual ~InspectorService() = default;

  /** 全部内存空间：主机自己的，加上卡带上的存储区（cart.<标识>）。 */
  virtual std::vector<hardware::MemorySpace> listSpaces() = 0;

  /** 无副作用地读一段内存。 */
  virtual std::vector<std::uint8_t> readMemory(const std::string& space, std::uint32_t address, std::uint32_t length) = 0;

  /** 写一段内存；返回写入的字节数。 */
  virtual std::size_t writeMemory(const std::string& space, std::uint32_t address, std::span<const std::uint8_t> bytes) = 0;

  /** 解析总线地址（space 为 cpu 或 ppu）。 */
  virtual hardware::AddressTarget resolve(const std::string& space, std::uint32_t address) = 0;

  /** 读状态；module 为空时返回全部模块。 */
  virtual std::vector<StateReport> getState(const std::optional<std::string>& module) = 0;

  /** 修改一个模块的状态，返回修改之后的状态。 */
  virtual StateReport setState(const std::string& module, std::span<const hardware::StateField> fields) = 0;

  /** 完整快照；module 为空时包含全部模块的状态。 */
  virtual SnapshotReport snapshot(const std::optional<std::string>& module) = 0;

  /** 监视一个状态项（取值变化时发 inspector.stateChanged），返回当前取值；已在监视时没有效果。 */
  virtual WatchEntry watch(const std::string& path) = 0;

  /** 取消监视；没有监视这个路径时抛出 inspector.noWatch。 */
  virtual void unwatch(const std::string& path) = 0;

  /**
   * 开始跟踪或调用记录：内容记在内存里，stopRecording 时写成调试输出目录下的文件。
   * @param fileName 文件名；为空时按种类与编号生成。
   * @return 记录编号。
   * @throws shared::CoreError output.invalidFileName、inspector.recordingLimit。
   */
  virtual std::uint32_t startRecording(const hardware::RecorderSpec& spec, const std::string& fileName) = 0;

  /**
   * 结束记录并写文件。callLog 为 true 时只能结束调用记录，否则只能结束跟踪。
   * @throws shared::CoreError inspector.noRecording、output.fileFailed。
   */
  virtual RecordingReport stopRecording(std::uint32_t id, bool callLog) = 0;

  /** 添加断点（CPU 总线上的地址，范围含两端）。 */
  virtual hardware::Breakpoint addBreakpoint(hardware::BreakpointKind kind, std::uint16_t first, std::uint16_t last) = 0;

  /** 全部断点。 */
  virtual std::vector<hardware::Breakpoint> listBreakpoints() = 0;

  /** 删除一个断点；没有这个编号时抛出 inspector.noBreakpoint。 */
  virtual void removeBreakpoint(std::uint32_t id) = 0;

  /** 删除全部断点，返回删除的个数。 */
  virtual std::size_t clearBreakpoints() = 0;
};

}  // namespace ember::contracts::inspector
