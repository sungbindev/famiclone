/*
 * ------------------------------------------------------------------------
 * 名称：mapper-chip.cpp
 * 说明：由插件创建的卡带的实现：存储区、窗口与译码表、名称表连法、脚本回调与宿主对象。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

#include "infra/mappers/mapper-chip.hpp"

#include <algorithm>
#include <span>
#include <utility>

#include <nlohmann/json.hpp>

#include "contracts/mappers/mapper-error.hpp"
#include "shared/errors/core-error.hpp"

namespace ember::infra::mappers {

namespace {

namespace hw = contracts::hardware;
namespace mapper_error = contracts::mappers;

/** 一条脚本日志的最大字节数，保留的最大条数。 */
constexpr std::size_t MAX_LOG_BYTES = 2000;
constexpr std::size_t MAX_LOG_ENTRIES = 200;
/** CPU 总线 64K，PPU 侧只有图案表 8K。 */
constexpr std::size_t CPU_SLOT_COUNT = 0x10000;
constexpr std::size_t PPU_SLOT_COUNT = 0x2000;

/** 文件头里的镜像方式对应的名称表连法。 */
NametableMode fromHeader(mapper_error::Mirroring mirroring) {
  switch (mirroring) {
    case mapper_error::Mirroring::Vertical:
      return NametableMode::Vertical;
    case mapper_error::Mirroring::FourScreen:
      return NametableMode::Four;
    case mapper_error::Mirroring::Horizontal:
      break;
  }
  return NametableMode::Horizontal;
}

/** ctx.header 的 JSON：规范化的卡带描述。 */
std::string headerJson(const mapper_error::CartridgeDescription& description) {
  return nlohmann::json{{"mapper", description.mapper},
                        {"variant", description.variant},
                        {"prgSize", description.prgSize},
                        {"chrSize", description.chrSize},
                        {"mirroring", std::string(mapper_error::mirroringName(description.mirroring))},
                        {"battery", description.battery},
                        {"trainer", description.trainer},
                        {"crc32Prg", description.crc32Prg},
                        {"crc32All", description.crc32All}}
      .dump();
}

}  // namespace

