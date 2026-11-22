/*
 * ------------------------------------------------------------------------
 * 名称：shell-view.cpp
 * 说明：外壳界面的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：底部面板的尺寸取 rujian-ui-kit 的外壳样式（shell.css）：标题行上边与右边留 4（space-xs）、左边留 10（content-padding），标题 13 号半粗；内容区上边留 8（space-md）、左右与下边留 10；两个按钮之间留 4。
 * ------------------------------------------------------------------------
 */

#include "ui/shell/shell-view.hpp"

#include <algorithm>

#include "rujian/ui/button.hpp"
#include "rujian/ui/label.hpp"

namespace ember::win32::ui {

namespace {

using namespace rujian::ui;

constexpr double kPanelPaddingXs = 4;
constexpr double kPanelPaddingMd = 8;
constexpr double kPanelPadding = 10;
constexpr double kPanelTitleFontSize = 13;
constexpr int kPanelTitleFontWeight = 600;
/** 底部面板高度含顶部的分割线。 */
constexpr double kPanelBorder = 1;

IconId iconOf(contracts::ShellIcon icon) {
  switch (icon) {
    case contracts::ShellIcon::PlayerPlay: return IconId::PlayerPlay;
    case contracts::ShellIcon::PlayerPause: return IconId::PlayerPause;
    case contracts::ShellIcon::DeviceFloppy: return IconId::DeviceFloppy;
  }
  return IconId::PlayerPlay;
}

StatusItemTone toneOf(contracts::ShellTone tone) {
  switch (tone) {
    case contracts::ShellTone::Normal: return StatusItemTone::Normal;
    case contracts::ShellTone::Success: return StatusItemTone::Success;
    case contracts::ShellTone::Warning: return StatusItemTone::Warning;
    case contracts::ShellTone::Error: return StatusItemTone::Error;
  }
  return StatusItemTone::Normal;
}

NoticeLevel levelOf(contracts::NotificationLevel level) {
  switch (level) {
    case contracts::NotificationLevel::Info: return NoticeLevel::Info;
    case contracts::NotificationLevel::Success: return NoticeLevel::Success;
    case contracts::NotificationLevel::Warning: return NoticeLevel::Warning;
    case contracts::NotificationLevel::Error: return NoticeLevel::Error;
  }
  return NoticeLevel::Info;
}

MenuItem toMenuItem(const contracts::ShellMenuEntry& entry) {
  MenuItem item;
  item.id = entry.id;
  item.label = entry.label;
  item.checked = entry.checked;
  item.radio = entry.radio;
  item.disabled = entry.disabled;
  item.shortcut = entry.shortcut;
  item.separatorBefore = entry.separatorBefore;
  item.mnemonic = entry.mnemonic;
  item.hint = entry.hint;
  for (const auto& sub : entry.items) item.items.push_back(toMenuItem(sub));
  return item;
}

void appendSignature(std::string& out, const contracts::ShellMenuEntry& entry) {
  out += entry.id + '\x1f' + entry.label + '\x1f' + entry.shortcut + '\x1f' + entry.hint + '\x1f' + entry.mnemonic;
  out += entry.disabled ? 'd' : '-';
  out += entry.radio ? 'r' : '-';
  out += entry.separatorBefore ? 's' : '-';
  out += entry.checked ? (*entry.checked ? 'T' : 'F') : 'N';
  out += '(';
  for (const auto& sub : entry.items) appendSignature(out, sub);
  out += ')';
}

/** 底部面板：标题行（面板标题，右边是自动收缩开关与关闭按钮）加内容。 */
class BottomPanelView final : public Widget {
 public:
  BottomPanelView(const contracts::ShellOpenPanel& panel, std::function<void()> onAutoCollapse, std::function<void()> onClose, std::unique_ptr<Widget> content) {
    auto title = std::make_unique<Label>(panel.title);
    title->setFontSize(kPanelTitleFontSize);
    title->setFontWeight(kPanelTitleFontWeight);
    title->setWrap(false);
    title_ = static_cast<Label*>(addChild(std::move(title)));
    if (panel.autoCollapse) {
      auto button = std::make_unique<Button>("自动收缩");
      button->setVariant(ButtonVariant::Ghost);
      button->setIconOnly(true);
      button->setCompact(true);
      button->setIcon(IconId::LayoutBottombarCollapse);
      button->setToggled(panel.autoCollapseOn);
      button->setTooltip("自动收缩");
      button->setOnClick(std::move(onAutoCollapse));
      autoButton_ = static_cast<Button*>(addChild(std::move(button)));
    }
    auto close = std::make_unique<Button>("关闭");
    close->setVariant(ButtonVariant::Ghost);
    close->setIconOnly(true);
    close->setCompact(true);
    close->setIcon(IconId::X);
    close->setTooltip("关闭");
    close->setOnClick(std::move(onClose));
    closeButton_ = static_cast<Button*>(addChild(std::move(close)));
    content_ = addChild(std::move(content));
  }

