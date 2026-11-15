/*
 * ------------------------------------------------------------------------
 * 名称：machine-catalog.hpp
 * 说明：主机型号目录：按型号名创建主机；有 fc（经典 FC，要卡带）与 sb2000（小霸王 2000 学习机，要 BIOS）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：增加主机型号时在这里登记，同时在 mappers 模块的主机型号目录里登记（bootstrap/modules/mappers.cpp）。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <memory>

#include "contracts/hardware/floppy-activity.hpp"
#include "contracts/hardware/printer-output.hpp"
#include "contracts/runtime/machine-factory.hpp"
#include "logic/hardware/apu-2a03/apu-2a03.hpp"

namespace ember::bootstrap {

/** 程序里已有的主机型号。 */
class MachineCatalog final : public contracts::runtime::MachineFactory {
 public:
  /** 全部主机型号输出声音的采样率（赫兹）；声音输出按它打开设备。 */
  static constexpr unsigned AUDIO_SAMPLE_RATE = logic::hardware::Apu2a03::SAMPLE_RATE;

  /** @param printerOutput 整机机型的打印机把输出的页交给它；为空时主机不接打印机。 */
  explicit MachineCatalog(std::shared_ptr<contracts::hardware::PrinterOutput> printerOutput = nullptr, std::shared_ptr<contracts::hardware::FloppyActivity> floppyActivity = nullptr)
      : printerOutput_(std::move(printerOutput)), floppyActivity_(std::move(floppyActivity)) {}

  [[nodiscard]] std::unique_ptr<contracts::hardware::Machine> create(const std::string& host) const override;
  [[nodiscard]] std::unique_ptr<contracts::hardware::Machine> createWithBios(const std::string& machine, std::vector<std::uint8_t> bios) const override;

 private:
  std::shared_ptr<contracts::hardware::PrinterOutput> printerOutput_;
  std::shared_ptr<contracts::hardware::FloppyActivity> floppyActivity_;
};

}  // namespace ember::bootstrap