MapperChip::MapperChip(std::shared_ptr<const LoadedPlugin> plugin, const contracts::mappers::CartridgeImage& image, hw::IrqOutput& irq, hw::PadSink& pads, ScriptLimits limits)
    : plugin_(std::move(plugin)),
      irq_(irq),
      pads_(pads),
      tape_(pads.tapeLine()),
      padMasks_(pads.padPortCount(), 0),
      cpuSlots_(CPU_SLOT_COUNT),
      ppuSlots_(PPU_SLOT_COUNT),
      cpuSide_(*this, false),
      ppuSide_(*this, true) {
  const Manifest& manifest = plugin_->manifest;

  // 存储区：rom 从文件复制，来源为空且有后备时改用后备的 RAM；ram 清零。
  ScriptLayout layout;
  layout.headerJson = headerJson(image.description);
  layout.padPorts = padMasks_.size();
  layout.hasTape = tape_ != nullptr;
  for (const StorageDecl& decl : manifest.storage) {
    std::unique_ptr<StorageBlock> block;
    if (decl.ram) {
      block = std::make_unique<StorageBlock>(hw::StorageKind::Ram, std::vector<std::uint8_t>(decl.size, 0), decl.battery);
    } else {
      const std::vector<std::uint8_t>& source = decl.from == StorageSource::FilePrg ? image.prg : image.chr;
      if (source.empty() && decl.fallback) {
        block = std::make_unique<StorageBlock>(hw::StorageKind::Ram, std::vector<std::uint8_t>(decl.fallback->size, 0), decl.fallback->battery);
      } else {
        block = std::make_unique<StorageBlock>(hw::StorageKind::Rom, source, false);
      }
    }
    storages_.push_back(std::move(block));
    layout.storages.push_back(decl.id);
  }

  // 窗口与译码表。
  for (std::size_t index = 0; index < manifest.windows.size(); ++index) {
    const WindowDecl& decl = manifest.windows[index];
    const auto storage = std::find_if(manifest.storage.begin(), manifest.storage.end(), [&](const StorageDecl& item) { return item.id == decl.storage; });
    Window window;
    window.first = static_cast<std::uint16_t>(decl.range.first);
    window.size = decl.size;
    window.storage = storages_[static_cast<std::size_t>(storage - manifest.storage.begin())].get();
    window.writable = decl.writable;
    window.initialKind = decl.initialKind;
    window.initialBank = decl.initialBank;
    windows_.push_back(window);
    layout.windows.push_back(decl.id);
    std::vector<Slot>& slots = decl.ppu ? ppuSlots_ : cpuSlots_;
    for (std::uint32_t address = decl.range.first; address <= decl.range.last; ++address) slots[address].window = static_cast<std::int16_t>(index);
  }
  for (std::size_t index = 0; index < manifest.registers.size(); ++index) {
    const RegisterDecl& decl = manifest.registers[index];
    std::vector<Slot>& slots = decl.ppu ? ppuSlots_ : cpuSlots_;
    for (std::uint32_t address = decl.range.first; address <= decl.range.last; ++address) {
      slots[address].reg = static_cast<std::int16_t>(index);
      slots[address].readRegister = decl.read;
      slots[address].writeRegister = decl.write;
    }
  }

  // 状态与名称表。
  for (const StateDecl& decl : manifest.state) {
    layout.states.push_back(decl.name);
    stateValues_.push_back(0);
    stateMasks_.push_back(decl.bits >= 32 ? 0xFFFFFFFFU : (std::uint32_t{1} << decl.bits) - 1U);
  }
  initialNametable_ = manifest.nametable == NametableMode::Header ? fromHeader(image.description.mirroring) : manifest.nametable;
  nametable_ = initialNametable_;
  for (Window& window : windows_) applyInitial(window);

  // 清单声明的寄存器、查表与磁带计时：把状态名、窗口标识、表标识解析成下标，访问时不再查名字。
  const auto stateIndex = [&](const std::string& name) {
    return static_cast<std::size_t>(std::find_if(manifest.state.begin(), manifest.state.end(), [&](const StateDecl& item) { return item.name == name; }) - manifest.state.begin());
  };
  const auto windowIndex = [&](const std::string& id) {
    return static_cast<std::size_t>(std::find_if(manifest.windows.begin(), manifest.windows.end(), [&](const WindowDecl& item) { return item.id == id; }) - manifest.windows.begin());
  };
  const auto tableIndex = [&](const std::string& id) {
    return static_cast<std::size_t>(std::find_if(manifest.tables.begin(), manifest.tables.end(), [&](const TableDecl& item) { return item.id == id; }) - manifest.tables.begin());
  };
  for (const TableDecl& decl : manifest.tables) {
    Table table;
    table.id = decl.id;
    for (const IndexSource& source : decl.index) table.sources.push_back(TableSource{source.from, source.from == IndexFrom::State ? stateIndex(source.state) : 0, source.shift, source.bits});
    table.valueMask = decl.bits >= 32 ? 0xFFFFFFFFU : (std::uint32_t{1} << decl.bits) - 1U;
    table.values.assign(std::size_t{1} << decl.indexBits, 0);
    table.known.assign(std::size_t{1} << decl.indexBits, 0);
    tables_.push_back(std::move(table));
  }
  for (const RegisterDecl& decl : manifest.registers) {
    ResolvedRegister resolved;
    resolved.readKind = decl.readAction.kind;
    resolved.readValue = decl.readAction.value;
    if (decl.readAction.kind == ReadKind::State) resolved.readState = stateIndex(decl.readAction.state);
    resolved.releaseIrq = decl.readAction.releaseIrq;
    resolved.writeScript = decl.writeAction.script;
    if (!decl.writeAction.state.empty()) resolved.writeState = stateIndex(decl.writeAction.state);
    for (const BankSetter& setter : decl.writeAction.banks) resolved.banks.push_back(ResolvedRegister::Bank{windowIndex(setter.window), setter.shift, setter.mask});
    if (!decl.writeAction.updateTable.empty()) {
      resolved.table = tableIndex(decl.writeAction.updateTable);
      resolved.into = stateIndex(decl.writeAction.updateInto);
    }
    registers_.push_back(std::move(resolved));
  }
  if (manifest.tape && manifest.tape->timer) {
    timerSamples_ = stateIndex(manifest.tape->timer->samples);
    timerTicks_ = stateIndex(manifest.tape->timer->ticks);
  }

  // 脚本：清单里有要回调脚本的寄存器或要脚本计算的查表时必须有脚本并导出对应的函数；只有声明式寄存器的卡带可以没有脚本。
  const bool needsScript = manifest.needsReadHandler() || manifest.needsWriteHandler() || manifest.needsTables();
  if (!plugin_->script.empty() || needsScript) {
    if (plugin_->script.empty()) throw shared::CoreError(mapper_error::ERROR_INVALID_PACKAGE, "插件 " + manifest.id + " 声明了要回调脚本的寄存器或查表，但包里没有 main.js");
    ScriptBindings& bindings = *this;
    script_ = std::make_unique<ScriptRuntime>(manifest.id, plugin_->script, std::move(layout), bindings, limits);
    if (manifest.needsReadHandler() && !script_->hasRead()) throw shared::CoreError(mapper_error::ERROR_INVALID_PACKAGE, "插件 " + manifest.id + " 声明了读寄存器，但 main.js 没有导出 read");
    if (manifest.needsWriteHandler() && !script_->hasWrite()) throw shared::CoreError(mapper_error::ERROR_INVALID_PACKAGE, "插件 " + manifest.id + " 声明了写寄存器，但 main.js 没有导出 write");
    for (const Table& table : tables_) {
      if (!script_->hasTable(table.id)) throw shared::CoreError(mapper_error::ERROR_INVALID_PACKAGE, "插件 " + manifest.id + " 声明了查表 " + table.id + "，但 main.js 没有导出 tables." + table.id);
    }
  }
  // 磁带数据线的翻转在翻转的时刻通知卡带（清单声明的边沿计时与脚本的 onTapeEdge）；主机没有磁带机时不挂。
  if (tape_ != nullptr && (timerSamples_.has_value() || (script_ != nullptr && script_->hasTapeEdge()))) {
    tapeListening_ = true;
    tape_->setEdgeListener([this](bool level, std::uint32_t samples) { onTapeEdge(level, samples); });
  }
}

