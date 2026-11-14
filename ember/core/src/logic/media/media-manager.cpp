/*
 * ------------------------------------------------------------------------
 * 名称：media-manager.cpp
 * 说明：介质服务的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * ------------------------------------------------------------------------
 */

#include "logic/media/media-manager.hpp"

#include <algorithm>
#include <optional>
#include <utility>

#include "contracts/media/media-error.hpp"
#include "shared/errors/core-error.hpp"

namespace ember::logic::media {

namespace api = contracts::media;
namespace hw = contracts::hardware;

namespace {

/** 路径里最后一段（文件名）；两种分隔符都认。 */
std::string baseName(const std::string& path) {
  const auto position = path.find_last_of("/\\");
  return position == std::string::npos ? path : path.substr(position + 1);
}

/** 驱动器当前的状态。 */
api::FloppyStatus statusOf(std::int64_t index, const hw::FloppyDrive& drive) {
  api::FloppyStatus status;
  status.drive = index;
  status.inserted = drive.inserted();
  if (drive.inserted() || drive.mediaSuspended()) status.fileName = baseName(drive.source());
  status.writeProtected = drive.writeProtected();
  status.modified = drive.modified();
  status.size = static_cast<std::int64_t>(drive.image().size());
  status.cylinder = drive.cylinder();
  status.motor = drive.motorRunning();
  status.faults.ejected = drive.mediaSuspended();
  status.faults.noIndex = drive.faults().noIndex;
  status.faults.seekFail = drive.faults().seekFail;
  status.faults.unresponsive = drive.faults().unresponsive;
  for (const int sector : drive.faults().badSectors) status.faults.badSectors.push_back(sector);
  return status;
}

}  // namespace

MediaManager::MediaManager(std::shared_ptr<api::FloppyImageStore> store, std::shared_ptr<contracts::runtime::RuntimeService> runtime, std::shared_ptr<platform::EventSink> events,
                           std::shared_ptr<api::PrinterRecord> printerRecord)
    : store_(std::move(store)), runtime_(std::move(runtime)), events_(std::move(events)), printerRecord_(std::move(printerRecord)) {}

template <typename Action>
api::FloppyStatus MediaManager::withDrive(std::int64_t drive, Action action) {
  std::optional<api::FloppyStatus> status;
  bool missingDrive = false;
  const bool hasMachine = runtime_->useMachine([&](hw::Machine& machine) {
    hw::FloppyDrive* unit = drive >= 0 && drive < static_cast<std::int64_t>(machine.floppyDriveCount()) ? machine.floppyDrive(static_cast<unsigned>(drive)) : nullptr;
    if (unit == nullptr) {
      missingDrive = true;
      return;
    }
    action(*unit);
    status = statusOf(drive, *unit);
  });
  if (!hasMachine) throw shared::CoreError(api::ERROR_NO_MACHINE, "还没有主机，请先打开卡带或用整机机型开机");
  if (missingDrive || !status) throw shared::CoreError(api::ERROR_NO_DRIVE, "这台主机没有软驱 " + std::to_string(drive));
  return *status;
}

void MediaManager::publishChanged(std::int64_t drive) { events_->publish(api::MEDIA_CHANGED_EVENT, nlohmann::json(api::MediaChangedData{"floppy", drive})); }

api::MediaFloppyInsertResult MediaManager::floppyInsert(const api::MediaFloppyInsertParams& params) {
  std::vector<std::uint8_t> image = store_->read(params.path);
  const bool writeProtected = params.writeProtected.value_or(false);
  auto status = withDrive(params.drive, [&](hw::FloppyDrive& drive) { drive.insertImage(std::move(image), writeProtected, params.path); });
  publishChanged(params.drive);
  return api::MediaFloppyInsertResult{std::move(status)};
}

api::MediaFloppyEjectResult MediaManager::floppyEject(const api::MediaFloppyEjectParams& params) {
  auto status = withDrive(params.drive, [](hw::FloppyDrive& drive) { drive.ejectImage(); });
  publishChanged(params.drive);
  return api::MediaFloppyEjectResult{std::move(status)};
}

api::MediaFloppySaveResult MediaManager::floppySave(const api::MediaFloppySaveParams& params) {
  auto status = withDrive(params.drive, [&](hw::FloppyDrive& drive) {
    if (!drive.inserted() && !drive.mediaSuspended()) throw shared::CoreError(api::ERROR_NO_DISK, "驱动器里没有盘片");
    store_->write(drive.source(), drive.image());
    drive.markSaved();
  });
  publishChanged(params.drive);
  return api::MediaFloppySaveResult{std::move(status)};
}

api::MediaFloppyWriteProtectResult MediaManager::floppyWriteProtect(const api::MediaFloppyWriteProtectParams& params) {
  auto status = withDrive(params.drive, [&](hw::FloppyDrive& drive) { drive.setWriteProtected(params.writeProtected); });
  publishChanged(params.drive);
  return api::MediaFloppyWriteProtectResult{std::move(status)};
}

api::MediaFloppyStatusResult MediaManager::floppyStatus() {
  api::MediaFloppyStatusResult result;
  runtime_->useMachine([&](hw::Machine& machine) {
    for (unsigned index = 0; index < machine.floppyDriveCount(); ++index) {
      if (const hw::FloppyDrive* unit = machine.floppyDrive(index)) result.drives.push_back(statusOf(index, *unit));
    }
  });
  return result;
}

api::MediaFloppyFaultResult MediaManager::floppyFault(const api::MediaFloppyFaultParams& params) {
  const bool enabled = params.enabled.value_or(true);
  if (params.fault == "badSector" && !params.sector) throw shared::CoreError(api::ERROR_SECTOR_REQUIRED, "指定扇区校验错必须给出扇区号");
  auto status = withDrive(params.drive, [&](hw::FloppyDrive& drive) {
    hw::FloppyFaults& faults = drive.mutableFaults();
    if (params.fault == "ejected") {
      if (enabled) {
        drive.suspendMedia();
      } else {
        drive.restoreMedia();
      }
    } else if (params.fault == "writeProtected") {
      drive.setWriteProtected(enabled);
    } else if (params.fault == "noIndex") {
      faults.noIndex = enabled;
    } else if (params.fault == "seekFail") {
      faults.seekFail = enabled;
    } else if (params.fault == "unresponsive") {
      faults.unresponsive = enabled;
    } else if (params.fault == "badSector") {
      const int sector = static_cast<int>(*params.sector);
      auto& bad = faults.badSectors;
      const auto found = std::find(bad.begin(), bad.end(), sector);
      if (enabled && found == bad.end()) bad.push_back(sector);
      if (!enabled && found != bad.end()) bad.erase(found);
    } else {
      // clear：关闭全部故障，放回盘片，取消写保护。
      faults = hw::FloppyFaults{};
      drive.restoreMedia();
      drive.setWriteProtected(false);
    }
  });
  publishChanged(params.drive);
  return api::MediaFloppyFaultResult{std::move(status)};
}

api::MediaPrinterStatusResult MediaManager::printerStatus() {
  const api::PrinterRecordSnapshot record = printerRecord_->snapshot();
  api::PrinterStatus status;
  status.state = "idle";
  status.pages = record.pages;
  if (!record.lastFile.empty()) status.lastFile = record.lastFile;
  runtime_->useMachine([&](hw::Machine& machine) {
    const hw::PrinterControl* printer = machine.printer();
    if (printer == nullptr || !machine.isPoweredOn()) return;
    status.hasContent = printer->pageHasContent();
    if (status.hasContent) {
      status.state = "printing";
    } else if (printer->pagesOutput() > 0) {
      status.state = "done";
    }
  });
  return api::MediaPrinterStatusResult{std::move(status)};
}

api::MediaPrinterFlushResult MediaManager::printerFlush() {
  bool missingPrinter = false;
  bool empty = false;
  const bool hasMachine = runtime_->useMachine([&](hw::Machine& machine) {
    hw::PrinterControl* printer = machine.printer();
    if (printer == nullptr) {
      missingPrinter = true;
    } else if (!printer->pageHasContent()) {
      empty = true;
    } else {
      printer->flushPage();
    }
  });
  if (!hasMachine) throw shared::CoreError(api::ERROR_NO_MACHINE, "还没有主机，请先打开卡带或用整机机型开机");
  if (missingPrinter) throw shared::CoreError(api::ERROR_NO_PRINTER, "这台主机没有打印机");
  if (empty) throw shared::CoreError(api::ERROR_PRINTER_EMPTY, "打印机当前页上没有内容");
  return api::MediaPrinterFlushResult{printerStatus().status};
}

}  // namespace ember::logic::media
