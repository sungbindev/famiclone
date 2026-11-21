/*
 * ------------------------------------------------------------------------
 * 名称：media-panel-content.hpp
 * 说明：底部面板“设备”的内容：紧凑的信息行，每个设备一行，没有边框与表格线，鼠标悬停时整行变底色。软驱一行依次是名称（带图标）、盘片、状态、行尾的三个图标按钮（写保护、载入镜像、取出）；磁带机一行是名称（带图标）、磁带、位置、状态与行尾的四个图标按钮（播放、录音、停止、取出）；打印机一行是名称（带图标）与状态。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：规则见 docs/05-界面.md 第 4.3 节；文字与按钮可用状态来自模型；展开期间每 500 毫秒让模型重新读取设备状态，面板收起时内容卸载、定时读取随之停止；
 *       状态列定宽，软驱行的按钮组前补一个按钮宽的空白，所以磁带机与软驱的状态左边对齐、取出按钮在同一位置；按钮只有图标，名称在悬停提示里；写保护是按下态的图标按钮（锁）。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <vector>

#include "contracts/media/media-model.hpp"
#include "rujian/ui/button.hpp"
#include "rujian/ui/row-list.hpp"
#include "rujian/ui/widget.hpp"

namespace ember::win32::ui {

/** 设备面板的内容。 */
class MediaPanelContent final : public rujian::ui::Widget {
 public:
  /** @param model 介质模块的模型，必须比本控件活得久。 */
  explicit MediaPanelContent(contracts::MediaModel& model);
  ~MediaPanelContent() override;

  [[nodiscard]] rujian::ui::Size measure(rujian::ui::Size available) override;
  void arrange(const rujian::ui::Rect& bounds) override;
  void onTimer(unsigned id) override;

  /** 软驱一行里的控件（测试用）。 */
  struct FloppyWidgets {
    rujian::ui::RowListItem* item{nullptr};
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

 private:
  void refresh();
  void scheduleRefresh();

  contracts::MediaModel& model_;
  rujian::ui::RowList* list_{nullptr};
  std::vector<FloppyWidgets> floppies_;
  TapeWidgets tape_;
  rujian::ui::RowListItem* printer_{nullptr};
  unsigned timer_{0};
  std::function<void()> unsubscribe_;
};

}  // namespace ember::win32::ui