MapperChip::~MapperChip() {
  if (tapeListening_) tape_->setEdgeListener({});
}

template <typename Call>
bool MapperChip::runScript(Call&& call) {
  if (failure_ || script_ == nullptr) return false;
  try {
    call();
    return true;
  } catch (const shared::CoreError& error) {
    failure_ = contracts::mappers::ScriptFailure{error.code(), plugin_->manifest.id, error.what()};
    return false;
  }
}

hw::CiramControl MapperChip::ciramControl(std::uint16_t ppuAddress) const {
  switch (nametable_) {
    case NametableMode::Horizontal:
      return hw::CiramControl{true, (ppuAddress & 0x800U) != 0};
    case NametableMode::Vertical:
      return hw::CiramControl{true, (ppuAddress & 0x400U) != 0};
    case NametableMode::Single0:
      return hw::CiramControl{true, false};
    case NametableMode::Single1:
      return hw::CiramControl{true, true};
    case NametableMode::Four:
      return hw::CiramControl{false, false};
    case NametableMode::Header:
      break;
  }
  return hw::CiramControl{true, false};
}

void MapperChip::reset(bool power) {
  failure_.reset();
  if (power) powerCycle();
  applyDeclaredReset();
  if (script_ != nullptr && script_->hasReset()) runScript([&] { script_->reset(power); });
}

void MapperChip::applyDeclaredReset() {
  const Manifest& manifest = plugin_->manifest;
  for (std::size_t index = 0; index < manifest.state.size(); ++index) {
    if (manifest.state[index].initial) stateValues_[index] = *manifest.state[index].initial;
  }
  if (!manifest.tape) return;
  if (manifest.tape->timer && manifest.tape->timer->irq) irq_.set(false);
  if (tape_ != nullptr && manifest.tape->comparator) {
    const TapeComparatorDecl& comparator = *manifest.tape->comparator;
    tape_->setComparator(hw::TapeComparator{static_cast<std::int16_t>(comparator.low), static_cast<std::int16_t>(comparator.high), comparator.invert});
  }
}

