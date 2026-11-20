/*
 * ------------------------------------------------------------------------
 * 名称：mappers-content.cpp
 * 说明：“mapper 插件”对话框内容的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：模型变化后不在当前的鼠标事件里重建表格（点击表格行就是表格自己的事件），而是用零毫秒的定时器延后到事件之外。
 * ------------------------------------------------------------------------
 */

#include "ui/mappers/mappers-content.hpp"

#include <algorithm>

#include "rujian/ui/facts.hpp"
#include "rujian/ui/icon-id.hpp"
#include "rujian/ui/label.hpp"
#include "rujian/ui/panel.hpp"
#include "rujian/ui/state.hpp"
#include "rujian/ui/status-label.hpp"

namespace ember::win32::ui {

namespace {

using namespace rujian::ui;

/** 用户可见的文字。 */
constexpr const char* kSearchPlaceholder = "搜索名称、标识或发布者";
constexpr const char* kInstall = "安装插件";
constexpr const char* kUninstall = "卸载";
constexpr const char* kBundledHint = "预装插件不能卸载，只能停用";
constexpr const char* kEnable = "启用";
constexpr const char* kDisable = "停用";
constexpr const char* kEnabled = "已启用";
constexpr const char* kDisabled = "已停用";
constexpr const char* kLoading = "正在读取…";
constexpr const char* kEmpty = "没有符合条件的插件";
constexpr const char* kDetailLoading = "正在读取清单…";
constexpr const char* kSourceAll = "全部来源";
constexpr const char* kSourceBundled = "预装";
constexpr const char* kSourceUser = "用户安装";
constexpr const char* kColumnName = "名称";
constexpr const char* kColumnVersion = "版本";
constexpr const char* kColumnMatch = "匹配";
constexpr const char* kColumnHost = "主机型号";
constexpr const char* kColumnSource = "来源";
constexpr const char* kColumnState = "状态";

/** 搜索框的宽度（逻辑像素）。 */
constexpr double kSearchWidth = 220;
/** 匹配列的最小宽度。 */
constexpr double kNameMinWidth = 140;
constexpr double kMatchMinWidth = 120;
/** 滚动区内容右边的留白（滚动条贴着对话框的右边，内容自己空出来）。 */
constexpr double kContentRightPadding = 10;

/** 固定宽度的容器：子控件的宽度固定，高度取自己的。 */
class FixedWidth final : public Widget {
 public:
  FixedWidth(std::unique_ptr<Widget> child, double width) : width_(width) { child_ = addChild(std::move(child)); }

  [[nodiscard]] Size measure(Size available) override {
    const Size size = child_->measure({width_, available.height});
    return {width_, size.height};
  }
  void arrange(const Rect& bounds) override {
    setBounds(bounds);
    child_->arrange(bounds);
  }
  [[nodiscard]] bool stretchesWidth() const override { return false; }

