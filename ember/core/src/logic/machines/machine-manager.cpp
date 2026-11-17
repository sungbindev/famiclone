/*
 * ------------------------------------------------------------------------
 * 名称：machine-manager.cpp
 * 说明：机型服务的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * ------------------------------------------------------------------------
 */

#include "logic/machines/machine-manager.hpp"

#include <utility>

#include "contracts/machines/machines-error.hpp"
#include "shared/errors/core-error.hpp"

namespace ember::logic::machines {

namespace api = contracts::machines;

namespace {

/** iNES 文件头的大小。 */
constexpr std::size_t HEADER_SIZE = 16;
/** 训练器的大小。 */
constexpr std::size_t TRAINER_SIZE = 512;
/** PRG 的单位（字节）。 */
constexpr std::size_t PRG_UNIT = 16 * 1024;
/** sb97 的 BIOS：1024KB 的 PRG。 */
constexpr std::size_t SB97_PRG_SIZE = 1024 * 1024;
/** sb2000 的 BIOS：512KB 的 PRG。 */
constexpr std::size_t SB2000_PRG_SIZE = 512 * 1024;
/** 步步高软驱一号与步步高 98 的 BIOS：128KB、2048KB 的 PRG。 */
constexpr std::size_t BBK10_PRG_SIZE = 128 * 1024;
constexpr std::size_t BBK20_PRG_SIZE = 2048 * 1024;

/** 路径里最后一段（文件名）；两种分隔符都认。 */
std::string baseName(const std::string& path) {
  const auto position = path.find_last_of("/\\");
  return position == std::string::npos ? path : path.substr(position + 1);
}

}  // namespace

MachineManager::MachineManager(std::shared_ptr<api::BiosStorage> storage, std::shared_ptr<contracts::runtime::RuntimeService> runtime, std::shared_ptr<platform::EventSink> events)
    : storage_(std::move(storage)),
      runtime_(std::move(runtime)),
      events_(std::move(events)),
      entries_{{"fc", "经典 FC", false, true, 0},
               {"sb97", "小霸王 97", true, true, SB97_PRG_SIZE},
               {"sb2000", "小霸王 2000", true, true, SB2000_PRG_SIZE},
               {"bbk10", "步步高软驱一号", true, true, BBK10_PRG_SIZE},
               {"bbk20", "步步高 98", true, true, BBK20_PRG_SIZE}} {}

const MachineManager::Entry& MachineManager::entry(const std::string& id) const {
  for (const Entry& candidate : entries_) {
    if (candidate.id == id) return candidate;
  }
  throw shared::CoreError(api::ERROR_NOT_FOUND, "没有这个机型：" + id);
}

api::MachineInfo MachineManager::info(const Entry& item) const {
  api::MachineInfo result;
  result.id = item.id;
  result.name = item.name;
  result.needsBios = item.needsBios;
  result.supported = item.supported;
  if (item.needsBios) {
    if (const auto stored = storage_->find(item.id)) result.bios = api::BiosFile{stored->fileName, stored->size};
  }
  return result;
}

std::vector<std::uint8_t> MachineManager::extractBios(const Entry& item, const std::vector<std::uint8_t>& file) {
  const bool hasHeader = file.size() >= HEADER_SIZE && file[0] == 'N' && file[1] == 'E' && file[2] == 'S' && file[3] == 0x1A;
  if (!hasHeader) {
    if (file.size() != item.prgSize) {
      throw shared::CoreError(api::ERROR_BIOS_INVALID, item.name + " 的 BIOS 必须是 " + std::to_string(item.prgSize / 1024) + "KB 的 BIOS 文件，这个文件是 " + std::to_string(file.size()) + " 字节");
    }
    return file;
  }
  const std::size_t prgSize = static_cast<std::size_t>(file[4]) * PRG_UNIT;
  if (prgSize != item.prgSize) {
    throw shared::CoreError(api::ERROR_BIOS_INVALID, item.name + " 的 BIOS 的 PRG 必须是 " + std::to_string(item.prgSize / 1024) + "KB，这个文件是 " + std::to_string(prgSize / 1024) + "KB");
  }
  const std::size_t start = HEADER_SIZE + ((file[6] & 0x04) != 0 ? TRAINER_SIZE : 0);
  if (file.size() < start + prgSize) throw shared::CoreError(api::ERROR_BIOS_INVALID, "BIOS 文件被截断，比文件头声明的内容短");
  return std::vector<std::uint8_t>(file.begin() + static_cast<std::ptrdiff_t>(start), file.begin() + static_cast<std::ptrdiff_t>(start + prgSize));
}

api::MachinesListResult MachineManager::list() const {
  api::MachinesListResult result;
  for (const Entry& candidate : entries_) result.machines.push_back(info(candidate));
  return result;
}

api::MachinesSetBiosResult MachineManager::setBios(const api::MachinesSetBiosParams& params) {
  const Entry& target = entry(params.machine);
  if (!target.needsBios) throw shared::CoreError(api::ERROR_BIOS_NOT_NEEDED, target.name + " 不需要 BIOS");
  if (!target.supported) throw shared::CoreError(api::ERROR_UNSUPPORTED, target.name + " 暂未支持");
  const std::vector<std::uint8_t> file = storage_->readSource(params.path);
  static_cast<void>(extractBios(target, file));
  storage_->store(target.id, baseName(params.path), file);
  events_->publish(api::MACHINES_CHANGED_EVENT, nlohmann::json(api::MachinesChangedData{target.id}));
  return api::MachinesSetBiosResult{info(target)};
}

api::MachinesRemoveBiosResult MachineManager::removeBios(const api::MachinesRemoveBiosParams& params) {
  const Entry& target = entry(params.machine);
  if (!target.needsBios) throw shared::CoreError(api::ERROR_BIOS_NOT_NEEDED, target.name + " 不需要 BIOS");
  const bool had = storage_->find(target.id).has_value();
  storage_->remove(target.id);
  if (had) events_->publish(api::MACHINES_CHANGED_EVENT, nlohmann::json(api::MachinesChangedData{target.id}));
  return api::MachinesRemoveBiosResult{info(target)};
}

api::MachinesBootResult MachineManager::boot(const api::MachinesBootParams& params) {
  const Entry& target = entry(params.machine);
  if (!target.needsBios) throw shared::CoreError(api::ERROR_CARTRIDGE_REQUIRED, target.name + " 开机必须有 NES 文件，请改用打开卡带");
  if (!target.supported) throw shared::CoreError(api::ERROR_UNSUPPORTED, target.name + " 暂未支持");
  if (!storage_->find(target.id)) throw shared::CoreError(api::ERROR_NO_BIOS, target.name + " 还没有添加 BIOS，请先在首选项的“机型与 BIOS”里添加");
  contracts::runtime::MachineBoot request;
  request.machine = target.id;
  request.bios = extractBios(target, storage_->load(target.id));
  request.powerOn = params.powerOn.value_or(true);
  request.resume = params.resume.value_or(true);
  const contracts::runtime::RuntimeState state = runtime_->bootMachine(std::move(request));
  return api::MachinesBootResult{info(target), state};
}

}  // namespace ember::logic::machines