void MapperChip::input(const hw::InputEvent& event) {
  if (script_ == nullptr) return;
  if (const auto* key = std::get_if<hw::KeyInput>(&event)) {
    // 物理键的状态先更新，脚本里 ctx.keys.isDown 读到的就是这次事件之后的状态。
    if (key->down) heldKeys_.insert(key->code);
    else heldKeys_.erase(key->code);
    if (script_->hasKey()) runScript([&] { script_->key(*key); });
  } else if (const auto* mouse = std::get_if<hw::MouseInput>(&event)) {
    if (script_->hasMouse()) runScript([&] { script_->mouse(*mouse); });
  }
}

std::vector<contracts::mappers::StorageSummary> MapperChip::storageList() const {
  std::vector<contracts::mappers::StorageSummary> list;
  const auto& decls = plugin_->manifest.storage;
  for (std::size_t index = 0; index < decls.size(); ++index) {
    const StorageBlock& block = *storages_[index];
    list.push_back({decls[index].id, block.kind(), block.size(), block.battery()});
  }
  return list;
}

const hw::StorageRegion* MapperChip::storage(std::string_view id) const {
  const auto& decls = plugin_->manifest.storage;
  const auto found = std::find_if(decls.begin(), decls.end(), [&](const StorageDecl& item) { return item.id == id; });
  return found == decls.end() ? nullptr : storages_[static_cast<std::size_t>(found - decls.begin())].get();
}

bool MapperChip::pokeStorage(std::string_view id, std::size_t offset, std::uint8_t value) {
  const auto& decls = plugin_->manifest.storage;
  const auto found = std::find_if(decls.begin(), decls.end(), [&](const StorageDecl& item) { return item.id == id; });
  if (found == decls.end()) return false;
  StorageBlock& block = *storages_[static_cast<std::size_t>(found - decls.begin())];
  return offset < block.size() && block.write(offset, value);
}

std::vector<std::string> MapperChip::stateNames() const {
  std::vector<std::string> names;
  for (const StateDecl& decl : plugin_->manifest.state) names.push_back(decl.name);
  return names;
}

std::optional<std::uint32_t> MapperChip::state(std::string_view name) const {
  const auto& decls = plugin_->manifest.state;
  const auto found = std::find_if(decls.begin(), decls.end(), [&](const StateDecl& item) { return item.name == name; });
  if (found == decls.end()) return std::nullopt;
  return stateValues_[static_cast<std::size_t>(found - decls.begin())];
}

// ---------------------------------------------------------------- 总线访问

hw::BusRead MapperChip::Side::read(std::uint16_t address) { return owner_.readBus(ppu_, address); }

hw::BusRead MapperChip::Side::peek(std::uint16_t address) { return owner_.peekBus(ppu_, address); }

hw::BusRead MapperChip::peekBus(bool ppu, std::uint16_t address) const {
  if (ppu && address >= PPU_SLOT_COUNT) return nametable_ == NametableMode::Four ? hw::driveBus(ownNametable_[address & 0xFFFU]) : hw::releaseBus();
  const Slot slot = ppu ? ppuSlots_[address] : cpuSlots_[address];
  if (slot.readRegister) {
    // 调试读不能有副作用：只有声明的固定值与状态可以直接给出，回调脚本的读不触发。
    const ResolvedRegister& decl = registers_[static_cast<std::size_t>(slot.reg)];
    if (decl.readKind == ReadKind::Value) return hw::driveBus(decl.readValue);
    if (decl.readKind == ReadKind::State) return hw::driveBus(static_cast<std::uint8_t>(stateValues_[decl.readState]));
    return hw::releaseBus();
  }
  return slot.window >= 0 ? readWindow(windows_[static_cast<std::size_t>(slot.window)], address) : hw::releaseBus();
}

