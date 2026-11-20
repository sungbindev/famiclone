/*
 * ------------------------------------------------------------------------
 * 名称：shell-controller.cpp
 * 说明：外壳控制器的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "logic/shell/shell-controller.hpp"

#include <algorithm>
#include <cctype>

#include "shared/shortcut/shortcut.hpp"

namespace ember::win32::logic {

namespace {

/** 用户可见的文字。 */
constexpr const char* kUnreadable = "无法读取拖入的文件，请从文件管理器里拖入磁盘上的文件";

std::string unsupportedText(const std::string& name) { return "不支持拖入的文件：" + name + "。可以拖入 NES 卡带、软盘镜像或声音文件"; }

/** 文件名的扩展名（小写、不带点）；没有扩展名时为空。 */
std::string extensionOf(const std::string& name) {
  const std::size_t dot = name.rfind('.');
  if (dot == std::string::npos) return {};
  std::string extension = name.substr(dot + 1);
  std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return extension;
}

}  // namespace

ShellController::ShellController(ShellRegistry registry, contracts::ShellWindowControl& window, contracts::ShellDialogOpener& dialogs, contracts::ShellNotifier& notifier,
                                 contracts::ShellStateService& state, contracts::DroppedFilesService& dropped)
    : registry_(std::move(registry)), window_(window), dialogs_(dialogs), notifier_(notifier), state_(state), dropped_(dropped) {
  const auto& off = state_.get().layout.autoCollapseOffPanels;
  autoCollapseOff_.insert(off.begin(), off.end());
}

ShellController::~ShellController() { detachModules(); }

void ShellController::attachModules() {
  if (!detachers_.empty()) return;
  for (const auto& [module, attach] : registry_.attachments) {
    if (auto detach = attach(*this)) detachers_.push_back(std::move(detach));
  }
}

void ShellController::detachModules() {
  // 后接上的先断开
  for (auto it = detachers_.rbegin(); it != detachers_.rend(); ++it) (*it)();
  detachers_.clear();
}

void ShellController::setWindowState(bool fullScreen, bool alwaysOnTop) {
  if (view_.fullScreen == fullScreen && view_.alwaysOnTop == alwaysOnTop) return;
  view_.fullScreen = fullScreen;
  view_.alwaysOnTop = alwaysOnTop;
  changed();
}

void ShellController::changed() {
  if (onChanged_) onChanged_();
}

void ShellController::openDialog(const std::string& dialogId) {
  if (registry_.dialogs.find(dialogId) == registry_.dialogs.end()) return;
  dialogs_.openDialog(dialogId);
}

void ShellController::togglePanel(const std::string& panelId) {
  if (findPanel(panelId) == nullptr) return;
  if (view_.bottomPanelOpen && view_.activePanelId == panelId) {
    view_.bottomPanelOpen = false;
    view_.activePanelId.clear();
  } else {
    view_.bottomPanelOpen = true;
    view_.activePanelId = panelId;
  }
  changed();
}

void ShellController::closePanel() {
  if (!view_.bottomPanelOpen) return;
  view_.bottomPanelOpen = false;
  view_.activePanelId.clear();
  changed();
}

void ShellController::toggleFullScreen() { window_.toggleFullScreen(); }

void ShellController::toggleAlwaysOnTop() { window_.toggleAlwaysOnTop(); }

void ShellController::quit() { window_.quit(); }

void ShellController::runCommand(const std::string& commandId) {
  const auto found = registry_.commands.find(commandId);
  if (found == registry_.commands.end()) return;
  const auto& command = found->second;
  if (command.enabled && !command.enabled(view_)) return;
  if (command.run) command.run(*this);
}

void ShellController::refreshCommands() { changed(); }

void ShellController::notify(const std::string& text, contracts::NotificationLevel level) { notifier_.notify(text, level); }

