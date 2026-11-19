/*
 * ------------------------------------------------------------------------
 * 名称：preferences-content.cpp
 * 说明：“首选项”对话框内容的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-20
 * 备注：左栏宽度：默认就是最小宽度 140，可拖到 320。
 * ------------------------------------------------------------------------
 */

#include "ui/preferences/preferences-content.hpp"

#include <algorithm>

#include "rujian/ui/callout.hpp"
#include "rujian/ui/list.hpp"
#include "rujian/ui/panel.hpp"
#include "rujian/ui/state.hpp"

namespace ember::win32::ui {

namespace {

using namespace rujian::ui;

/** 用户可见的文字。 */
constexpr const char* kSwitchOn = "开启";
constexpr const char* kReset = "恢复默认";
constexpr const char* kPick = "选择";
constexpr const char* kBiosIntro = "软件不自带 BIOS，也不会在电脑里自己查找：需要 BIOS 的机型要在这里手动添加 BIOS 文件。添加的文件会复制到软件的 BIOS 目录，开机时直接使用，原文件可以删除或移动；想换成别的 BIOS，点“替换”。";
constexpr const char* kBiosAdd = "添加 BIOS 文件";
constexpr const char* kBiosReplace = "替换";
constexpr const char* kBiosRemove = "移除";
constexpr const char* kBiosOpenDirectory = "打开 BIOS 目录";
/** “应用”分组的标识（核心登记）与其中“用户数据目录”一行的文字。 */
constexpr const char* kAppGroupId = "app";
constexpr const char* kUserDataLabel = "用户数据目录";
constexpr const char* kUserDataDescription = "设置、BIOS、日志等都保存在这里；点路径用文件管理器打开。软件所在目录旁有 ember-data 目录时使用它，否则使用当前用户的应用数据目录。";

/** 左栏的默认、最小与最大宽度（逻辑像素）。 */
constexpr double kSideWidthInitial = 140;
constexpr double kSideWidthMinimum = 140;
constexpr double kSideWidthMaximum = 320;

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

PreferencesContent::PreferencesContent(contracts::PreferencesModel& preferences, contracts::BiosModel& bios, contracts::UserDataModel& userData) : preferences_(preferences), bios_(bios), userData_(userData) {
  auto split = std::make_unique<SplitView>();
  split->setSideWidthRange(kSideWidthMinimum, kSideWidthMaximum);
  split->setSideWidth(kSideWidthInitial);
  auto tree = std::make_unique<Tree>();
  tree_ = tree.get();
  tree_->setOnSelect([this](const std::string& id) {
    selected_ = id;
    refresh();
  });
  split->setSide(std::move(tree));
  auto pane = std::make_unique<ScrollArea>();
  pane_ = pane.get();
  split->setMain(std::move(pane));
  split_ = static_cast<SplitView*>(addChild(std::move(split)));

  refresh();
  unsubscribePreferences_ = preferences_.subscribe([this] { refresh(); });
  unsubscribeBios_ = bios_.subscribe([this] { refresh(); });
}

PreferencesContent::~PreferencesContent() {
  if (unsubscribePreferences_) unsubscribePreferences_();
  if (unsubscribeBios_) unsubscribeBios_();
}

Size PreferencesContent::measure(Size available) { return split_->measure(available); }

void PreferencesContent::arrange(const Rect& bounds) {
  setBounds(bounds);
  split_->arrange(bounds);
}

std::string PreferencesContent::signatureOf(const contracts::PreferencesView& view, const contracts::PreferenceGroupView* group, const contracts::BiosView& bios, const contracts::UserDataView& userData) {
  std::string signature = std::to_string(static_cast<int>(view.load)) + '\x1f' + view.message + '\x1f' + (group != nullptr ? group->id : std::string()) + '\x1d';
  if (group == nullptr) return signature;
  if (group->machines) {
    signature += std::to_string(static_cast<int>(bios.load)) + '\x1f' + bios.message + '\x1e';
    for (const contracts::BiosRow& row : bios.rows) signature += row.machine + '\x1f' + row.name + '\x1f' + row.status + '\x1f' + (row.hasBios ? 'b' : '-') + (row.supported ? 's' : '-') + '\x1e';
    return signature;
  }
  for (const contracts::PreferenceRow& row : group->rows) {
    signature += row.id + '\x1f' + std::to_string(static_cast<int>(row.control)) + '\x1f' + row.label + '\x1f' + row.description + '\x1f' + (row.directoryPicker ? 'p' : '-');
    for (const contracts::PreferenceChoice& option : row.options) signature += '\x1f' + option.value + '\x1f' + option.label;
    signature += '\x1e';
  }
  if (group->id == kAppGroupId) signature += userData.directory + '\x1f' + userData.switchLabel + '\x1f' + userData.unavailableReason + '\x1e';
  return signature;
}

void PreferencesContent::refresh() {
  const contracts::PreferencesView view = preferences_.view();
  const contracts::BiosView biosView = bios_.view();
  const contracts::UserDataView userDataView = userData_.view();

  std::string treeSignature;
  for (const contracts::PreferenceGroupView& group : view.groups) treeSignature += group.id + '\x1f' + group.label + '\x1e';
  if (treeSignature != treeSignature_) {
    treeSignature_ = treeSignature;
    std::vector<TreeItem> items;
    for (const contracts::PreferenceGroupView& group : view.groups) items.push_back({group.id, group.label});
    tree_->setItems(std::move(items));
  }

  // 选中的分组不在清单里（还没读到、核心重启）时取第一个
  const auto found = std::find_if(view.groups.begin(), view.groups.end(), [this](const contracts::PreferenceGroupView& group) { return group.id == selected_; });
  const contracts::PreferenceGroupView* current = found != view.groups.end() ? &*found : (view.groups.empty() ? nullptr : &view.groups.front());
  selected_ = current != nullptr ? current->id : std::string();
  tree_->setSelectedId(selected_);

  const std::string signature = signatureOf(view, current, biosView, userDataView);
  if (signature != paneSignature_) {
    paneSignature_ = signature;
    rebuildPane(view, current, biosView, userDataView);
  } else if (current != nullptr && !current->machines) {
    updateRows(*current);
  }
}

void PreferencesContent::rebuildPane(const contracts::PreferencesView& view, const contracts::PreferenceGroupView* group, const contracts::BiosView& bios, const contracts::UserDataView& userData) {
  rows_.clear();
  biosRows_.clear();
  openDirectory_ = nullptr;
  userDataLink_ = nullptr;
  userDataSwitch_ = nullptr;

  auto stack = std::make_unique<Stack>();
  if (view.load == contracts::PreferencesLoad::Failed) stack->addChild(errorState(view.message));

  if (group != nullptr && group->machines) {
    auto callout = std::make_unique<Callout>();
    callout->setText(kBiosIntro);
    auto link = std::make_unique<Button>(kBiosOpenDirectory);
    link->setVariant(ButtonVariant::Link);
    link->setOnClick([this] { bios_.openDirectory(); });
    openDirectory_ = static_cast<Button*>(callout->addContent(std::move(link)));
    stack->addChild(std::move(callout));
    if (bios.load == contracts::PreferencesLoad::Failed) stack->addChild(errorState(bios.message));
    if (!bios.rows.empty()) {
      auto list = std::make_unique<List>();
      for (const contracts::BiosRow& spec : bios.rows) {
        auto row = std::make_unique<SettingRow>();
        row->setLabel(spec.name);
        row->setDescription(spec.status);
        auto controls = std::make_unique<rujian::ui::Row>();
        BiosWidgets widgets;
        widgets.machine = spec.machine;
        const std::string machine = spec.machine;
        if (!spec.hasBios) {
          auto add = compactButton(kBiosAdd);
          add->setEnabled(spec.supported);
          add->setOnClick([this, machine] { bios_.choose(machine); });
          widgets.add = static_cast<Button*>(controls->addChild(std::move(add)));
        } else {
          auto replace = compactButton(kBiosReplace);
          replace->setEnabled(spec.supported);
          replace->setOnClick([this, machine] { bios_.choose(machine); });
          widgets.replace = static_cast<Button*>(controls->addChild(std::move(replace)));
          auto remove = compactButton(kBiosRemove);
          remove->setEnabled(spec.supported);
          remove->setOnClick([this, machine] { bios_.remove(machine); });
          widgets.remove = static_cast<Button*>(controls->addChild(std::move(remove)));
        }
        row->setControl(std::move(controls));
        list->addRow(std::move(row));
        biosRows_.push_back(widgets);
      }
      auto panel = std::make_unique<Panel>();
      panel->addContent(std::move(list));
      stack->addChild(std::move(panel));
    }
  } else if (group != nullptr && (!group->rows.empty() || group->id == kAppGroupId)) {
    auto list = std::make_unique<List>();
    for (const contracts::PreferenceRow& spec : group->rows) rows_.push_back(makeRow(spec, *list));
    if (group->id == kAppGroupId) addUserDataRow(*list, userData);
    auto panel = std::make_unique<Panel>();
    panel->addContent(std::move(list));
    stack->addChild(std::move(panel));
  }

  pane_->setContent(std::move(stack));
  requestLayout();
}

void PreferencesContent::addUserDataRow(List& list, const contracts::UserDataView& userData) {
  auto row = std::make_unique<SettingRow>();
  row->setLabel(kUserDataLabel);
  row->setDescription(userData.unavailableReason.empty() ? std::string(kUserDataDescription) : std::string(kUserDataDescription) + "\n" + userData.unavailableReason);
  auto controls = std::make_unique<rujian::ui::Row>();
  auto link = std::make_unique<Button>(userData.directory);
  link->setVariant(ButtonVariant::Link);
  link->setOnClick([this] { userData_.openDirectory(); });
  userDataLink_ = static_cast<Button*>(controls->addChild(std::move(link)));
  auto change = compactButton(userData.switchLabel.c_str());
  change->setEnabled(userData.unavailableReason.empty());
  change->setOnClick([this] { userData_.switchPlace(); });
  userDataSwitch_ = static_cast<Button*>(controls->addChild(std::move(change)));
  row->setControl(std::move(controls));
  list.addRow(std::move(row));
}

std::unique_ptr<PreferencesContent::RowWidgets> PreferencesContent::makeRow(const contracts::PreferenceRow& spec, List& list) {
  auto state = std::make_unique<RowWidgets>();
  RowWidgets* raw = state.get();
  state->id = spec.id;
  state->control = spec.control;
  state->shownText = spec.text;
  const std::string id = spec.id;

  auto row = std::make_unique<SettingRow>();
  row->setLabel(spec.label);
  row->setDescription(spec.description);
  auto controls = std::make_unique<rujian::ui::Row>();

  switch (spec.control) {
    case contracts::PreferenceControl::Choice: {
      auto select = std::make_unique<Select>();
      select->setAllowEmpty(false);
      std::vector<SelectOption> options;
      for (const contracts::PreferenceChoice& option : spec.options) options.push_back({option.value, option.label});
      select->setOptions(std::move(options));
      select->setValue(spec.text);
      select->setOnChange([this, id](const std::string& value) { preferences_.setChoice(id, value); });
      raw->select = static_cast<Select*>(controls->addChild(std::move(select)));
      break;
    }
    case contracts::PreferenceControl::Switch: {
      auto checkbox = std::make_unique<Checkbox>(kSwitchOn);
      checkbox->setChecked(spec.checked);
      checkbox->setOnChange([this, id](bool checked) { preferences_.setSwitch(id, checked); });
      raw->checkbox = static_cast<Checkbox*>(controls->addChild(std::move(checkbox)));
      break;
    }
    case contracts::PreferenceControl::Integer:
    case contracts::PreferenceControl::Text: {
      auto input = std::make_unique<TextInput>();
      input->setValue(spec.text);
      input->setOnChange([raw](const std::string&) {
        raw->dirty = true;
        raw->resync = false;
        raw->input->setInvalid(false);
        if (!raw->error.empty()) {
          raw->error.clear();
          raw->row->setError({});
          raw->row->requestLayout();
        }
      });
      input->setOnBlur([this, raw] { submit(*raw); });
      input->setOnEnter([this, raw] { submit(*raw); });
      raw->input = static_cast<TextInput*>(controls->addChild(std::move(input)));
      break;
    }
  }

  if (spec.directoryPicker) {
    auto pick = compactButton(kPick);
    pick->setOnClick([this, id] { preferences_.pickDirectory(id); });
    raw->pick = static_cast<Button*>(controls->addChild(std::move(pick)));
  }
  auto reset = compactButton(kReset);
  reset->setVisible(spec.changed);
  reset->setOnClick([this, id] { preferences_.reset(id); });
  raw->reset = static_cast<Button*>(controls->addChild(std::move(reset)));

  row->setControl(std::move(controls));
  raw->row = static_cast<SettingRow*>(list.addRow(std::move(row)));
  return state;
}

void PreferencesContent::submit(RowWidgets& state) {
  if (!state.dirty) return;
  // 提交后输入框回到模型里的值（成功时是新值，失败或无变化时是原值）
  state.resync = true;
  const std::string message = preferences_.submitText(state.id, state.input->value());
  if (message.empty()) {
    // 已提交：到模型通知时输入框回到模型里的值，在此之前失焦等不再重复提交
    state.dirty = false;
    return;
  }
  state.resync = false;
  state.error = message;
  state.input->setInvalid(true);
  state.row->setError(message);
  state.row->requestLayout();
}

void PreferencesContent::updateRows(const contracts::PreferenceGroupView& group) {
  bool layoutChanged = false;
  for (const auto& state : rows_) {
    const auto spec = std::find_if(group.rows.begin(), group.rows.end(), [&state](const contracts::PreferenceRow& row) { return row.id == state->id; });
    if (spec == group.rows.end()) continue;
    switch (state->control) {
      case contracts::PreferenceControl::Choice: state->select->setValue(spec->text); break;
      case contracts::PreferenceControl::Switch: state->checkbox->setChecked(spec->checked); break;
      case contracts::PreferenceControl::Integer:
      case contracts::PreferenceControl::Text:
        // 输入中的内容只在模型里的值变了、或刚提交过时才被冲掉
        if (state->resync || spec->text != state->shownText) {
          state->input->setValue(spec->text);
          state->shownText = spec->text;
          state->dirty = false;
          state->resync = false;
          state->input->setInvalid(false);
          if (!state->error.empty()) {
            state->error.clear();
            state->row->setError({});
            layoutChanged = true;
          }
        }
        break;
    }
    if (state->reset->visible() != spec->changed) {
      state->reset->setVisible(spec->changed);
      layoutChanged = true;
    }
  }
  if (layoutChanged) requestLayout();
}

}  // namespace ember::win32::ui
