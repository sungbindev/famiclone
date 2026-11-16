/*
 * ------------------------------------------------------------------------
 * 名称：media-manager.hpp
 * 说明：介质服务的实现：软驱盘片的插入、取出、保存、状态与故障注入，磁带机的载入、取出、播放、录音与状态。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-16
 * 备注：镜像文件的读取在请求线程完成，不碰硬件状态；软驱与磁带机都是独立于主机的设备，不经主机找到，直接用共享的那一台；
 *       它们的状态只在模拟线程上读写，所以都经 runOnSimulation（不要求有主机、不要求通电）；
 *       盘片镜像由驱动器持有，是唯一的数据；学习机写盘只改内存里的镜像，由本服务的后台线程自动写回文件：
 *       每个间隔看一次驱动器的修改序号，有未写回的修改且序号与上一次相同（写盘已停下）时，在模拟线程上复制镜像，在后台线程写文件，
 *       写完再在模拟线程上确认序号没变才清除“已修改”（期间又被改过则留给下一轮）；写回失败时发 media.floppySaveFailed（成功前只发一次）并写错误日志，之后每 5 秒重试；
 *       取出与换盘前、核心停止（shutdown）时先把未写回的修改写回，保存文件的动作用一把锁串行。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "contracts/hardware/floppy-activity.hpp"
#include "contracts/hardware/floppy-drive.hpp"
#include "contracts/hardware/tape-deck.hpp"
#include "contracts/media/floppy-image-store.hpp"
#include "contracts/media/media-service.hpp"
#include "contracts/media/printer-record.hpp"
#include "contracts/media/tape-store.hpp"
#include "contracts/runtime/runtime-service.hpp"
#include "platform/events/event-sink.hpp"
#include "platform/logging/error-log.hpp"

namespace ember::logic::media {

/** 介质服务；同时接收软驱马达的开关通知（变化时发 media.changed，软驱状态里的马达字段随之变化）。 */
class MediaManager final : public contracts::media::MediaService, public contracts::hardware::FloppyActivity {
 public:
  /**
   * @param deck 磁带机：独立于主机的设备，与主机型号目录共用同一台。
   * @param floppyDrives 软驱（按编号排列）：同样是独立于主机的设备，与主机型号目录共用。
   * @param errors 写回失败时写错误日志；为空时不写。
   * @param autoSaveInterval 自动写回的检查间隔；为 0 时不启动后台线程（取出、换盘与停止时的写回仍然有效）。
   */
  MediaManager(std::shared_ptr<contracts::media::FloppyImageStore> store, std::shared_ptr<contracts::media::TapeStore> tapeStore, std::shared_ptr<contracts::runtime::RuntimeService> runtime,
               std::shared_ptr<platform::EventSink> events, std::shared_ptr<contracts::media::PrinterRecord> printerRecord, std::shared_ptr<contracts::hardware::TapeControl> deck,
               std::vector<std::shared_ptr<contracts::hardware::FloppyDrive>> floppyDrives, std::shared_ptr<platform::ErrorLog> errors = nullptr,
               std::chrono::milliseconds autoSaveInterval = std::chrono::milliseconds(0));
  ~MediaManager() override;

  MediaManager(const MediaManager&) = delete;
  MediaManager& operator=(const MediaManager&) = delete;

  /** 停止后台线程并把所有未写回的修改写回；在运行服务停止之前调用（核心停止时），可重复调用。 */
  void shutdown();

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
   * 在模拟线程上取驱动器并执行动作，返回动作做完后的驱动器状态；不需要主机。
   * @throws shared::CoreError media.noDrive，以及动作里抛出的错误。
   */
  template <typename Action>
  contracts::media::FloppyStatus withDrive(std::int64_t drive, Action action);

  /** 发 media.changed。 */
  void publishChanged(std::int64_t drive);

  /**
   * 把驱动器里的镜像写回放入时的文件并清除标志。在模拟线程上调用。
   * @throws shared::CoreError media.saveFailed。
   */
  void saveImage(std::size_t index);

  /** 驱动器里有未写回的修改时写回，否则什么都不做。在模拟线程上调用。 */
  void writeBack(std::size_t index);

  /** 后台线程为每个驱动器记住的状态。 */
  struct AutoSaveState {
    /** 上一次检查时的修改序号。 */
    std::uint64_t seenRevision{0};
    /** 上一次写回是否失败（失败后只通知一次）。 */
    bool failed{false};
    /** 失败后最早的下一次重试时刻。 */
    std::chrono::steady_clock::time_point retryAt{};
  };

  /** 后台线程的主循环。 */
  void autoSaveLoop();

  /** 后台线程检查并写回一个驱动器。 */
  void autoSaveDrive(std::size_t index, AutoSaveState& state);

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
  std::vector<std::shared_ptr<contracts::hardware::FloppyDrive>> drives_;
  std::shared_ptr<platform::ErrorLog> errors_;
  std::chrono::milliseconds autoSaveInterval_;
  /** 保存镜像文件的动作串行（后台线程、取出与换盘、立即保存）。 */
  std::mutex saveMutex_;
  /** 每个驱动器已经写回文件的最新修改序号（由 saveMutex_ 保护）。 */
  std::vector<std::uint64_t> writtenRevisions_;
  std::mutex wakeMutex_;
  std::condition_variable wake_;
  bool stopping_{false};
  std::thread autoSaveThread_;
};

}  // namespace ember::logic::media
