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
