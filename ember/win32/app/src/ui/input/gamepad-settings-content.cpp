/*
 * ------------------------------------------------------------------------
 * 名称：gamepad-settings-content.cpp
 * 说明：手柄设置对话框内容的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-20
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "ui/input/gamepad-settings-content.hpp"

#include <algorithm>

#include "ui/input/pad-layout.hpp"

namespace ember::win32::ui {

namespace {

using namespace rujian::ui;

/** 用户可见的文字。 */
constexpr const char* kIntro = "键盘只在机器本身不用键盘时才当手柄用：红白机一直如此，学习机在红白机兼容模式下如此。实体手柄在哪种机器上都能用。同一个键盘按键只能绑给一个手柄键。";
constexpr const char* kReset = "恢复默认";
constexpr const char* kKeyboard = "键盘";
constexpr const char* kGamepad = "实体手柄";
constexpr const char* kDevice = "使用的手柄";
constexpr const char* kDeviceHint = "手柄接上后要按一下它的任意按钮，系统才会发现它。";
constexpr const char* kNone = "不使用";
constexpr const char* kClear = "清除";
constexpr const char* kSourceKey = "key";
constexpr const char* kSourcePad = "pad";

/** 玩家的个数：核心固定两个手柄口。 */
constexpr int kPlayerCount = 2;

std::string tabId(int player) { return "slot" + std::to_string(player); }

}  // namespace

GamepadSettingsContent::GamepadSettingsContent(contracts::GamepadSettingsModel& model) : model_(model) {
  auto stack = std::make_unique<Stack>();

  auto intro = std::make_unique<Callout>();
  intro->setText(kIntro);
  intro_ = static_cast<Callout*>(stack->addChild(std::move(intro)));

  auto failed = std::make_unique<State>();
  failed->setError(true);
  failed->setVisible(false);
  failed_ = static_cast<State*>(stack->addChild(std::move(failed)));

  auto tabs = std::make_unique<Tabs>();
  tabs->setVisible(false);
  tabs_ = tabs.get();
  for (int player = 0; player < kPlayerCount; ++player) {
    auto page = std::make_unique<Stack>();
    players_.push_back(makePlayer(player, *page));
    tabs->addTab(tabId(player), "玩家 " + std::to_string(player + 1), std::move(page));
  }
  tabs->setOnSelect([this](const std::string& id) {
    for (int player = 0; player < kPlayerCount; ++player) {
      if (id == tabId(player)) model_.selectPlayer(player);
    }
  });
  stack->addChild(std::move(tabs));

  auto resetRow = std::make_unique<Row>();
  resetRow->setVisible(false);
  auto reset = std::make_unique<Button>(kReset);
  reset->setOnClick([this] { model_.reset(); });
  reset_ = static_cast<Button*>(resetRow->addChild(std::move(reset)));
  resetRow_ = stack->addChild(std::move(resetRow));

  stack_ = static_cast<Stack*>(addChild(std::move(stack)));
  refresh();
  unsubscribe_ = model_.subscribe([this] { refresh(); });
}

GamepadSettingsContent::~GamepadSettingsContent() {
  if (unsubscribe_) unsubscribe_();
  if (timer_ != 0 && host() != nullptr) host()->stopTimer(timer_);
}

GamepadSettingsContent::PlayerWidgets GamepadSettingsContent::makePlayer(int player, Widget& page) {
  PlayerWidgets widgets;

  auto sourceRow = std::make_unique<Row>();
  auto source = std::make_unique<RadioGroup>();
  source->setOptions({{kSourceKey, kKeyboard, false}, {kSourcePad, kGamepad, false}});
  source->setValue(kSourceKey);
  source->setOnChange([this, player](const std::string& value) { model_.setSource(player, value == kSourcePad ? contracts::PadInputSource::Gamepad : contracts::PadInputSource::Keyboard); });
  widgets.source = static_cast<RadioGroup*>(sourceRow->addChild(std::move(source)));
  page.addChild(std::move(sourceRow));

  auto field = std::make_unique<Field>();
  field->setLabel(kDevice);
  field->setDescription(kDeviceHint);
  auto device = std::make_unique<Select>();
  device->setPlaceholder(kNone);
  device->setAllowEmpty(true);
  device->setOnChange([this, player](const std::string& value) { model_.assignDevice(player, value); });
  widgets.device = device.get();
  field->setControl(std::move(device));
  field->setVisible(false);
  widgets.deviceField = static_cast<Field*>(page.addChild(std::move(field)));

  auto diagram = std::make_unique<PadDiagram>(padLayout());
  diagram->setOnPick([this, player](const std::string& key) { model_.pickKey(player, key); });
  widgets.diagram = static_cast<PadDiagram*>(page.addChild(std::move(diagram)));

  auto status = std::make_unique<DescriptionRow>();
  auto clear = std::make_unique<Button>(kClear);
  clear->setCompact(true);
  clear->setOnClick([this, player] { model_.clearSelected(player); });
  widgets.clear = clear.get();
  status->setAction(std::move(clear));
  widgets.status = static_cast<DescriptionRow*>(page.addChild(std::move(status)));
  return widgets;
}