contracts::mappers::AddressInfo MapperChip::describeAddress(bool ppu, std::uint16_t address) const {
  using Info = contracts::mappers::AddressInfo;
  Info info;
  if (ppu && address >= PPU_SLOT_COUNT) {
    info.kind = nametable_ == NametableMode::Four ? Info::Kind::Nametable : Info::Kind::None;
    info.offset = address & 0xFFFU;
    return info;
  }
  const Slot slot = ppu ? ppuSlots_[address] : cpuSlots_[address];
  if (slot.readRegister || slot.writeRegister) {
    info.kind = Info::Kind::Register;
    return info;
  }
  if (slot.window < 0) return info;
  const Window& window = windows_[static_cast<std::size_t>(slot.window)];
  const std::size_t capacity = window.storage->size();
  info.mapped = window.mapped && capacity != 0;
  if (!info.mapped) return info;
  std::size_t position = window.offset + (address - window.first);
  if (position >= capacity) position %= capacity;
  info.kind = Info::Kind::Storage;
  info.offset = position;
  const auto& decls = plugin_->manifest.storage;
  for (std::size_t index = 0; index < decls.size(); ++index) {
    if (storages_[index].get() == window.storage) info.storage = decls[index].id;
  }
  return info;
}

void MapperChip::Side::write(std::uint16_t address, std::uint8_t value) { owner_.writeBus(ppu_, address, value); }

hw::BusRead MapperChip::readBus(bool ppu, std::uint16_t address) {
  // 名称表地址只在四屏时由卡带响应；图案表地址按窗口译码。
  if (ppu && address >= PPU_SLOT_COUNT) return nametable_ == NametableMode::Four ? hw::driveBus(ownNametable_[address & 0xFFFU]) : hw::releaseBus();
  const Slot slot = ppu ? ppuSlots_[address] : cpuSlots_[address];
  if (slot.readRegister) return readRegister(ppu, address, slot.reg);
  return slot.window >= 0 ? readWindow(windows_[static_cast<std::size_t>(slot.window)], address) : hw::releaseBus();
}

hw::BusRead MapperChip::readRegister(bool ppu, std::uint16_t address, std::int16_t index) {
  const ResolvedRegister& decl = registers_[static_cast<std::size_t>(index)];
  hw::BusRead result = hw::releaseBus();
  switch (decl.readKind) {
    case ReadKind::Script: {
      std::optional<std::uint8_t> value;
      if (runScript([&] { value = script_->read(ppu, address); }) && value) result = hw::driveBus(*value);
      break;
    }
    case ReadKind::Value:
      result = hw::driveBus(decl.readValue);
      break;
    case ReadKind::State:
      result = hw::driveBus(static_cast<std::uint8_t>(stateValues_[decl.readState]));
      break;
    case ReadKind::Open:
    case ReadKind::None:
      break;
  }
  if (decl.releaseIrq) irq_.set(false);
  return result;
}

void MapperChip::writeBus(bool ppu, std::uint16_t address, std::uint8_t value) {
  if (ppu && address >= PPU_SLOT_COUNT) {
    if (nametable_ == NametableMode::Four) ownNametable_[address & 0xFFFU] = value;
    return;
  }
  const Slot slot = ppu ? ppuSlots_[address] : cpuSlots_[address];
  if (slot.writeRegister) writeRegister(ppu, address, value, slot.reg);
  else if (slot.window >= 0) writeWindow(windows_[static_cast<std::size_t>(slot.window)], address, value);
}

void MapperChip::writeRegister(bool ppu, std::uint16_t address, std::uint8_t value, std::int16_t index) {
  const ResolvedRegister& decl = registers_[static_cast<std::size_t>(index)];
  if (decl.writeScript) {
    runScript([&] { script_->write(ppu, address, value); });
    return;
  }
  if (decl.writeState) stateSet(*decl.writeState, value);
  for (const ResolvedRegister::Bank& bank : decl.banks) windowSetBank(bank.window, (value >> bank.shift) & bank.mask);
  if (decl.table) {
    if (const auto result = lookup(*decl.table, address, value)) stateSet(decl.into, *result);
  }
}