std::vector<contracts::ShellMenuModel> ShellController::menus() {
  std::vector<contracts::ShellMenuModel> result;
  for (const auto& info : contracts::kShellMenus) {
    contracts::ShellMenuModel menu{info.id, info.label, info.mnemonic, {}};
    std::size_t index = 0;
    for (const auto& item : registry_.menuItems) {
      if (item.menu != info.id) continue;
      const bool separator = item.separatorBefore && index > 0;
      ++index;
      contracts::ShellMenuEntry entry;
      entry.mnemonic = item.mnemonic;
      entry.separatorBefore = separator;
      if (item.submenu.has_value()) {
        const auto& submenu = *item.submenu;
        entry.id = submenu.id;
        entry.label = submenu.titleText ? submenu.titleText() : submenu.title;
        const auto shown = submenu.entries ? submenu.entries() : std::vector<contracts::ShellSubmenuEntry>{};
        // 没有子菜单项时这一项置灰
        if (shown.empty()) {
          entry.disabled = true;
        } else {
          for (const auto& sub : shown) {
            contracts::ShellMenuEntry child;
            child.id = submenu.id + "." + sub.id;
            child.label = sub.label;
            child.hint = sub.hint;
            child.disabled = sub.disabled;
            child.separatorBefore = sub.separatorBefore;
            if (sub.checked.has_value()) {
              child.checked = *sub.checked;
              child.radio = true;
            }
            entry.items.push_back(std::move(child));
          }
        }
      } else {
        const auto found = registry_.commands.find(item.command);
        if (found == registry_.commands.end()) continue;
        const auto& command = found->second;
        entry.id = command.id;
        entry.label = command.title ? command.title() : command.id;
        entry.shortcut = command.defaultShortcut;
        if (command.checked) entry.checked = command.checked(view_);
        entry.disabled = command.enabled && !command.enabled(view_);
      }
      menu.items.push_back(std::move(entry));
    }
    if (!menu.items.empty()) result.push_back(std::move(menu));
  }
  return result;
}

void ShellController::selectMenuItem(const std::string& itemId) {
  // 子菜单的项按“子菜单标识.项标识”找到并执行（项随数据变化，选中时重新取），其余按命令执行
  for (const auto& item : registry_.menuItems) {
    if (!item.submenu.has_value() || itemId.rfind(item.submenu->id + ".", 0) != 0) continue;
    if (!item.submenu->entries) return;
    for (const auto& entry : item.submenu->entries()) {
      if (item.submenu->id + "." + entry.id != itemId) continue;
      if (!entry.disabled && entry.run) entry.run(*this);
      return;
    }
    return;
  }
  runCommand(itemId);
}

std::optional<std::string> ShellController::menuForShortcut(const std::string& shortcut) {
  for (const auto& info : contracts::kShellMenus) {
    if (shortcut != std::string("Alt+") + info.mnemonic) continue;
    const bool hasItems = std::any_of(registry_.menuItems.begin(), registry_.menuItems.end(), [&info](const contracts::ShellMenuItemDefinition& item) { return item.menu == info.id; });
    if (hasItems) return std::string(info.id);
    return std::nullopt;
  }
  return std::nullopt;
}

std::optional<std::string> ShellController::shortcutFromKey(int virtualKey, unsigned modifiers) const { return shared::shortcutFromKey(virtualKey, modifiers); }

bool ShellController::runShortcut(const std::string& shortcut, bool textEditing) {
  // 焦点在输入控件里时，不带 Ctrl 或 Alt 的快捷键（如 F7）留给控件自己
  if (textEditing && !shared::shortcutHasCtrlOrAlt(shortcut)) return false;
  const auto found = registry_.shortcuts.find(shortcut);
  if (found == registry_.shortcuts.end()) return false;
  runCommand(found->second);
  return true;
}

const contracts::ShellPanelDefinition* ShellController::findPanel(const std::string& panelId) const {
  const auto found = std::find_if(registry_.bottomPanels.begin(), registry_.bottomPanels.end(), [&panelId](const contracts::ShellPanelDefinition& panel) { return panel.id == panelId; });
  return found == registry_.bottomPanels.end() ? nullptr : &*found;
}