Size GamepadSettingsContent::measure(Size available) { return stack_->measure(available); }

Size GamepadSettingsContent::minimumSize() {
  if (host() == nullptr) return Size{};
  double width = std::max(tabs_->tabBar()->measure(Size{}).width, reset_->measure(Size{}).width);
  for (const PlayerWidgets& player : players_) width = std::max({width, player.source->measure(Size{}).width, player.diagram->measure(Size{}).width});
  // “使用的手柄”一项切到实体手柄时才出现，高度按它显示的情况算
  std::vector<bool> shown;
  for (const PlayerWidgets& player : players_) {
    shown.push_back(player.deviceField->visible());
    player.deviceField->setVisible(true);
  }
  const double height = stack_->measure(Size{width, 0}).height;
  for (std::size_t i = 0; i < players_.size(); ++i) players_[i].deviceField->setVisible(shown[i]);
  return Size{width, height};
}

void GamepadSettingsContent::arrange(const Rect& bounds) {
  setBounds(bounds);
  stack_->arrange(bounds);
  scheduleTimer();
}

void GamepadSettingsContent::scheduleTimer() {
  if (timer_ == 0 && host() != nullptr) timer_ = host()->startTimer(this, model_.pollIntervalMs());
}

void GamepadSettingsContent::onTimer(unsigned id) {
  if (id != timer_) return;
  timer_ = 0;
  model_.poll();
  scheduleTimer();
}

void GamepadSettingsContent::refresh() {
  const contracts::PadSettingsView view = model_.view();
  const bool ready = view.load == contracts::PadSettingsLoad::Ready;
  std::string signature = std::to_string(static_cast<int>(view.load)) + '\x1f' + view.message;

  failed_->setVisible(view.load == contracts::PadSettingsLoad::Failed);
  failed_->setText(view.message);
  tabs_->setVisible(ready);
  resetRow_->setVisible(ready);

  if (ready) {
    for (std::size_t i = 0; i < players_.size() && i < view.players.size(); ++i) {
      const contracts::PadPlayerView& player = view.players[i];
      PlayerWidgets& widgets = players_[i];
      widgets.source->setValue(player.source == contracts::PadInputSource::Gamepad ? kSourcePad : kSourceKey);
      widgets.deviceField->setVisible(player.showDevice);
      std::vector<SelectOption> options;
      for (const contracts::PadDeviceOption& option : player.devices) options.push_back({option.value, option.label});
      widgets.device->setOptions(std::move(options));
      widgets.device->setValue(player.assignedDevice);
      std::vector<PadHotspot> hotspots;
      for (const contracts::PadKeyView& key : player.keys) hotspots.push_back({key.id, key.role, key.current, key.unbound, key.tip, key.selected, key.learning});
      widgets.diagram->setHotspots(hotspots);
      widgets.diagram->setDisabled(!player.keysEnabled);
      widgets.status->setText(player.message);
      widgets.clear->setEnabled(player.canClear);
      signature += '\x1e' + std::string(player.showDevice ? "d" : "-") + player.message;
    }
    tabs_->select(tabId(std::clamp(view.activePlayer, 0, kPlayerCount - 1)));
  }

  // 说明文字、下拉框是否显示这类会改变高度的内容变了才重新布局
  if (signature != layoutSignature_) {
    layoutSignature_ = signature;
    requestLayout();
  }
}

}  // namespace ember::win32::ui
