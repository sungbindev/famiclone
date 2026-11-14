/*
 * ------------------------------------------------------------------------
 * 名称：local-inspector-service.cpp
 * 说明：检查器服务的实现，见 local-inspector-service.hpp。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

#include "logic/inspector/local-inspector-service.hpp"

#include <algorithm>
#include <string>
#include <utility>

#include "contracts/hardware/hardware-error.hpp"
#include "contracts/inspector/inspector-error.hpp"
#include "contracts/runtime/runtime-error.hpp"
#include "shared/errors/core-error.hpp"

namespace ember::logic::inspector {

namespace api = contracts::inspector;
namespace hw = contracts::hardware;
namespace rt = contracts::runtime;

namespace {

/** 卡带存储区空间的前缀。 */
constexpr std::string_view CART_PREFIX = "cart.";

/** 运行与硬件的错误码转成 inspector 的；其余原样返回。 */
std::string translate(const std::string& code) {
  if (code == rt::ERROR_NO_CARTRIDGE) return std::string(api::ERROR_NO_CARTRIDGE);
  if (code == rt::ERROR_NOT_POWERED) return std::string(api::ERROR_NOT_POWERED);
  if (code == rt::ERROR_DEBUG_UNSUPPORTED) return std::string(api::ERROR_UNSUPPORTED);
  if (code == hw::ERROR_UNKNOWN_SPACE) return std::string(api::ERROR_UNKNOWN_SPACE);
  if (code == hw::ERROR_SPACE_OUT_OF_RANGE || code == hw::ERROR_STORAGE_OUT_OF_RANGE) return std::string(api::ERROR_OUT_OF_RANGE);
  if (code == hw::ERROR_SPACE_READ_ONLY) return std::string(api::ERROR_READ_ONLY);
  if (code == hw::ERROR_INVALID_STATE) return std::string(api::ERROR_INVALID_STATE);
  if (code == hw::ERROR_INVALID_BREAKPOINT) return std::string(api::ERROR_INVALID_BREAKPOINT);
  if (code == hw::ERROR_BREAKPOINT_LIMIT) return std::string(api::ERROR_BREAKPOINT_LIMIT);
  return code;
}

/** 卡带存储区空间的存储区标识；不是 cart. 开头时返回空。 */
std::optional<std::string> cartStorageId(const std::string& space) {
  if (!space.starts_with(CART_PREFIX)) return std::nullopt;
  return space.substr(CART_PREFIX.size());
}

/** 按空间标识取卡带的存储区；没有时抛出 inspector.unknownSpace。 */
const hw::StorageRegion& cartStorage(rt::DebugContext& context, const std::string& id, const std::string& space) {
  const hw::StorageRegion* region = context.cartridge == nullptr ? nullptr : context.cartridge->storage(id);
  if (region == nullptr) throw shared::CoreError(hw::ERROR_UNKNOWN_SPACE, "没有这个内存空间：" + space);
  return *region;
}

void checkRange(const std::string& space, std::size_t size, std::uint32_t address, std::size_t length) {
  if (address > size || length > size - address) {
    throw shared::CoreError(hw::ERROR_SPACE_OUT_OF_RANGE, "空间 " + space + " 的大小是 " + std::to_string(size) + " 字节，访问 " + std::to_string(address) + " 起的 " + std::to_string(length) + " 字节超出范围");
  }
}

/** 卡带脚本状态。 */
api::StateReport mapperState(const contracts::mappers::MapperCartridge& cartridge) {
  api::StateReport state{"mapper", false, {}};
  for (const std::string& name : cartridge.stateNames()) state.fields.push_back({name, static_cast<std::int64_t>(cartridge.state(name).value_or(0)), "卡带脚本的内部状态（只读）"});
  return state;
}

}  // namespace

LocalInspectorService::LocalInspectorService(std::shared_ptr<rt::RuntimeService> runtime) : runtime_(std::move(runtime)) {}

void LocalInspectorService::inspect(const std::function<void(rt::DebugContext&)>& action) {
  try {
    runtime_->inspect(action);
  } catch (const shared::CoreError& error) {
    const std::string code(error.code());
    const std::string translated = translate(code);
    if (translated == code) throw;
    throw shared::CoreError(translated, error.what());
  }
}

std::vector<hw::MemorySpace> LocalInspectorService::listSpaces() {
  std::vector<hw::MemorySpace> spaces;
  inspect([&](rt::DebugContext& context) {
    spaces = context.debugger.memorySpaces();
    if (context.cartridge == nullptr) return;
    for (const auto& item : context.cartridge->storageList()) {
      const bool ram = item.kind == hw::StorageKind::Ram;
      spaces.push_back({std::string(CART_PREFIX) + item.id, std::string("卡带的存储区 ") + item.id + (ram ? "（RAM）" : "（ROM，不能写）"), item.size, ram});
    }
  });
  return spaces;
}

