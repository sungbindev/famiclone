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
constexpr const char* kRegenerate = "更换";
/** “应用”分组的标识（核心登记）与其中“用户数据目录”一行的文字。 */
constexpr const char* kAppGroupId = "app";
constexpr const char* kUserDataLabel = "用户数据目录";
constexpr const char* kUserDataDescription = "设置、ROM、日志等都保存在这里；点路径用文件管理器打开。软件所在目录旁有 ember-data 目录时使用它，否则使用当前用户的应用数据目录。";

/** 左栏的默认、最小与最大宽度（逻辑像素）。 */
constexpr double kSideWidthInitial = 140;
constexpr double kSideWidthMinimum = 140;
constexpr double kSideWidthMaximum = 320;
/** 右侧滚动区内容右边的留白（滚动条贴着对话框的右边，内容自己空出来）。 */
constexpr double kPaneRightPadding = 10;
/** 用户数据目录的路径链接的最大宽度：路径很长时在链接里换行，不让设置行（和对话框的最小宽度）跟着变宽。 */
constexpr double kPathLinkMaxWidth = 280;
/** “用户数据目录”一行里路径链接与按钮上下两行之间的间距。 */
constexpr double kUserDataControlsGap = 6;

/** 限制最大宽度的容器：子控件自然宽度不超过最大宽度时取自然宽度，超过时按最大宽度排版（文字换行）。 */
class MaxWidth final : public Widget {
 public:
  MaxWidth(std::unique_ptr<Widget> child, double maxWidth) : maxWidth_(maxWidth) { child_ = addChild(std::move(child)); }

  [[nodiscard]] Size measure(Size available) override {
    const double limit = available.width > 0 ? std::min(available.width, maxWidth_) : maxWidth_;
    const Size natural = child_->measure(Size{});
    return natural.width <= limit ? natural : child_->measure(Size{limit, 0});
  }
  void arrange(const Rect& bounds) override {
    setBounds(bounds);
    child_->arrange(bounds);
  }
  [[nodiscard]] bool stretchesWidth() const override { return false; }

 private:
  double maxWidth_;
  Widget* child_{nullptr};
};

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

PreferencesContent::PreferencesContent(contracts::PreferencesModel& preferences, contracts::UserDataModel& userData) : preferences_(preferences), userData_(userData) {
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
}

PreferencesContent::~PreferencesContent() {
  if (unsubscribePreferences_) unsubscribePreferences_();
}

Size PreferencesContent::measure(Size available) { return split_->measure(available); }

Size PreferencesContent::minimumSize() {
  // 还没有挂到窗口上时量不出文字，不缓存
  if (host() == nullptr) return Size{};
  const contracts::PreferencesView view = preferences_.view();
  const contracts::UserDataView userData = userData_.view();
  std::string signature;
  for (const contracts::PreferenceGroupView& group : view.groups) signature += signatureOf(view, &group, userData) + '\x1c';
  if (signature != minimumSignature_) {
    // 各分组的右边内容都临时挂到本控件上量一遍：宽度取最宽的一页，高度取在这个宽度下最高的一页
    std::vector<std::unique_ptr<RowWidgets>> scratch;
    std::pair<Button*, Button*> userDataButtons{nullptr, nullptr};
    std::vector<Widget*> panes;
    double width = 0;
    for (const contracts::PreferenceGroupView& group : view.groups) {
      Widget* pane = addChild(buildPane(view, &group, userData, scratch, userDataButtons, true));
      panes.push_back(pane);
      width = std::max(width, pane->measure(Size{}).width);
    }
    double height = 0;
    for (Widget* pane : panes) height = std::max(height, pane->measure(Size{width, 0}).height);
    for (Widget* pane : panes) removeChild(pane);
    minimumSignature_ = signature;
    paneMinimum_ = Size{width, height};
  }
  return Size{split_->sideExtent() + paneMinimum_.width, std::max(paneMinimum_.height, tree_->measure(Size{}).height)};
}

void PreferencesContent::arrange(const Rect& bounds) {
  setBounds(bounds);
  // 左栏不能拖得让右边内容放不下
  const double gap = split_->sideExtent() - split_->sideWidth();
  const double sideMaximum = std::clamp(bounds.width - minimumSize().width + split_->sideExtent() - gap, kSideWidthMinimum, kSideWidthMaximum);
  if (sideMaximum != appliedSideMaximum_) {
    appliedSideMaximum_ = sideMaximum;
    split_->setSideWidthRange(kSideWidthMinimum, sideMaximum);
  }
  split_->arrange(bounds);
}

