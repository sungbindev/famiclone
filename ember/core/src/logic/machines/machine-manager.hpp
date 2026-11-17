/*
 * ------------------------------------------------------------------------
 * 名称：machine-manager.hpp
 * 说明：机型服务的实现：机型清单、BIOS 文件的校验与保存、整机机型开机（把 BIOS 交给运行服务装配主机）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：清单里的机型顺序固定；机型的“已支持”以程序里实际有这个主机型号为准（目前 fc、sb97、sb2000、bbk10 与 bbk20）；
 *       BIOS 文件是只含程序的纯文件，大小等于机型的 PRG 容量；也接受带 iNES 文件头的文件（16 字节文件头，可选的 512 字节训练器，之后是 PRG）。保存时保持文件原样，开机时才取出 PRG 交给主机。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "contracts/machines/bios-storage.hpp"
#include "contracts/machines/machine-service.hpp"
#include "contracts/runtime/runtime-service.hpp"
#include "platform/events/event-sink.hpp"

namespace ember::logic::machines {

/** 机型服务。 */
class MachineManager final : public contracts::machines::MachineService {
 public:
  MachineManager(std::shared_ptr<contracts::machines::BiosStorage> storage, std::shared_ptr<contracts::runtime::RuntimeService> runtime, std::shared_ptr<platform::EventSink> events);

  [[nodiscard]] contracts::machines::MachinesListResult list() const override;
  contracts::machines::MachinesSetBiosResult setBios(const contracts::machines::MachinesSetBiosParams& params) override;
  contracts::machines::MachinesRemoveBiosResult removeBios(const contracts::machines::MachinesRemoveBiosParams& params) override;
  contracts::machines::MachinesBootResult boot(const contracts::machines::MachinesBootParams& params) override;

 private:
  /** 清单里的一个机型。 */
  struct Entry {
    std::string id;
    std::string name;
    bool needsBios;
    bool supported;
    /** 需要 BIOS 的机型：PRG 的字节数。 */
    std::size_t prgSize;
  };

  [[nodiscard]] const Entry& entry(const std::string& id) const;
  [[nodiscard]] contracts::machines::MachineInfo info(const Entry& item) const;
  /** 取出机型的 BIOS（PRG）并检查；@throws shared::CoreError machines.biosInvalid。 */
  [[nodiscard]] static std::vector<std::uint8_t> extractBios(const Entry& item, const std::vector<std::uint8_t>& file);

  std::shared_ptr<contracts::machines::BiosStorage> storage_;
  std::shared_ptr<contracts::runtime::RuntimeService> runtime_;
  std::shared_ptr<platform::EventSink> events_;
  std::vector<Entry> entries_;
};

}  // namespace ember::logic::machines