  [[nodiscard]] Size measure(Size available) override {
    const double width = available.width;
    return Size{width, headerHeight() + kPanelPaddingMd + content_->measure(Size{std::max(0.0, width - 2 * kPanelPadding), 0}).height + kPanelPadding};
  }

  void arrange(const Rect& bounds) override {
    Widget::arrange(bounds);
    const double header = headerHeight();
    double right = bounds.right() - kPanelPaddingXs;
    const Size closeSize = closeButton_->measure(Size{});
    right -= closeSize.width;
    closeButton_->arrange(Rect{right, bounds.y + kPanelPaddingXs + (header - kPanelPaddingXs - closeSize.height) / 2, closeSize.width, closeSize.height});
    if (autoButton_ != nullptr) {
      const Size autoSize = autoButton_->measure(Size{});
      right -= kPanelPaddingXs + autoSize.width;
      autoButton_->arrange(Rect{right, bounds.y + kPanelPaddingXs + (header - kPanelPaddingXs - autoSize.height) / 2, autoSize.width, autoSize.height});
    }
    const Size titleSize = title_->measure(Size{});
    title_->arrange(Rect{bounds.x + kPanelPadding, bounds.y + kPanelPaddingXs + (header - kPanelPaddingXs - titleSize.height) / 2, titleSize.width + 1, titleSize.height});
    const double top = bounds.y + header + kPanelPaddingMd;
    const double width = std::max(0.0, bounds.width - 2 * kPanelPadding);
    content_->arrange(Rect{bounds.x + kPanelPadding, top, width, content_->measure(Size{width, 0}).height});
  }

 private:
  /** 标题行的高度：上边留白加上标题与按钮里最高的一个。 */
  [[nodiscard]] double headerHeight() {
    double tallest = std::max(title_->measure(Size{}).height, closeButton_->measure(Size{}).height);
    if (autoButton_ != nullptr) tallest = std::max(tallest, autoButton_->measure(Size{}).height);
    return kPanelPaddingXs + tallest;
  }

