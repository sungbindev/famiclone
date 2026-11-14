/*
 * ------------------------------------------------------------------------
 * 名称：local-inspector-service.hpp
 * 说明：检查器服务的实现：经运行服务的调试访问在模拟线程读写主机与卡带，把卡带上的存储区合成 cart.<标识> 空间，把运行与硬件的错误码转成 inspector 的错误码。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

#pragma once

#include <memory>

#include "contracts/inspector/inspector-service.hpp"
#include "contracts/runtime/runtime-service.hpp"

namespace ember::logic::inspector {

/** 检查器服务。 */
class LocalInspectorService final : public contracts::inspector::InspectorService {
 public:
  explicit LocalInspectorService(std::shared_ptr<contracts::runtime::RuntimeService> runtime);

  std::vector<contracts::hardware::MemorySpace> listSpaces() override;
  std::vector<std::uint8_t> readMemory(const std::string& space, std::uint32_t address, std::uint32_t length) override;
  std::size_t writeMemory(const std::string& space, std::uint32_t address, std::span<const std::uint8_t> bytes) override;
  contracts::hardware::AddressTarget resolve(const std::string& space, std::uint32_t address) override;
  std::vector<contracts::inspector::StateReport> getState(const std::optional<std::string>& module) override;
  contracts::inspector::StateReport setState(const std::string& module, std::span<const contracts::hardware::StateField> fields) override;
  contracts::hardware::Breakpoint addBreakpoint(contracts::hardware::BreakpointKind kind, std::uint16_t first, std::uint16_t last) override;
  std::vector<contracts::hardware::Breakpoint> listBreakpoints() override;
  void removeBreakpoint(std::uint32_t id) override;
  std::size_t clearBreakpoints() override;

 private:
  /** 在模拟线程执行回调，并把错误码转成 inspector 的。 */
  void inspect(const std::function<void(contracts::runtime::DebugContext&)>& action);

  std::shared_ptr<contracts::runtime::RuntimeService> runtime_;
};

}  // namespace ember::logic::inspector