std::vector<std::uint8_t> LocalInspectorService::readMemory(const std::string& space, std::uint32_t address, std::uint32_t length) {
  std::vector<std::uint8_t> bytes(length);
  inspect([&](rt::DebugContext& context) {
    if (const auto id = cartStorageId(space)) {
      const hw::StorageRegion& region = cartStorage(context, *id, space);
      checkRange(space, region.size(), address, length);
      region.readBlock(address, bytes);
    } else {
      context.debugger.readMemory(space, address, bytes);
    }
  });
  return bytes;
}

std::size_t LocalInspectorService::writeMemory(const std::string& space, std::uint32_t address, std::span<const std::uint8_t> bytes) {
  inspect([&](rt::DebugContext& context) {
    if (const auto id = cartStorageId(space)) {
      const hw::StorageRegion& region = cartStorage(context, *id, space);
      checkRange(space, region.size(), address, bytes.size());
      if (region.kind() == hw::StorageKind::Rom) throw shared::CoreError(hw::ERROR_SPACE_READ_ONLY, "空间 " + space + " 是 ROM，不能写入");
      for (std::size_t index = 0; index < bytes.size(); ++index) context.cartridge->pokeStorage(*id, address + index, bytes[index]);
    } else {
      context.debugger.writeMemory(space, address, bytes);
    }
  });
  return bytes.size();
}

hw::AddressTarget LocalInspectorService::resolve(const std::string& space, std::uint32_t address) {
  hw::AddressTarget info;
  inspect([&](rt::DebugContext& context) {
    info = context.debugger.resolve(space, address);
    if (info.kind != "cartridge") return;
    if (context.cartridge == nullptr) {
      info = {"none", "", 0, "没有插卡带，没有设备响应"};
      return;
    }
    using Kind = contracts::mappers::AddressInfo::Kind;
    const auto described = context.cartridge->describeAddress(space == "ppu", static_cast<std::uint16_t>(address));
    switch (described.kind) {
      case Kind::Storage: {
        if (!described.mapped) {
          info = {"none", "", 0, "卡带的窗口当前没有映射，没有设备响应"};
          return;
        }
        const hw::StorageRegion* region = context.cartridge->storage(described.storage);
        const bool ram = region != nullptr && region->kind() == hw::StorageKind::Ram;
        info = {ram ? "ram" : "rom", std::string(CART_PREFIX) + described.storage, described.offset, std::string("卡带的存储区 ") + described.storage + "，经窗口映射"};
        break;
      }
      case Kind::Register:
        info = {"register", "cart.mapper", 0, "mapper 脚本的寄存器，读写都会回调脚本"};
        break;
      case Kind::Nametable:
        info = {"ram", "cart.nametable", described.offset, "卡带自带的名称表显存（四屏）"};
        break;
      case Kind::None:
        info = {"none", "", 0, "卡带不响应这个地址，读到总线上最近的值"};
        break;
    }
  });
  return info;
}

std::vector<api::StateReport> LocalInspectorService::getState(const std::optional<std::string>& module) {
  std::vector<api::StateReport> states;
  inspect([&](rt::DebugContext& context) {
    for (const std::string& name : context.debugger.stateModules()) {
      if (module.has_value() && *module != name) continue;
      states.push_back({name, name == "cpu", context.debugger.readState(name)});
    }
    if (context.cartridge != nullptr && (!module.has_value() || *module == "mapper")) states.push_back(mapperState(*context.cartridge));
    if (module.has_value() && states.empty()) throw shared::CoreError(hw::ERROR_INVALID_STATE, "没有这个状态模块：" + *module);
  });
  return states;
}

api::StateReport LocalInspectorService::setState(const std::string& module, std::span<const hw::StateField> fields) {
  api::StateReport state;
  inspect([&](rt::DebugContext& context) {
    if (module == "mapper") throw shared::CoreError(hw::ERROR_SPACE_READ_ONLY, "mapper 的状态只读");
    context.debugger.writeState(module, fields);
    state = {module, module == "cpu", context.debugger.readState(module)};
  });
  return state;
}

hw::Breakpoint LocalInspectorService::addBreakpoint(hw::BreakpointKind kind, std::uint16_t first, std::uint16_t last) {
  hw::Breakpoint added;
  inspect([&](rt::DebugContext& context) { added = context.debugger.addBreakpoint(kind, first, last); });
  return added;
}

std::vector<hw::Breakpoint> LocalInspectorService::listBreakpoints() {
  std::vector<hw::Breakpoint> list;
  inspect([&](rt::DebugContext& context) { list = context.debugger.breakpoints(); });
  return list;
}

void LocalInspectorService::removeBreakpoint(std::uint32_t id) {
  inspect([&](rt::DebugContext& context) {
    if (!context.debugger.removeBreakpoint(id)) throw shared::CoreError(api::ERROR_NO_BREAKPOINT, "没有编号为 " + std::to_string(id) + " 的断点");
  });
}

std::size_t LocalInspectorService::clearBreakpoints() {
  std::size_t removed = 0;
  inspect([&](rt::DebugContext& context) { removed = context.debugger.clearBreakpoints(); });
  return removed;
}

}  // namespace ember::logic::inspector
