/*
 * ------------------------------------------------------------------------
 * 名称：media-controller.hpp
 * 说明：介质模块的控制器：保存各软驱、磁带机与打印机的状态，执行载入镜像、取出、写保护、磁带的取出、播放（没有磁带时先选磁带）、录音与停止等操作，订阅核心事件与核心运行状态。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：状态变化时通知订阅者并让外壳重算命令与重画状态栏（软驱读写灯与软盘名取自这里的状态），所以设备面板没有展开时状态也是最新的；
 *       学习机写盘后核心会自动把镜像写回文件，界面没有保存操作；自动写回失败（media.floppySaveFailed）时用通知给出原因；
 *       软驱的读写状态与磁道、磁带的位置、打印机的状态随机器运行不断变化，核心不推送，所以设备面板展开期间由界面经 refresh 定时读取，面板没有展开时不读取；
 *       失败时用通知提示；打印机每输出一页，用通知告知文件名，通知里带“打开图片目录”链接（规则见 docs/05-界面.md 第 4.3 节）。
 *       磁带的时间轴（声音大小与空白段）由核心在后台分析：读到磁带后、磁带的序号变了或还在分析中（读进度）时读取，收到 media.tapeAnalyzed 时立即再读；没有磁带时清空。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "contracts/core-gateway/core-gateway.hpp"
#include "contracts/media/media-model.hpp"
#include "contracts/shell/request-broker.hpp"
#include "contracts/shell/shell-registry.hpp"
#include "logic/media/media-rules.hpp"

namespace ember::win32::logic {

/** 介质模块的控制器。 */
class MediaController final : public contracts::MediaModel {
 public:
  /**
   * @param broker 请求中转：核心的请求与外壳自己处理的请求（载入镜像、载入磁带、选择录音文件）都经它。
   * @param core 核心网关：订阅核心事件与核心运行状态。
   */
  MediaController(contracts::RequestBroker& broker, contracts::CoreGateway& core);
  ~MediaController() override;
  MediaController(const MediaController&) = delete;
  MediaController& operator=(const MediaController&) = delete;

  /**
   * 接上外壳：订阅核心事件与核心运行状态，并读取初始状态。
   * @returns 断开的函数。
   */
  std::function<void()> attach(contracts::ShellApi& shell);

  // ---- 状态 ----
  /** 各软驱的状态，按编号排列；还没读到、核心不可用或主机没有软驱时为空。 */
  [[nodiscard]] const std::vector<FloppyState>& floppies() const { return floppies_; }
  /** 磁带机的状态；还没读到时没有值。 */
  [[nodiscard]] const std::optional<TapeState>& tape() const { return tape_; }

  // ---- 操作 ----
  /** 把拖入窗口的软盘镜像载入软驱（编号来自外壳登记的拖入文件），不弹文件选择窗口。 */
  void insertDroppedFloppy(int drive, std::int64_t droppedId);
  /** 把拖入窗口的声音文件载入磁带机（只载入，不自动播放）。 */
  void loadDroppedTape(std::int64_t droppedId);

  // ---- MediaModel ----
  [[nodiscard]] contracts::MediaView view() const override;
  void insertFloppy(int drive) override;
  void ejectFloppy(int drive) override;
  void setWriteProtected(int drive, bool writeProtected) override;
  void ejectTape() override;
  void playTape() override;
  void recordTape() override;
  void stopTape() override;
  void seekTape(double seconds) override;
  [[nodiscard]] contracts::MediaTimelineView timelineView() const override;
  void refresh() override { refreshAll(); }
  [[nodiscard]] std::function<void()> subscribe(std::function<void()> handler) override;

 private:
  /** 执行一个请求：失败时用通知提示；结束后重新读取状态。 */
  void perform(const std::string& action, const std::string& request, nlohmann::json params);
  void report(const std::string& action, const std::string& reason);
  void handleEvent(const std::string& name, const nlohmann::json& data);
  /** 用文件管理器打开打印机图片的保存目录；失败时用通知提示。 */
  void openPrinterDirectory();
  void handleCoreStatus(const contracts::CoreStatus& status);
  void refreshAll();
  void refreshFloppies();
  void refreshPrinter();
  void refreshTape();
  /** 读取磁带的时间轴（分析结果由核心在后台算出，结果出来时核心发 media.tapeAnalyzed）。 */
  void refreshTimeline();
  void clear();
  void changed();
  /** 请求：回复只在仍接着外壳时处理。 */
  void ask(const std::string& name, nlohmann::json params, std::function<void(const contracts::CoreReply&)> done);

  contracts::RequestBroker& broker_;
  contracts::CoreGateway& core_;
  contracts::ShellApi* shell_{nullptr};
  std::vector<std::function<void()>> stops_;
  /** 控制器存在期间为 true：回复到达时控制器已经销毁就不处理。 */
  std::shared_ptr<bool> alive_{std::make_shared<bool>(true)};
  /** 每次接上外壳加一：上一次接上时发出的请求，回复到了也不处理。 */
  std::uint64_t generation_{0};

  std::vector<FloppyState> floppies_;
  std::optional<PrinterMode> printer_;
  std::optional<TapeState> tape_;
  /** 磁带的时间轴；没有磁带、还没读到时没有值，内容里的序号与磁带的序号不同时说明是上一盘的。 */
  std::optional<TimelineState> timeline_;
  /** 最近一次读取的编号：迟到的旧结果不覆盖新结果。 */
  std::uint64_t floppyRequest_{0};
  std::uint64_t printerRequest_{0};
  std::uint64_t tapeRequest_{0};
  std::uint64_t timelineRequest_{0};
  std::map<int, std::function<void()>> listeners_;
  int nextListener_{1};
};

}  // namespace ember::win32::logic
