/*
 * ------------------------------------------------------------------------
 * 名称：machines-content.cpp
 * 说明：“机型与 ROM”对话框内容的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "ui/machines/machines-content.hpp"

#include <algorithm>

#include "rujian/ui/callout.hpp"
#include "rujian/ui/list.hpp"
#include "rujian/ui/panel.hpp"
#include "rujian/ui/setting-row.hpp"
#include "rujian/ui/state.hpp"

namespace ember::win32::ui {

namespace {

using namespace rujian::ui;

/** 用户可见的文字。 */
constexpr const char* kRomIntro = "软件不自带 ROM，也不会在电脑里自己查找：需要 ROM 的机型要在这里手动添加 ROM 文件。添加的文件会复制到软件的 ROM 目录，开机时直接使用，原文件可以删除或移动；想换成别的 ROM，点“替换”。";
constexpr const char* kRomAdd = "添加 ROM 文件";
constexpr const char* kRomReplace = "替换";
constexpr const char* kRomRemove = "移除";
constexpr const char* kRomOpenDirectory = "打开 ROM 目录";

/** 滚动区内容右边的留白（滚动条贴着对话框的右边，内容自己空出来）。 */
constexpr double kPaneRightPadding = 10;
/** 顶部说明段落的最小宽度：段落可以折行，但不窄到文字挤成一条（文字宽度至少约 420，再加上说明框的边框与内边距）。 */
constexpr double kIntroMinWidth = 460;

std::unique_ptr<Button> compactButton(const char* label) {
  auto button = std::make_unique<Button>(label);
  button->setCompact(true);
  return button;
}

std::unique_ptr<State> errorState(const std::string& message) {
  auto state = std::make_unique<State>();
  state->setText(message);
  state->setError(true);
  return state;
}

}  // namespace

MachinesContent::MachinesContent(contracts::RomModel& rom) : rom_(rom) {
  auto pane = std::make_unique<ScrollArea>();
  pane_ = static_cast<ScrollArea*>(addChild(std::move(pane)));
  refresh();
  unsubscribe_ = rom_.subscribe([this] { refresh(); });
}

MachinesContent::~MachinesContent() {
  if (unsubscribe_) unsubscribe_();
}

Size MachinesContent::measure(Size available) { return pane_->measure(available); }

Size MachinesContent::minimumSize() {
  if (host() == nullptr) return Size{};
  const double list = listPanel_ != nullptr ? listPanel_->measure(Size{}).width : 0.0;
  return Size{std::max(list, kIntroMinWidth) + kPaneRightPadding, 0.0};
}

void MachinesContent::arrange(const Rect& bounds) {
  setBounds(bounds);
  pane_->arrange(bounds);
}

std::string MachinesContent::signatureOf(const contracts::RomView& view) {
  std::string signature = std::to_string(static_cast<int>(view.load)) + '\x1f' + view.message + '\x1e';
  for (const contracts::RomRow& row : view.rows) signature += row.machine + '\x1f' + row.name + '\x1f' + row.status + '\x1f' + (row.hasRom ? 'b' : '-') + (row.supported ? 's' : '-') + '\x1e';
  return signature;
}

void MachinesContent::refresh() {
  const contracts::RomView view = rom_.view();
  const std::string signature = signatureOf(view);
  if (signature == signature_) return;
  signature_ = signature;
  rebuild(view);
}

void MachinesContent::rebuild(const contracts::RomView& view) {
  romRows_.clear();
  openDirectory_ = nullptr;
  listPanel_ = nullptr;

  auto stack = std::make_unique<Stack>();
  stack->setPadding(Insets{0, kPaneRightPadding, 0, 0});

  auto callout = std::make_unique<Callout>();
  callout->setText(kRomIntro);
  auto link = std::make_unique<Button>(kRomOpenDirectory);
  link->setVariant(ButtonVariant::Link);
  link->setOnClick([this] { rom_.openDirectory(); });
  openDirectory_ = static_cast<Button*>(callout->addContent(std::move(link)));
  stack->addChild(std::move(callout));
  if (view.load == contracts::MachinesLoad::Failed) stack->addChild(errorState(view.message));
  if (!view.rows.empty()) {
    auto list = std::make_unique<List>();
    for (const contracts::RomRow& spec : view.rows) {
      auto row = std::make_unique<SettingRow>();
      row->setSideBySide(true);
      row->setLabel(spec.name);
      row->setDescription(spec.status);
      auto controls = std::make_unique<rujian::ui::Row>();
      RomWidgets widgets;
      widgets.machine = spec.machine;
      const std::string machine = spec.machine;
      if (!spec.hasRom) {
        auto add = compactButton(kRomAdd);
        add->setEnabled(spec.supported);
        add->setOnClick([this, machine] { rom_.choose(machine); });
        widgets.add = static_cast<Button*>(controls->addChild(std::move(add)));
      } else {
        auto replace = compactButton(kRomReplace);
        replace->setEnabled(spec.supported);
        replace->setOnClick([this, machine] { rom_.choose(machine); });
        widgets.replace = static_cast<Button*>(controls->addChild(std::move(replace)));
        auto remove = compactButton(kRomRemove);
        remove->setEnabled(spec.supported);
        remove->setOnClick([this, machine] { rom_.remove(machine); });
        widgets.remove = static_cast<Button*>(controls->addChild(std::move(remove)));
      }
      row->setControl(std::move(controls));
      list->addRow(std::move(row));
      romRows_.push_back(widgets);
    }
    auto panel = std::make_unique<Panel>();
    panel->addContent(std::move(list));
    listPanel_ = stack->addChild(std::move(panel));
  }

  pane_->setContent(std::move(stack));
  requestLayout();
}

}  // namespace ember::win32::ui