 private:
  double width_;
  Widget* child_{nullptr};
};

std::unique_ptr<State> stateOf(const std::string& text, bool error) {
  auto state = std::make_unique<State>();
  state->setText(text);
  state->setError(error);
  return state;
}

}  // namespace

MappersContent::MappersContent(contracts::MappersModel& model) : model_(model) {
  auto stack = std::make_unique<Stack>();
  stack->setPadding(Insets{0, kContentRightPadding, 0, 0});

  auto toolbar = std::make_unique<Toolbar>();
  auto search = std::make_unique<TextInput>();
  search->setSearch(true);
  search->setPlaceholder(kSearchPlaceholder);
  search_ = search.get();
  search_->setOnChange([this](const std::string& text) {
    if (!syncing_) model_.setSearch(text);
  });
  toolbar->setLeading(std::make_unique<FixedWidth>(std::move(search), kSearchWidth));

  auto actions = std::make_unique<Actions>();
  auto source = std::make_unique<Select>();
  source->setAllowEmpty(false);
  source->setOptions({{contracts::kMapperSourceAll, kSourceAll}, {contracts::kMapperSourceBundled, kSourceBundled}, {contracts::kMapperSourceUser, kSourceUser}});
  source->setValue(contracts::kMapperSourceAll);
  source->setOnChange([this](const std::string& value) {
    if (!syncing_) model_.setSource(value);
  });
  source_ = static_cast<Select*>(actions->addChild(std::move(source)));
  auto install = std::make_unique<Button>(kInstall);
  install->setVariant(ButtonVariant::Primary);
  install->setIcon(IconId::Plus);
  install->setOnClick([this] { model_.install(); });
  install_ = static_cast<Button*>(actions->addChild(std::move(install)));
  toolbar->setActions(std::move(actions));
  toolbar_ = static_cast<Toolbar*>(stack->addChild(std::move(toolbar)));

  body_ = static_cast<Stack*>(stack->addChild(std::make_unique<Stack>()));

  auto scroll = std::make_unique<ScrollArea>();
  scroll->setContent(std::move(stack));
  scroll_ = static_cast<ScrollArea*>(addChild(std::move(scroll)));

  refresh();
  unsubscribe_ = model_.subscribe([this] {
    if (host() == nullptr) {
      refresh();
      return;
    }
    if (refreshTimer_ == 0) refreshTimer_ = host()->startTimer(this, 0);
  });
}

MappersContent::~MappersContent() {
  if (unsubscribe_) unsubscribe_();
  if (refreshTimer_ != 0 && host() != nullptr) host()->stopTimer(refreshTimer_);
}

void MappersContent::onTimer(unsigned id) {
  if (id != refreshTimer_) return;
  refreshTimer_ = 0;
  refresh();
}

Size MappersContent::measure(Size available) { return scroll_->measure(available); }

Size MappersContent::minimumSize() {
  if (host() == nullptr) return Size{};
  const double table = table_ != nullptr ? table_->minimumSize().width : 0.0;
  return Size{std::max(toolbar_->measure(Size{}).width, table) + kContentRightPadding, 0.0};
}

void MappersContent::arrange(const Rect& bounds) {
  setBounds(bounds);
  scroll_->arrange(bounds);
}

std::string MappersContent::signatureOf(const contracts::MappersView& view) {
  std::string signature = std::to_string(static_cast<int>(view.load)) + '\x1f' + view.message + '\x1f' + view.selectedId + '\x1f' + std::to_string(static_cast<int>(view.detailLoad)) + '\x1f' + view.detailMessage + '\x1d';
  for (const contracts::MapperRow& row : view.rows) {
    signature += row.id + '\x1f' + row.name + '\x1f' + row.description + '\x1f' + row.version + '\x1f' + row.match + '\x1f' + row.hosts + '\x1f' + row.source + '\x1f' + (row.enabled ? 'e' : '-') + '\x1e';
  }
  signature += '\x1d' + view.selected.name + '\x1f' + (view.selected.enabled ? 'e' : '-') + '\x1f' + (view.selected.bundled ? 'b' : '-') + '\x1d';
  for (const contracts::MapperFact& fact : view.facts) {
    signature += fact.label;
    for (const std::string& line : fact.lines) signature += '\x1f' + line;
    signature += '\x1e';
  }
  return signature;
}

void MappersContent::refresh() {
  const contracts::MappersView view = model_.view();

  // 搜索框与来源下拉框跟随模型（别处改了才变），不触发回调
  syncing_ = true;
  if (search_->value() != view.search) search_->setValue(view.search);
  source_->setValue(view.source);
  syncing_ = false;

  if (view.stats != shownStats_) {
    shownStats_ = view.stats;
    toolbar_->setDescription(view.stats);
    toolbar_->requestLayout();
  }

  const std::string signature = signatureOf(view);
  if (signature == signature_) return;
  signature_ = signature;
  rebuildBody(view);
}

void MappersContent::rebuildBody(const contracts::MappersView& view) {
  while (!body_->children().empty()) body_->removeChild(body_->children().front().get());
  table_ = nullptr;
  rowIds_.clear();
  detail_ = {};
  statusText_.clear();

  if (view.load == contracts::MappersLoad::Failed) {
    statusText_ = view.message;
    body_->addChild(stateOf(statusText_, true));
  } else if (view.load == contracts::MappersLoad::Loading) {
    statusText_ = kLoading;
    body_->addChild(stateOf(statusText_, false));
  } else if (view.rows.empty()) {
    statusText_ = kEmpty;
    body_->addChild(stateOf(statusText_, false));
  }

  if (view.load == contracts::MappersLoad::Ready && !view.rows.empty()) {
    auto table = std::make_unique<Table>();
    table->addColumn({kColumnName, kNameMinWidth, false, false});
    table->addColumn({kColumnVersion, 0, true, true});
    table->addColumn({kColumnMatch, kMatchMinWidth, false, true});
    table->addColumn({kColumnHost, 0, true, true});
    table->addColumn({kColumnSource, 0, true, true});
    table->addColumn({kColumnState, 0, true, false});
    int selectedRow = -1;
    for (const contracts::MapperRow& row : view.rows) {
      std::vector<std::unique_ptr<Widget>> cells;
      auto main = std::make_unique<TableMainCell>();
      main->setText(row.name);
      main->setDescription(row.description);
      cells.push_back(std::move(main));
      cells.push_back(Table::makeText(row.version));
      cells.push_back(Table::makeText(row.match));
      cells.push_back(Table::makeText(row.hosts));
      cells.push_back(Table::makeText(row.source));
      auto state = std::make_unique<StatusLabel>();
      state->setTone(row.enabled ? StatusTone::Success : StatusTone::Stopped);
      state->setText(row.enabled ? kEnabled : kDisabled);
      cells.push_back(std::move(state));
      const std::size_t index = table->addRow(std::move(cells));
      rowIds_.push_back(row.id);
      if (row.id == view.selectedId) {
        selectedRow = static_cast<int>(index);
        table->setRowDetail(index, makeDetail(view));
      }
    }
    table->setOnRowSelect([this](std::size_t row) {
      if (row < rowIds_.size()) model_.toggleSelect(rowIds_[row]);
    });
    table->setSelectedRow(selectedRow);
    table_ = static_cast<Table*>(body_->addChild(std::move(table)));
  }
  requestLayout();
}

std::unique_ptr<Widget> MappersContent::makeDetail(const contracts::MappersView& view) {
  auto panel = std::make_unique<Panel>();
  panel->setPlain(true);
  panel->setCompactHead(true);
  panel->setTitle(view.selected.name);

  auto actions = std::make_unique<Actions>();
  const std::string id = view.selected.id;
  auto toggle = std::make_unique<Button>(view.selected.enabled ? kDisable : kEnable);
  toggle->setCompact(true);
  const bool enable = !view.selected.enabled;
  toggle->setOnClick([this, id, enable] { model_.setEnabled(id, enable); });
  detail_.toggle = static_cast<Button*>(actions->addChild(std::move(toggle)));
  auto uninstall = std::make_unique<Button>(kUninstall);
  uninstall->setVariant(ButtonVariant::Danger);
  uninstall->setCompact(true);
  uninstall->setIcon(IconId::Trash);
  uninstall->setEnabled(!view.selected.bundled);
  uninstall->setTooltip(view.selected.bundled ? kBundledHint : kUninstall);
  uninstall->setOnClick([this, id] { model_.uninstall(id); });
  detail_.uninstall = static_cast<Button*>(actions->addChild(std::move(uninstall)));
  panel->setAside(std::move(actions));

  if (view.detailLoad == contracts::MapperDetailLoad::Failed) {
    panel->addContent(stateOf(view.detailMessage, true));
  } else if (view.detailLoad == contracts::MapperDetailLoad::Ready) {
    auto facts = std::make_unique<Facts>();
    for (const contracts::MapperFact& fact : view.facts) {
      std::string text;
      for (std::size_t i = 0; i < fact.lines.size(); ++i) text += (i == 0 ? "" : "\n") + fact.lines[i];
      facts->addFact(fact.label, std::move(text));
    }
    panel->addContent(std::move(facts));
  } else {
    panel->addContent(stateOf(kDetailLoading, false));
  }
  return panel;
}

}  // namespace ember::win32::ui