std::optional<std::uint32_t> MapperChip::lookup(std::size_t tableNumber, std::uint16_t address, std::uint8_t value) {
  Table& table = tables_[tableNumber];
  std::array<std::uint32_t, MAX_INDEX_SOURCES> parts{};
  std::size_t index = 0;
  for (std::size_t position = 0; position < table.sources.size(); ++position) {
    const TableSource& source = table.sources[position];
    std::uint32_t part = 0;
    switch (source.from) {
      case IndexFrom::State:
        part = stateValues_[source.state];
        break;
      case IndexFrom::Address:
        part = address >> source.shift;
        break;
      case IndexFrom::Data:
        part = static_cast<std::uint32_t>(value) >> source.shift;
        break;
      case IndexFrom::Tape:
        part = tape_ != nullptr && tape_->level() ? 1U : 0U;
        break;
    }
    part &= (std::uint32_t{1} << source.bits) - 1U;
    parts[position] = part;
    index = (index << source.bits) | part;
  }
  if (table.known[index] != 0) return table.values[index];
  // 第一次遇到这个索引：调脚本计算一次并缓存（脚本的 tables 函数必须是纯函数）。
  std::optional<std::uint32_t> computed;
  if (!runScript([&] { computed = script_->table(table.id, std::span<const std::uint32_t>(parts.data(), table.sources.size())); }) || !computed) return std::nullopt;
  table.values[index] = *computed & table.valueMask;
  table.known[index] = 1;
  return table.values[index];
}

void MapperChip::onTapeEdge(bool level, std::uint32_t samples) {
  if (timerSamples_) {
    const TapeTimerDecl& timer = *plugin_->manifest.tape->timer;
    stateSet(*timerSamples_, static_cast<std::uint32_t>(std::uint64_t{stateValues_[*timerSamples_]} + samples));
    if (!level) {
      // 下降沿：把累加的采样数折成计时加进 ticks，清零采样数。
      const std::uint64_t ticks = std::uint64_t{stateValues_[*timerSamples_]} * timer.scaleNumerator / timer.scaleDenominator;
      stateSet(timerTicks_, static_cast<std::uint32_t>(std::uint64_t{stateValues_[timerTicks_]} + ticks));
      stateSet(*timerSamples_, 0);
      if (timer.irq) irq_.set(true);
    }
  }
  if (script_ != nullptr && script_->hasTapeEdge()) runScript([&] { script_->tapeEdge(level, samples); });
}

hw::BusRead MapperChip::readWindow(const Window& window, std::uint16_t address) const {
  const std::size_t capacity = window.storage->size();
  if (!window.mapped || capacity == 0) return hw::releaseBus();
  std::size_t position = window.offset + (address - window.first);
  if (position >= capacity) position %= capacity;
  return hw::driveBus(window.storage->byteAt(position));
}

void MapperChip::writeWindow(const Window& window, std::uint16_t address, std::uint8_t value) const {
  const std::size_t capacity = window.storage->size();
  if (!window.mapped || !window.writable || capacity == 0) return;
  std::size_t position = window.offset + (address - window.first);
  if (position >= capacity) position %= capacity;
  window.storage->writeAt(position, value);
}

void MapperChip::applyInitial(Window& window) const {
  const std::size_t capacity = window.storage->size();
  const std::size_t banks = std::max<std::size_t>(1, capacity / window.size);
  std::size_t bank = 0;
  if (window.initialKind == InitialKind::Last) bank = banks - 1;
  else if (window.initialKind == InitialKind::Bank) bank = window.initialBank % banks;
  window.offset = bank * window.size;
  window.mapped = true;
}

void MapperChip::powerCycle() {
  for (auto& block : storages_) block->powerCycle();
  for (Window& window : windows_) applyInitial(window);
  std::fill(stateValues_.begin(), stateValues_.end(), 0U);
  nametable_ = initialNametable_;
  irq_.set(false);
}

// ---------------------------------------------------------------- 宿主对象

void MapperChip::windowSetBank(std::size_t window, std::int64_t bank) {
  if (bank < 0) throw ScriptArgumentError("bank 不能为负数");
  Window& target = windows_.at(window);
  const std::size_t banks = std::max<std::size_t>(1, target.storage->size() / target.size);
  target.offset = static_cast<std::size_t>(static_cast<std::uint64_t>(bank) % banks) * target.size;
  target.mapped = true;
}