bool ShellController::autoCollapseOn(const std::string& panelId) const { return autoCollapseOff_.find(panelId) == autoCollapseOff_.end(); }

std::vector<contracts::ShellPanelButton> ShellController::panelButtons() {
  std::vector<contracts::ShellPanelButton> result;
  for (const auto& panel : registry_.bottomPanels) {
    contracts::ShellPanelButton button;
    button.id = panel.id;
    button.title = panel.title;
    contracts::ShellPanelLook look;
    look.icon = panel.icon;
    if (panel.look) look = panel.look();
    button.icon = look.icon;
    button.tone = look.tone;
    button.hint = look.hint.empty() ? panel.title : look.hint;
    button.pressed = view_.bottomPanelOpen && view_.activePanelId == panel.id;
    if (panel.lamp) button.lamp = panel.lamp();
    result.push_back(std::move(button));
  }
  return result;
}

std::vector<contracts::ShellStatusItemModel> ShellController::statusItems(bool alignEnd) {
  std::vector<contracts::ShellStatusItemModel> result;
  for (const auto& item : registry_.statusItems) {
    if (item.alignEnd != alignEnd || !item.view) continue;
    const auto view = item.view();
    if (!view) continue;
    result.push_back({item.id, view->text, view->title, view->tone, static_cast<bool>(item.click)});
  }
  return result;
}

std::vector<contracts::ShellTitleBarItemModel> ShellController::titleBarItems() {
  std::vector<contracts::ShellTitleBarItemModel> result;
  for (const auto& item : registry_.titleBarItems) {
    if (!item.view) continue;
    const auto view = item.view();
    if (view) result.push_back({item.id, view->text, view->hint});
  }
  return result;
}

void ShellController::clickStatusItem(const std::string& itemId) {
  for (const auto& item : registry_.statusItems) {
    if (item.id == itemId && item.click) item.click(*this);
  }
}

void ShellController::clickTitleBarItem(const std::string& itemId) {
  for (const auto& item : registry_.titleBarItems) {
    if (item.id == itemId && item.click) item.click(*this);
  }
}

std::optional<contracts::ShellOpenPanel> ShellController::openPanel() const {
  if (!view_.bottomPanelOpen) return std::nullopt;
  const auto* panel = findPanel(view_.activePanelId);
  if (panel == nullptr) return std::nullopt;
  return contracts::ShellOpenPanel{panel->id, panel->title, panel->autoCollapse, autoCollapseOn(panel->id)};
}

void ShellController::setPanelAutoCollapse(const std::string& panelId, bool on) {
  if (autoCollapseOn(panelId) == on) return;
  if (on) autoCollapseOff_.erase(panelId);
  else autoCollapseOff_.insert(panelId);
  shared::ShellLayout layout = state_.get().layout;
  layout.autoCollapseOffPanels.assign(autoCollapseOff_.begin(), autoCollapseOff_.end());
  state_.updateLayout(layout);
  changed();
}

void ShellController::pressedOutsidePanel() {
  const auto open = openPanel();
  if (!open || !open->autoCollapse || !open->autoCollapseOn) return;
  closePanel();
}

void ShellController::dropFiles(const std::vector<std::string>& paths) {
  if (paths.empty()) return;
  const auto files = dropped_.registerFiles(paths);
  if (files.empty()) notifier_.notify(kUnreadable, contracts::NotificationLevel::Error);
  for (const auto& file : files) {
    const std::string extension = extensionOf(file.name);
    const auto handler = std::find_if(registry_.fileHandlers.begin(), registry_.fileHandlers.end(), [&extension](const contracts::ShellFileHandlerDefinition& candidate) {
      return !extension.empty() && std::find(candidate.extensions.begin(), candidate.extensions.end(), extension) != candidate.extensions.end();
    });
    if (handler == registry_.fileHandlers.end()) notifier_.notify(unsupportedText(file.name), contracts::NotificationLevel::Error);
    else if (handler->open) handler->open(file);
  }
}

}  // namespace ember::win32::logic