  Label* title_{nullptr};
  Button* autoButton_{nullptr};
  Button* closeButton_{nullptr};
  Widget* content_{nullptr};
};

}  // namespace

ShellView::ShellView(Root& root, MainWindowView& view, contracts::ShellModel& model, std::function<void(std::function<void()>)> post) : root_(root), view_(view), model_(model), post_(std::move(post)) {
  center_ = std::make_unique<NotificationCenter>(root_);
  tooltip_ = std::make_unique<Tooltip>(root_);
  view_.frame().setNotificationCenter(center_.get());
  menuBar_ = view_.titleBar().setMenuBar(std::make_unique<MenuBar>());
  menuBar_->setOnSelect([this](const std::string&, const std::string& itemId, std::optional<bool>) { model_.selectMenuItem(itemId); });
  menuBar_->setOnOpenChange([this](const std::string& menuId) {
    if (!menuId.empty()) return;
    // 菜单关闭后把焦点还给画面区，并补上菜单打开期间没更新的内容；重建打开着的菜单时中间会闭合一次，那时不还
    scheduleRefresh();
    const auto alive = alive_;
    post_([this, alive] {
      if (*alive && focusScreen_ && menuBar_->openMenuId().empty()) focusScreen_();
    });
  });
  model_.setOnChanged([this] { scheduleRefresh(); });
}

ShellView::~ShellView() {
  *alive_ = false;
  model_.setOnChanged({});
  view_.frame().setNotificationCenter(nullptr);
}

void ShellView::setScreen(std::unique_ptr<Widget> screen) { view_.frame().setPage(std::move(screen)); }

void ShellView::setPanelContent(const std::string& panelId, WidgetFactory factory) { panelFactories_[panelId] = std::move(factory); }

void ShellView::notify(const std::string& text, contracts::NotificationLevel level, contracts::NotificationAction action) {
  center_->notify(text, levelOf(level), NoticeAction{std::move(action.label), std::move(action.run)});
}

void ShellView::scheduleRefresh() {
  if (refreshPending_) return;
  refreshPending_ = true;
  const auto alive = alive_;
  post_([this, alive] {
    if (!*alive) return;
    refreshPending_ = false;
    refresh();
  });
}

void ShellView::refresh() {
  rebuildMenus();
  rebuildStatusBar();
  rebuildTitleBarItems();
  rebuildPanel();
  applyFullScreen();
}

void ShellView::rebuildMenus() {
  // 菜单打开着时也更新：控件库按新内容重建弹层并保留高亮项（勾选项切换后菜单保持打开，勾选标记要立刻更新）
  const auto menus = model_.menus();
  std::string signature;
  for (const auto& menu : menus) {
    signature += menu.id + '\x1e';
    for (const auto& entry : menu.items) appendSignature(signature, entry);
  }
  if (signature == menusSignature_) return;
  menusSignature_ = signature;
  std::vector<MenuBarMenu> converted;
  for (const auto& menu : menus) {
    MenuBarMenu target;
    target.id = menu.id;
    target.label = menu.label;
    target.mnemonic = std::string(1, menu.mnemonic);
    for (const auto& entry : menu.items) target.items.push_back(toMenuItem(entry));
    converted.push_back(std::move(target));
  }
  menuBar_->setMenus(std::move(converted));
}

void ShellView::rebuildStatusBar() {
  StatusBar* bar = view_.frame().statusBar();
  if (bar == nullptr) return;
  const auto buttons = model_.panelButtons();
  const auto start = model_.statusItems(false);
  const auto end = model_.statusItems(true);
  std::string signature;
  for (const auto& button : buttons) {
    signature += button.id + '\x1f' + button.hint + '\x1f' + std::to_string(static_cast<int>(button.icon)) + std::to_string(static_cast<int>(button.tone)) + (button.pressed ? 'p' : '-');
    if (button.lamp) signature += (button.lamp->lit ? 'L' : 'l') + button.lamp->label;
    signature += '\x1e';
  }
  const auto addItems = [&signature](const std::vector<contracts::ShellStatusItemModel>& items) {
    for (const auto& item : items) signature += item.id + '\x1f' + item.text + '\x1f' + item.title + std::to_string(static_cast<int>(item.tone)) + '\x1e';
    signature += '\x1d';
  };
  addItems(start);
  addItems(end);
  if (signature == statusSignature_) return;
  statusSignature_ = signature;

  bar->clear();
  panelButtons_.clear();
  for (const auto& button : buttons) {
    auto item = std::make_unique<StatusBarItem>(button.title);
    item->setIcon(iconOf(button.icon));
    item->setIconOnly(true);
    item->setTone(toneOf(button.tone));
    item->setTooltip(button.hint);
    item->setPressed(button.pressed);
    if (button.lamp) {
      StatusLamp lamp;
      lamp.lit = button.lamp->lit;
      lamp.label = button.lamp->label;
      if (button.lamp->inIcon) lamp.inIcon = Point{button.lamp->inIcon->first, button.lamp->inIcon->second};
      item->setLamp(lamp);
    }
    const std::string id = button.id;
    item->setOnClick([this, id] { model_.togglePanel(id); });
    panelButtons_.push_back(bar->addStart(std::move(item)));
  }
  const auto addStatusItem = [this, bar](const contracts::ShellStatusItemModel& model, bool atEnd) {
    auto item = std::make_unique<StatusBarItem>(model.text);
    item->setTone(toneOf(model.tone));
    if (!model.title.empty()) item->setTooltip(model.title);
    if (model.clickable) {
      const std::string id = model.id;
      item->setOnClick([this, id] { model_.clickStatusItem(id); });
    }
    if (atEnd) bar->addEnd(std::move(item));
    else bar->addStart(std::move(item));
  };
  for (const auto& item : start) addStatusItem(item, false);
  for (const auto& item : end) addStatusItem(item, true);
  auto notificationButton = std::make_unique<NotificationButton>(*center_);
  center_->setToggleWidget(notificationButton.get());
  bar->addEndWidget(std::move(notificationButton));
}

void ShellView::rebuildTitleBarItems() {
  const auto items = model_.titleBarItems();
  std::string signature;
  for (const auto& item : items) signature += item.id + '\x1f' + item.text + '\x1f' + item.hint + '\x1e';
  if (signature == titleSignature_) return;
  titleSignature_ = signature;
  TitleBar& titleBar = view_.titleBar();
  titleBar.clearActions();
  for (const auto& item : items) {
    auto button = std::make_unique<Button>(item.text);
    button->setVariant(ButtonVariant::Primary);
    button->setCompact(true);
    if (!item.hint.empty()) button->setTooltip(item.hint);
    const std::string id = item.id;
    button->setOnClick([this, id] { model_.clickTitleBarItem(id); });
    titleBar.addAction(std::move(button));
  }
}

void ShellView::rebuildPanel() {
  AppFrame& frame = view_.frame();
  const auto open = model_.openPanel();
  const std::string signature = open ? open->id + '\x1f' + (open->autoCollapseOn ? 'a' : '-') : std::string();
  if (signature == panelSignature_) return;
  panelSignature_ = signature;
  if (!open) {
    // 面板收起：内容卸载
    frame.setBottomPanel(std::make_unique<Widget>());
    frame.setBottomPanelOpen(false);
    return;
  }
  const auto factory = panelFactories_.find(open->id);
  std::unique_ptr<Widget> content = factory != panelFactories_.end() ? factory->second() : std::make_unique<Widget>();
  const std::string id = open->id;
  const bool autoCollapseOn = open->autoCollapseOn;
  auto panel = std::make_unique<BottomPanelView>(*open, [this, id, autoCollapseOn] { model_.setPanelAutoCollapse(id, !autoCollapseOn); }, [this] { model_.closePanel(); }, std::move(content));
  // 先挂到窗口上再量高度：没有宿主时文字量不出来
  BottomPanelView* attached = panel.get();
  frame.setBottomPanel(std::move(panel));
  const double width = std::max(1.0, root_.size().width);
  frame.setBottomPanelMinHeight(0);
  frame.setBottomPanelHeight(attached->measure(Size{width, 0}).height + kPanelBorder);
  frame.setBottomPanelOpen(!fullScreen_);
}

void ShellView::setFullScreen(bool fullScreen) {
  if (fullScreen_ == fullScreen) return;
  fullScreen_ = fullScreen;
  applyFullScreen();
}

void ShellView::applyFullScreen() {
  AppFrame& frame = view_.frame();
  frame.titleBar()->setVisible(!fullScreen_);
  if (frame.statusBar() != nullptr) frame.statusBar()->setVisible(!fullScreen_);
  frame.setBottomPanelOpen(model_.openPanel().has_value() && !fullScreen_);
  root_.layout(root_.size());
}

bool ShellView::handleKeyDown(int virtualKey, unsigned modifiers, bool repeat) {
  if (center_->handleEscape(virtualKey)) return true;
  if (repeat) return false;
  const auto shortcut = model_.shortcutFromKey(virtualKey, modifiers);
  if (!shortcut) return false;
  if (const auto menu = model_.menuForShortcut(*shortcut)) {
    if (!fullScreen_) menuBar_->openMenu(*menu);
    return true;
  }
  // 焦点在输入控件里时，不带 Ctrl 或 Alt 的快捷键（如 F7）留给控件自己
  const Widget* focused = root_.focusedWidget();
  const bool textEditing = focused != nullptr && focused->acceptsText();
  return model_.runShortcut(*shortcut, textEditing);
}

void ShellView::handleMouseDown(Point position) {
  if (!model_.openPanel()) return;
  AppFrame& frame = view_.frame();
  // 不算点在面板之外的：面板本身、标题栏（含菜单）、弹层、系统栏里的面板按钮
  if (frame.bottomRegion()->visible() && frame.bottomRegion()->bounds().contains(position)) return;
  if (frame.titleBar()->visible() && frame.titleBar()->bounds().contains(position)) return;
  for (const Rect& rect : layerRects_) {
    if (rect.contains(position)) return;
  }
  for (const Widget* button : panelButtons_) {
    if (button->bounds().contains(position)) return;
  }
  model_.pressedOutsidePanel();
}

void ShellView::handleMouseUp() {
  const auto alive = alive_;
  // 延到点击处理之后，不影响点击本身的效果；菜单打开着时（要用键盘操作）不还，文字输入控件上不还
  post_([this, alive] {
    if (!*alive || !focusScreen_ || !menuBar_->openMenuId().empty()) return;
    const Widget* focused = root_.focusedWidget();
    if (focused != nullptr && focused->acceptsText()) return;
    focusScreen_();
  });
}

}  // namespace ember::win32::ui