void MapperChip::windowSetOffset(std::size_t window, std::int64_t offset) {
  if (offset < 0) throw ScriptArgumentError("偏移不能为负数");
  Window& target = windows_.at(window);
  const std::size_t capacity = target.storage->size();
  target.offset = capacity == 0 ? 0 : static_cast<std::size_t>(static_cast<std::uint64_t>(offset) % capacity);
  target.mapped = true;
}

void MapperChip::windowUnmap(std::size_t window) { windows_.at(window).mapped = false; }

void MapperChip::nametableSet(std::string_view mode) {
  if (mode == "horizontal") nametable_ = NametableMode::Horizontal;
  else if (mode == "vertical") nametable_ = NametableMode::Vertical;
  else if (mode == "single0") nametable_ = NametableMode::Single0;
  else if (mode == "single1") nametable_ = NametableMode::Single1;
  else if (mode == "four") nametable_ = NametableMode::Four;
  else throw ScriptArgumentError("名称表连法必须是 horizontal、vertical、single0、single1 或 four");
}

std::uint8_t MapperChip::storageRead(std::size_t storage, std::int64_t offset) {
  const StorageBlock& block = *storages_.at(storage);
  if (offset < 0 || static_cast<std::uint64_t>(offset) >= block.size()) throw ScriptArgumentError("存储区的偏移越界");
  return block.byteAt(static_cast<std::size_t>(offset));
}

bool MapperChip::storageWrite(std::size_t storage, std::int64_t offset, std::uint8_t value) {
  StorageBlock& block = *storages_.at(storage);
  if (offset < 0 || static_cast<std::uint64_t>(offset) >= block.size()) throw ScriptArgumentError("存储区的偏移越界");
  return block.writeAt(static_cast<std::size_t>(offset), value);
}

void MapperChip::log(std::string_view message) {
  if (message.size() > MAX_LOG_BYTES) {
    // 截断时退到 UTF-8 字符的边界，不留半个字符。
    std::size_t end = MAX_LOG_BYTES;
    while (end > 0 && (static_cast<unsigned char>(message[end]) & 0xC0U) == 0x80U) --end;
    message = message.substr(0, end);
  }
  log_.emplace_back(message);
  if (log_.size() > MAX_LOG_ENTRIES) log_.erase(log_.begin());
}

void MapperChip::padSet(std::size_t port, std::string_view button, bool down) {
  std::uint8_t bit = 0;
  if (button == "a") bit = hw::pad_button::A;
  else if (button == "b") bit = hw::pad_button::B;
  else if (button == "select") bit = hw::pad_button::SELECT;
  else if (button == "start") bit = hw::pad_button::START;
  else if (button == "up") bit = hw::pad_button::UP;
  else if (button == "down") bit = hw::pad_button::DOWN;
  else if (button == "left") bit = hw::pad_button::LEFT;
  else if (button == "right") bit = hw::pad_button::RIGHT;
  else throw ScriptArgumentError("按键必须是 a、b、select、start、up、down、left 或 right");
  std::uint8_t& mask = padMasks_.at(port);
  mask = down ? static_cast<std::uint8_t>(mask | bit) : static_cast<std::uint8_t>(mask & ~bit);
  pads_.setButtons(static_cast<unsigned>(port), mask);
}

void MapperChip::padRelease(std::size_t port) {
  padMasks_.at(port) = 0;
  pads_.setButtons(static_cast<unsigned>(port), 0);
}

bool MapperChip::tapeLevel() {
  if (tape_ == nullptr) throw ScriptArgumentError("当前主机没有磁带机");
  return tape_->level();
}

void MapperChip::tapeSetComparator(std::int16_t low, std::int16_t high, bool invert) {
  if (tape_ == nullptr) throw ScriptArgumentError("当前主机没有磁带机");
  tape_->setComparator(hw::TapeComparator{low, high, invert});
}

}  // namespace ember::infra::mappers
