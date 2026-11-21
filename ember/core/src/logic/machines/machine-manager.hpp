/*
 * ------------------------------------------------------------------------
 * 名称：machine-manager.hpp
 * 说明：机型服务的实现：机型清单、ROM 文件的校验与保存、整机机型开机（把 ROM 交给运行服务装配主机）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：清单里的机型顺序固定；机型的“已支持”以程序里实际有这个主机型号为准（目前 fc、sb97、sb2000、bbk10、bbk20、pec9588、pec586 与 yuxing）；
 *       ROM 文件是只含程序的纯文件，大小等于机型的 PRG 容量；也接受带 iNES 文件头的文件（16 字节文件头，可选的 512 字节训练器，之后是 PRG）。保存时保持文件原样，开机时才取出 PRG 交给主机。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "contracts/machines/rom-storage.hpp"
#include "contracts/machines/machine-service.hpp"
#include "contracts/runtime/runtime-service.hpp"
#include "platform/events/event-sink.hpp"

namespace ember::logic::machines {

/** 机型服务。 */
class MachineManager final : public contracts::machines::MachineService {
 public:
  MachineManager(std::shared_ptr<contracts::machines::RomStorage> storage, std::shared_ptr<contracts::runtime::RuntimeService> runtime, std::shared_ptr<platform::EventSink> events);

  [[nodiscard]] contracts::machines::MachinesListResult list() const override;
  contracts::machines::MachinesSetRomResult setRom(const contracts::machines::MachinesSetRomParams& params) override;
  contracts::machines::MachinesRemoveRomResult removeRom(const contracts::machines::MachinesRemoveRomParams& params) override;
  contracts::machines::MachinesBootResult boot(const contracts::machines::MachinesBootParams& params) override;

 private:
  /** 清单里的一个机型。 */
  struct Entry {
    std::string id;
    std::string name;
    bool needsRom;
    bool supported;
    /** 需要 ROM 的机型：PRG 的字节数。 */
    std::size_t prgSize;
  };

  [[nodiscard]] const Entry& entry(const std::string& id) const;
  [[nodiscard]] contracts::machines::MachineInfo info(const Entry& item) const;
  /** 取出机型的 ROM（PRG）并检查；@throws shared::CoreError machines.romInvalid。 */
  [[nodiscard]] static std::vector<std::uint8_t> extractRom(const Entry& item, const std::vector<std::uint8_t>& file);

  std::shared_ptr<contracts::machines::RomStorage> storage_;
  std::shared_ptr<contracts::runtime::RuntimeService> runtime_;
  std::shared_ptr<platform::EventSink> events_;
  std::vector<Entry> entries_;
};

}  // namespace ember::logic::machines
