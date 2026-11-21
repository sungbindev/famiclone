/*
 * ------------------------------------------------------------------------
 * 名称：media-panel-content.cpp
 * 说明：设备面板内容的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "ui/media/media-panel-content.hpp"

#include <memory>
#include <string>

#include "rujian/ui/layout.hpp"

namespace ember::win32::ui {

namespace {

/** 定时读取状态的间隔（毫秒）。 */
constexpr unsigned kPollMilliseconds = 500;
/** 设备名列的宽度（逻辑像素，含图标）；刚好放下图标与“磁带机”“打印机”三个字。 */
constexpr double kLeadingWidth = 60;
/** 磁带位置列的宽度（逻辑像素）；能放下“28:09 / 28:09”（约 68）。 */
constexpr double kPositionWidth = 72;
/** 状态列的宽度（逻辑像素）；能放下最长的“读写中（磁道 79）”（约 100）。 */
constexpr double kStateWidth = 108;

/** 行尾按钮组里占位用的空白：宽度始终等于参照控件，让软驱行的按钮组与磁带机行等宽，两行的状态列才左边对齐。 */
class Spacer final : public rujian::ui::Widget {
 public:
  void setReference(rujian::ui::Widget* reference) { reference_ = reference; }
  [[nodiscard]] rujian::ui::Size measure(rujian::ui::Size available) override { return reference_ != nullptr ? reference_->measure(available) : rujian::ui::Size{}; }
  [[nodiscard]] bool stretchesWidth() const override { return false; }
  [[nodiscard]] bool stretchesHeight() const override { return false; }

 private:
  rujian::ui::Widget* reference_{nullptr};
};

/** 行尾的只有图标的按钮：名称在悬停提示里。 */
rujian::ui::Button* addIconButton(rujian::ui::Widget& parent, const char* label, rujian::ui::IconId icon, std::function<void()> onClick) {
  auto button = std::make_unique<rujian::ui::Button>(label);
  button->setVariant(rujian::ui::ButtonVariant::Ghost);
  button->setIconOnly(true);
  button->setCompact(true);
  button->setIcon(icon);
  button->setTooltip(label);
  button->setOnClick(std::move(onClick));
  return static_cast<rujian::ui::Button*>(parent.addChild(std::move(button)));
}

}  // namespace

MediaPanelContent::MediaPanelContent(contracts::MediaModel& model) : model_(model) {
  using rujian::ui::IconId;
  auto list = std::make_unique<rujian::ui::RowList>();
  const contracts::MediaView view = model_.view();
  for (const contracts::MediaFloppyRow& row : view.floppies) {
    const int drive = row.drive;
    auto item = std::make_unique<rujian::ui::RowListItem>();
    item->setLeading({row.label, IconId::DeviceFloppy, kLeadingWidth, true});
    item->setTitle(row.disk);
    item->addCell({row.state, kStateWidth, true});
    auto actions = std::make_unique<rujian::ui::Actions>();
    FloppyWidgets widgets;
    // 软驱比磁带机少一个按钮，前面补一个按钮宽的空白，使状态列与磁带机的状态列对齐
    Spacer* spacer = static_cast<Spacer*>(actions->addChild(std::make_unique<Spacer>()));
    widgets.writeProtect = addIconButton(*actions, "写保护", IconId::LockOpen, [this, drive] {
      for (const auto& floppy : model_.view().floppies) {
        if (floppy.drive == drive) model_.setWriteProtected(drive, !floppy.writeProtected);
      }
    });
    spacer->setReference(widgets.writeProtect);
    widgets.insert = addIconButton(*actions, "载入镜像", IconId::FolderOpen, [this, drive] { model_.insertFloppy(drive); });
    widgets.eject = addIconButton(*actions, "取出", IconId::PlayerEject, [this, drive] { model_.ejectFloppy(drive); });
    item->setActions(std::move(actions));
    widgets.item = static_cast<rujian::ui::RowListItem*>(list->addChild(std::move(item)));
    floppies_.push_back(widgets);
  }

  auto tape = std::make_unique<rujian::ui::RowListItem>();
  tape->setLeading({"磁带机", IconId::DeviceAudioTape, kLeadingWidth, true});
  tape->setTitle(view.tape.disk);
  tape->addCell({view.tape.position, kPositionWidth, true});
  tape->addCell({view.tape.state, kStateWidth, true});
  auto tapeActions = std::make_unique<rujian::ui::Actions>();
  tape_.play = addIconButton(*tapeActions, "播放", IconId::PlayerPlay, [this] { model_.playTape(); });
  tape_.record = addIconButton(*tapeActions, "录音", IconId::PlayerRecord, [this] { model_.recordTape(); });
  tape_.stop = addIconButton(*tapeActions, "停止", IconId::PlayerStop, [this] { model_.stopTape(); });
  tape_.eject = addIconButton(*tapeActions, "取出", IconId::PlayerEject, [this] { model_.ejectTape(); });
  tape->setActions(std::move(tapeActions));
  tape_.item = static_cast<rujian::ui::RowListItem*>(list->addChild(std::move(tape)));

  auto printer = std::make_unique<rujian::ui::RowListItem>();
  printer->setLeading({"打印机", IconId::Printer, kLeadingWidth, true});
  printer->setTitle(view.printer);
  printer_ = static_cast<rujian::ui::RowListItem*>(list->addChild(std::move(printer)));

  list_ = static_cast<rujian::ui::RowList*>(addChild(std::move(list)));
  refresh();
  unsubscribe_ = model_.subscribe([this] { refresh(); });
  // 展开时立刻读一次，之后定时读
  model_.refresh();
}

MediaPanelContent::~MediaPanelContent() {
  if (unsubscribe_) unsubscribe_();
}

rujian::ui::Size MediaPanelContent::measure(rujian::ui::Size available) { return list_->measure(available); }

void MediaPanelContent::arrange(const rujian::ui::Rect& bounds) {
  Widget::arrange(bounds);
  scheduleRefresh();
}

void MediaPanelContent::scheduleRefresh() {
  if (timer_ == 0 && host() != nullptr) timer_ = host()->startTimer(this, kPollMilliseconds);
}

void MediaPanelContent::onTimer(unsigned id) {
  if (id != timer_) return;
  timer_ = 0;
  model_.refresh();
  scheduleRefresh();
}

void MediaPanelContent::refresh() {
  using rujian::ui::IconId;
  const contracts::MediaView view = model_.view();
  for (std::size_t i = 0; i < floppies_.size() && i < view.floppies.size(); ++i) {
    const contracts::MediaFloppyRow& row = view.floppies[i];
    const FloppyWidgets& widgets = floppies_[i];
    widgets.item->setTitle(row.disk);
    widgets.item->setCellText(0, row.state);
    widgets.writeProtect->setIcon(row.writeProtected ? IconId::Lock : IconId::LockOpen);
    widgets.writeProtect->setToggled(row.writeProtected);
    widgets.writeProtect->setEnabled(row.canWriteProtect);
    widgets.insert->setEnabled(row.canInsert);
    widgets.eject->setEnabled(row.canEject);
  }
  tape_.item->setTitle(view.tape.disk);
  tape_.item->setCellText(0, view.tape.position);
  tape_.item->setCellText(1, view.tape.state);
  tape_.eject->setEnabled(view.tape.canEject);
  tape_.play->setEnabled(view.tape.canPlay);
  tape_.record->setEnabled(view.tape.canRecord);
  tape_.stop->setEnabled(view.tape.canStop);
  printer_->setTitle(view.printer);
}

}  // namespace ember::win32::ui