std::string PreferencesContent::signatureOf(const contracts::PreferencesView& view, const contracts::PreferenceGroupView* group, const contracts::UserDataView& userData) {
  std::string signature = std::to_string(static_cast<int>(view.load)) + '\x1f' + view.message + '\x1f' + (group != nullptr ? group->id : std::string()) + '\x1d';
  if (group == nullptr) return signature;
  for (const contracts::PreferenceRow& row : group->rows) {
    signature += row.id + '\x1f' + std::to_string(static_cast<int>(row.control)) + '\x1f' + row.label + '\x1f' + row.description + '\x1f' + (row.directoryPicker ? 'p' : '-') + std::string(row.regenerates ? "r" : "-");
    for (const contracts::PreferenceChoice& option : row.options) signature += '\x1f' + option.value + '\x1f' + option.label;
    signature += '\x1e';
  }
  if (group->id == kAppGroupId) signature += userData.directory + '\x1f' + userData.switchLabel + '\x1f' + userData.unavailableReason + '\x1e';
  return signature;
}

void PreferencesContent::refresh() {
  const contracts::PreferencesView view = preferences_.view();
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

  const std::string signature = signatureOf(view, current, userDataView);
  if (signature != paneSignature_) {
    paneSignature_ = signature;
    rebuildPane(view, current, userDataView);
  } else if (current != nullptr) {
    updateRows(*current);
  }
}

void PreferencesContent::rebuildPane(const contracts::PreferencesView& view, const contracts::PreferenceGroupView* group, const contracts::UserDataView& userData) {
  rows_.clear();
  std::pair<Button*, Button*> userDataButtons{nullptr, nullptr};
  auto stack = buildPane(view, group, userData, rows_, userDataButtons, false);
  userDataLink_ = userDataButtons.first;
  userDataSwitch_ = userDataButtons.second;
  pane_->setContent(std::move(stack));
  requestLayout();
}

std::unique_ptr<Stack> PreferencesContent::buildPane(const contracts::PreferencesView& view, const contracts::PreferenceGroupView* group, const contracts::UserDataView& userData, std::vector<std::unique_ptr<RowWidgets>>& rows,
                                                      std::pair<Button*, Button*>& userDataButtons, bool measuring) {
  auto stack = std::make_unique<Stack>();
  stack->setPadding(Insets{0, kPaneRightPadding, 0, 0});
  if (!measuring && view.load == contracts::PreferencesLoad::Failed) stack->addChild(errorState(view.message));

  if (group != nullptr && (!group->rows.empty() || group->id == kAppGroupId)) {
    auto list = std::make_unique<List>();
    for (const contracts::PreferenceRow& spec : group->rows) rows.push_back(makeRow(spec, *list));
    if (group->id == kAppGroupId) userDataButtons = addUserDataRow(*list, userData);
    if (measuring) {
      for (const auto& state : rows) state->reset->setVisible(state->regenerate == nullptr);
    }
    auto panel = std::make_unique<Panel>();
    panel->addContent(std::move(list));
    stack->addChild(std::move(panel));
  }
  return stack;
}

std::pair<Button*, Button*> PreferencesContent::addUserDataRow(List& list, const contracts::UserDataView& userData) {
  auto row = std::make_unique<SettingRow>();
  row->setSideBySide(true);
  row->setLabel(kUserDataLabel);
  row->setDescription(userData.unavailableReason.empty() ? std::string(kUserDataDescription) : std::string(kUserDataDescription) + "\n" + userData.unavailableReason);
  auto controls = std::make_unique<FlexBox>(FlexDirection::Column, false, kUserDataControlsGap, FlexAlign::Start);
  auto link = std::make_unique<Button>(userData.directory);
  link->setVariant(ButtonVariant::Link);
  link->setOnClick([this] { userData_.openDirectory(); });
  Button* linkRaw = link.get();
  controls->addChild(std::make_unique<MaxWidth>(std::move(link), kPathLinkMaxWidth));
  auto change = compactButton(userData.switchLabel.c_str());
  change->setEnabled(userData.unavailableReason.empty());
  change->setOnClick([this] { userData_.switchPlace(); });
  auto* changeRaw = static_cast<Button*>(controls->addChild(std::move(change)));
  row->setControl(std::move(controls));
  list.addRow(std::move(row));
  return {linkRaw, changeRaw};
}

std::unique_ptr<PreferencesContent::RowWidgets> PreferencesContent::makeRow(const contracts::PreferenceRow& spec, List& list) {
  auto state = std::make_unique<RowWidgets>();
  RowWidgets* raw = state.get();
  state->id = spec.id;
  state->control = spec.control;
  state->shownText = spec.text;
  const std::string id = spec.id;

  auto row = std::make_unique<SettingRow>();
  row->setSideBySide(true);
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
  if (spec.regenerates) {
    auto regenerate = compactButton(kRegenerate);
    regenerate->setOnClick([this, id] { preferences_.reset(id); });
    raw->regenerate = static_cast<Button*>(controls->addChild(std::move(regenerate)));
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
