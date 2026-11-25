/*
 * ------------------------------------------------------------------------
 * 名称：media-panel-content.hpp
 * 说明：底部面板“设备”的内容：紧凑的信息行，每个设备一行，没有边框与表格线，鼠标悬停时整行变底色。软驱一行依次是名称（带图标）、盘片、状态、行尾的四个图标按钮（磁盘镜像管理、写保护、载入镜像、取出）；磁带机一行是名称（带图标）、磁带、位置、状态与行尾的四个图标按钮（播放、录音、停止、取出）；打印机一行是名称（带图标）与状态。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：规则见 docs/05-界面.md 第 4.3 节；文字与按钮可用状态来自模型；展开期间每 500 毫秒让模型重新读取设备状态，面板收起时内容卸载、定时读取随之停止；
 *       状态列定宽，软驱行有四个按钮、磁带机行也有四个，所以磁带机与软驱的状态左边对齐、取出按钮在同一位置；按钮只有图标，名称在悬停提示里；写保护是按下态的图标按钮（锁）。
 *       磁带机的位置列可点击（里面有磁带且不在录音时）：在这一列上方打开时间轴弹层，弹层里的内容随每次刷新同步，松开鼠标时把定位位置交给模型；面板收起、内容卸载时弹层随之关闭。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "contracts/media/media-model.hpp"
#include "rujian/ui/button.hpp"
#include "rujian/ui/row-list.hpp"
#include "rujian/ui/timeline-popup.hpp"
#include "rujian/ui/widget.hpp"

namespace ember::win32::ui {

/** 设备面板的内容。 */
class MediaPanelContent final : public rujian::ui::Widget {
 public:
  /** @param model 介质模块的模型，必须比本控件活得久。
   *  @param openDiskManager 打开“磁盘镜像管理”对话框（软驱行的第一个按钮）；不给时这个按钮不可用。 */
  explicit MediaPanelContent(contracts::MediaModel& model, std::function<void()> openDiskManager = {});
  ~MediaPanelContent() override;

  [[nodiscard]] rujian::ui::Size measure(rujian::ui::Size available) override;
  void arrange(const rujian::ui::Rect& bounds) override;
  void onTimer(unsigned id) override;

  /** 软驱一行里的控件（测试用）。 */
  struct FloppyWidgets {
    rujian::ui::RowListItem* item{nullptr};
    rujian::ui::Button* manager{nullptr};
    rujian::ui::Button* writeProtect{nullptr};
    rujian::ui::Button* insert{nullptr};
    rujian::ui::Button* eject{nullptr};
  };
  /** 磁带机一行里的控件（测试用）。 */
  struct TapeWidgets {
    rujian::ui::RowListItem* item{nullptr};
    rujian::ui::Button* eject{nullptr};
    rujian::ui::Button* play{nullptr};
    rujian::ui::Button* record{nullptr};
    rujian::ui::Button* stop{nullptr};
  };
  [[nodiscard]] const std::vector<FloppyWidgets>& floppies() const { return floppies_; }
  [[nodiscard]] const TapeWidgets& tape() const { return tape_; }
  [[nodiscard]] rujian::ui::RowListItem& printerRow() { return *printer_; }
  /** 磁带时间轴弹层（测试用）；还没有打开过时为空。 */
  [[nodiscard]] rujian::ui::TimelinePopup* timelinePopup() const { return timeline_.get(); }

 private:
  void refresh();
  void scheduleRefresh();
  /** 点击磁带位置列：弹层没有打开就打开，已打开就关闭。 */
  void toggleTimeline(const rujian::ui::Rect& anchor);
  /** 把模型里的时间轴内容同步到已打开的弹层；不再可用（取出磁带、开始录音）时关闭它。 */
  void syncTimeline();

  contracts::MediaModel& model_;
  std::function<void()> openDiskManager_;
  rujian::ui::RowList* list_{nullptr};
  std::vector<FloppyWidgets> floppies_;
  TapeWidgets tape_;
  rujian::ui::RowListItem* printer_{nullptr};
  unsigned timer_{0};
  std::function<void()> unsubscribe_;
  std::unique_ptr<rujian::ui::TimelinePopup> timeline_;
  /** 弹层里已设置的包络属于哪盘磁带、是否已分析完：变了才重新设置，不在每次刷新时复制整份包络。 */
  std::int64_t shownSerial_{-1};
  bool shownReady_{false};
};

}  // namespace ember::win32::ui
