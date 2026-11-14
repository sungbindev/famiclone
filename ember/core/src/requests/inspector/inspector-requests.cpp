/*
 * ------------------------------------------------------------------------
 * 名称：inspector-requests.cpp
 * 说明：inspector 模块的请求登记，见 inspector-requests.hpp。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：readMemory 的 data 以 nlohmann::json::binary 放在结果里，调试服务据此改发二进制帧。
 * ------------------------------------------------------------------------
 */

#include "requests/inspector/inspector-requests.hpp"

#include <utility>

#include "contracts/inspector/generated/inspector.hpp"

namespace ember::requests::inspector {

namespace contract = ::ember::contracts::inspector;

namespace {

namespace hw = ::ember::contracts::hardware;

hw::BreakpointKind kindOf(const std::string& name) {
  if (name == "read") return hw::BreakpointKind::Read;
  if (name == "write") return hw::BreakpointKind::Write;
  return hw::BreakpointKind::Execute;
}

contract::Breakpoint breakpointToWire(const hw::Breakpoint& breakpoint) {
  contract::Breakpoint wire;
  wire.id = breakpoint.id;
  wire.kind = breakpoint.kind == hw::BreakpointKind::Execute ? "execute" : (breakpoint.kind == hw::BreakpointKind::Read ? "read" : "write");
  wire.address = breakpoint.first;
  wire.endAddress = breakpoint.last;
  wire.hits = static_cast<std::int64_t>(breakpoint.hits);
  return wire;
}

contract::ModuleState toWire(const contract::StateReport& state) {
  contract::ModuleState wire;
  wire.module = state.module;
  wire.writable = state.writable;
  for (const auto& field : state.fields) wire.fields.push_back({field.name, field.value, field.description});
  return wire;
}

}  // namespace

void addInspectorRequests(platform::RequestRouter& router, std::shared_ptr<contract::InspectorService> service) {
  router.add("inspector", contract::INSPECTOR_LIST_SPACES_REQUEST, [service](const nlohmann::json& params, const platform::CancellationToken&) {
    static_cast<void>(contract::parseInspectorListSpacesParams(params, "params"));
    contract::InspectorListSpacesResult result;
    for (const auto& space : service->listSpaces()) result.spaces.push_back({space.id, space.description, static_cast<std::int64_t>(space.size), space.writable});
    return nlohmann::json(result);
  });
  router.add("inspector", contract::INSPECTOR_READ_MEMORY_REQUEST, [service](const nlohmann::json& params, const platform::CancellationToken&) {
    const auto parsed = contract::parseInspectorReadMemoryParams(params, "params");
    auto bytes = service->readMemory(parsed.space, static_cast<std::uint32_t>(parsed.address), static_cast<std::uint32_t>(parsed.length));
    contract::InspectorReadMemoryResult result;
    result.space = parsed.space;
    result.address = parsed.address;
    result.length = parsed.length;
    nlohmann::json json(result);
    json["data"] = nlohmann::json::binary(std::move(bytes));
    return json;
  });
  router.add("inspector", contract::INSPECTOR_WRITE_MEMORY_REQUEST, [service](const nlohmann::json& params, const platform::CancellationToken&) {
    const auto parsed = contract::parseInspectorWriteMemoryParams(params, "params");
    std::vector<std::uint8_t> bytes;
    bytes.reserve(parsed.bytes.size());
    for (const auto value : parsed.bytes) bytes.push_back(static_cast<std::uint8_t>(value));
    const auto written = service->writeMemory(parsed.space, static_cast<std::uint32_t>(parsed.address), bytes);
    return nlohmann::json(contract::InspectorWriteMemoryResult{parsed.space, parsed.address, static_cast<std::int64_t>(written)});
  });
  router.add("inspector", contract::INSPECTOR_RESOLVE_REQUEST, [service](const nlohmann::json& params, const platform::CancellationToken&) {
    const auto parsed = contract::parseInspectorResolveParams(params, "params");
    const auto target = service->resolve(parsed.space, static_cast<std::uint32_t>(parsed.address));
    contract::AddressInfo wire;
    wire.kind = target.kind;
    if (!target.region.empty()) {
      wire.region = target.region;
      wire.offset = static_cast<std::int64_t>(target.offset);
    }
    wire.description = target.description;
    return nlohmann::json(contract::InspectorResolveResult{wire});
  });
  router.add("inspector", contract::INSPECTOR_GET_STATE_REQUEST, [service](const nlohmann::json& params, const platform::CancellationToken&) {
    const auto parsed = contract::parseInspectorGetStateParams(params, "params");
    contract::InspectorGetStateResult result;
    for (const auto& state : service->getState(parsed.module)) result.states.push_back(toWire(state));
    return nlohmann::json(result);
  });
  router.add("inspector", contract::INSPECTOR_ADD_BREAKPOINT_REQUEST, [service](const nlohmann::json& params, const platform::CancellationToken&) {
    const auto parsed = contract::parseInspectorAddBreakpointParams(params, "params");
    const auto first = static_cast<std::uint16_t>(parsed.address);
    const auto last = static_cast<std::uint16_t>(parsed.endAddress.value_or(parsed.address));
    return nlohmann::json(contract::InspectorAddBreakpointResult{breakpointToWire(service->addBreakpoint(kindOf(parsed.kind), first, last))});
  });
  router.add("inspector", contract::INSPECTOR_LIST_BREAKPOINTS_REQUEST, [service](const nlohmann::json& params, const platform::CancellationToken&) {
    static_cast<void>(contract::parseInspectorListBreakpointsParams(params, "params"));
    contract::InspectorListBreakpointsResult result;
    for (const auto& breakpoint : service->listBreakpoints()) result.breakpoints.push_back(breakpointToWire(breakpoint));
    return nlohmann::json(result);
  });
  router.add("inspector", contract::INSPECTOR_REMOVE_BREAKPOINT_REQUEST, [service](const nlohmann::json& params, const platform::CancellationToken&) {
    const auto parsed = contract::parseInspectorRemoveBreakpointParams(params, "params");
    service->removeBreakpoint(static_cast<std::uint32_t>(parsed.id));
    return nlohmann::json(contract::InspectorRemoveBreakpointResult{});
  });
  router.add("inspector", contract::INSPECTOR_CLEAR_BREAKPOINTS_REQUEST, [service](const nlohmann::json& params, const platform::CancellationToken&) {
    static_cast<void>(contract::parseInspectorClearBreakpointsParams(params, "params"));
    return nlohmann::json(contract::InspectorClearBreakpointsResult{static_cast<std::int64_t>(service->clearBreakpoints())});
  });
  router.add("inspector", contract::INSPECTOR_SET_STATE_REQUEST, [service](const nlohmann::json& params, const platform::CancellationToken&) {
    const auto parsed = contract::parseInspectorSetStateParams(params, "params");
    std::vector<contracts::hardware::StateField> fields;
    for (const auto& field : parsed.fields) fields.push_back({field.name, field.value, field.description.value_or("")});
    return nlohmann::json(contract::InspectorSetStateResult{toWire(service->setState(parsed.module, fields))});
  });
}

}  // namespace ember::requests::inspector
