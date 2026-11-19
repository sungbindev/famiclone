/*
 * ------------------------------------------------------------------------
 * 名称：runtime-panel-content.cpp
 * 说明：运行面板内容的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "ui/runtime/runtime-panel-content.hpp"

#include <memory>
#include <string>

#include "rujian/ui/layout.hpp"

namespace ember::win32::ui {

namespace {

/** 运行面板里音量滑块的宽度（逻辑像素）。 */
constexpr double kVolumeSliderWidth = 80;

/** 创建一个按钮并加进容器。 */
rujian::ui::Button* addButton(rujian::ui::Widget& parent, const char* label, rujian::ui::IconId icon, std::function<void()> onClick) {
  auto button = std::make_unique<rujian::ui::Button>(label);
  button->setIcon(icon);
  button->setOnClick(std::move(onClick));
  return static_cast<rujian::ui::Button*>(parent.addChild(std::move(button)));
}

}  // namespace

RuntimePanelContent::RuntimePanelContent(contracts::RuntimeModel& model) : model_(model) {
  auto row = std::make_unique<rujian::ui::Row>();
  auto actions = std::make_unique<rujian::ui::Actions>();
  power_ = addButton(*actions, "开机", rujian::ui::IconId::Power, [this] { model_.togglePower(); });
  power_->setVariant(rujian::ui::ButtonVariant::Primary);
  pause_ = addButton(*actions, "暂停", rujian::ui::IconId::PlayerPause, [this] { model_.togglePause(); });
  softReset_ = addButton(*actions, "软复位", rujian::ui::IconId::Rotate, [this] { model_.softReset(); });
  hardReset_ = addButton(*actions, "硬复位", rujian::ui::IconId::Refresh, [this] { model_.hardReset(); });
  row->addChild(std::move(actions));

  auto volume = std::make_unique<rujian::ui::Row>();
  auto mute = std::make_unique<rujian::ui::Button>("静音");
  mute->setVariant(rujian::ui::ButtonVariant::Ghost);
  mute->setIconOnly(true);
  mute->setCompact(true);
  mute->setIcon(rujian::ui::IconId::Volume);
  mute->setTooltip("静音");
  mute->setOnClick([this] { model_.toggleMute(); });
  mute_ = static_cast<rujian::ui::Button*>(volume->addChild(std::move(mute)));
  auto slider = std::make_unique<rujian::ui::Slider>();
  slider->setPreferredWidth(kVolumeSliderWidth);
  slider->setOnChange([this](double value) { model_.setVolume(static_cast<int>(value)); });
  volume_ = static_cast<rujian::ui::Slider*>(volume->addChild(std::move(slider)));
  auto value = std::make_unique<rujian::ui::Label>("100%");
  value->setWrap(false);
  volumeValue_ = static_cast<rujian::ui::Label*>(volume->addChild(std::move(value)));
  row->addChild(std::move(volume));
  row_ = addChild(std::move(row));

  refresh();
  unsubscribe_ = model_.subscribe([this] { refresh(); });
}

RuntimePanelContent::~RuntimePanelContent() {
  if (unsubscribe_) unsubscribe_();
}

rujian::ui::Size RuntimePanelContent::measure(rujian::ui::Size available) { return row_->measure(available); }

void RuntimePanelContent::refresh() {
  const contracts::RuntimeView view = model_.view();
  power_->setLabel(view.powerToggleText);
  power_->setEnabled(view.controls.powerToggle);
  pause_->setLabel(view.pauseToggleText);
  pause_->setIcon(view.state == contracts::MachineState::Running ? rujian::ui::IconId::PlayerPause : rujian::ui::IconId::PlayerPlay);
  pause_->setEnabled(view.controls.pauseToggle);
  softReset_->setEnabled(view.controls.softReset);
  hardReset_->setEnabled(view.controls.hardReset);
  mute_->setIcon(view.muted ? rujian::ui::IconId::VolumeOff : rujian::ui::IconId::Volume);
  mute_->setToggled(view.muted);
  volume_->setValue(view.volume);
  volumeValue_->setText(std::to_string(view.volume) + "%");
}

}  // namespace ember::win32::ui
