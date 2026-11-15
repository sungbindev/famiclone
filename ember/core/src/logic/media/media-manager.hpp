/*
 * ------------------------------------------------------------------------
 * 名称：media-manager.hpp
 * 说明：介质服务的实现：软驱盘片的插入、取出、保存、状态与故障注入，磁带机的载入、取出、播放、录音与状态。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-16
 * 备注：镜像文件的读取在请求线程完成，不碰硬件状态；对驱动器的操作经运行服务在模拟线程上执行（不要求通电，盘片与电源无关）；
 *       磁带机是独立的设备，不经主机找到，直接用共享的那一台（状态只在模拟线程上读写，所以也经 runOnSimulation）；
 *       盘片镜像由驱动器持有，是唯一的数据；保存才写回文件，不自动落盘。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <memory>
#include <string>

#include "contracts/hardware/floppy-activity.hpp"
#include "contracts/hardware/floppy-drive.hpp"
#include "contracts/hardware/tape-deck.hpp"
#include "contracts/media/floppy-image-store.hpp"
#include "contracts/media/media-service.hpp"
#include "contracts/media/printer-record.hpp"
#include "contracts/media/tape-store.hpp"
#include "contracts/runtime/runtime-service.hpp"
#include "platform/events/event-sink.hpp"

namespace ember::logic::media {

/** 介质服务；同时接收软驱马达的开关通知（变化时发 media.changed，软驱状态里的马达字段随之变化）。 */
class MediaManager final : public contracts::media::MediaService, public contracts::hardware::FloppyActivity {
 public:
  /**
   * @param deck 磁带机：独立于主机的设备，与主机型号目录共用同一台。
   */
  MediaManager(std::shared_ptr<contracts::media::FloppyImageStore> store, std::shared_ptr<contracts::media::TapeStore> tapeStore, std::shared_ptr<contracts::runtime::RuntimeService> runtime,
               std::shared_ptr<platform::EventSink> events, std::shared_ptr<contracts::media::PrinterRecord> printerRecord, std::shared_ptr<contracts::hardware::TapeControl> deck);

  contracts::media::MediaFloppyInsertResult floppyInsert(const contracts::media::MediaFloppyInsertParams& params) override;
  contracts::media::MediaFloppyEjectResult floppyEject(const contracts::media::MediaFloppyEjectParams& params) override;
  contracts::media::MediaFloppySaveResult floppySave(const contracts::media::MediaFloppySaveParams& params) override;
  contracts::media::MediaFloppyWriteProtectResult floppyWriteProtect(const contracts::media::MediaFloppyWriteProtectParams& params) override;
  [[nodiscard]] contracts::media::MediaFloppyStatusResult floppyStatus() override;
  contracts::media::MediaFloppyFaultResult floppyFault(const contracts::media::MediaFloppyFaultParams& params) override;
  [[nodiscard]] contracts::media::MediaPrinterStatusResult printerStatus() override;
  contracts::media::MediaPrinterFlushResult printerFlush() override;
  contracts::media::MediaTapeLoadResult tapeLoad(const contracts::media::MediaTapeLoadParams& params) override;
  contracts::media::MediaTapeEjectResult tapeEject() override;
  contracts::media::MediaTapePlayResult tapePlay() override;
  contracts::media::MediaTapeRecordResult tapeRecord(const contracts::media::MediaTapeRecordParams& params) override;
  contracts::media::MediaTapeStopResult tapeStop() override;
  [[nodiscard]] contracts::media::MediaTapeStatusResult tapeStatus() override;
  void motorChanged(unsigned drive, bool on) override;

 private:
  /**
   * 在模拟线程上取驱动器并执行动作，返回动作做完后的驱动器状态。
   * @throws shared::CoreError media.noMachine、media.noDrive，以及动作里抛出的错误。
   */
  template <typename Action>
  contracts::media::FloppyStatus withDrive(std::int64_t drive, Action action);

  /** 发 media.changed。 */
  void publishChanged(std::int64_t drive);

  /**
   * 在模拟线程上用磁带机执行动作，返回动作做完后的磁带机状态；不需要主机。
   * @throws 动作里抛出的错误。
   */
  template <typename Action>
  contracts::media::TapeStatus withDeck(Action action);

  /** 发磁带机的 media.changed。 */
  void publishTapeChanged();

  std::shared_ptr<contracts::media::FloppyImageStore> store_;
  std::shared_ptr<contracts::media::TapeStore> tapeStore_;
  std::shared_ptr<contracts::runtime::RuntimeService> runtime_;
  std::shared_ptr<platform::EventSink> events_;
  std::shared_ptr<contracts::media::PrinterRecord> printerRecord_;
  std::shared_ptr<contracts::hardware::TapeControl> deck_;
};

}  // namespace ember::logic::media
