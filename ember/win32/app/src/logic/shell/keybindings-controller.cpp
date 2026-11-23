/*
 * ------------------------------------------------------------------------
 * 名称：keybindings-controller.cpp
 * 说明：快捷键控制器的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-24
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "logic/shell/keybindings-controller.hpp"

#include <set>

namespace ember::win32::logic {

namespace {

/** Esc 的虚拟键码：等用户按键时按它取消。 */
constexpr int kVirtualKeyEscape = 0x1b;
/** 不在任何菜单里的命令放的一组。 */
constexpr const char* kOtherGroupId = "other";
constexpr const char* kOtherGroupLabel = "其他";

}  // namespace

KeybindingsController::KeybindingsController(const std::map<std::string, contracts::ShellCommandDefinition>& commands, const std::vector<contracts::ShellMenuItemDefinition>& menuItems, contracts::ShellStateService& state,
                                             std::function<void()> onChanged)
    : commands_(commands), state_(state), onChanged_(std::move(onChanged)) {
  for (const auto& menu : contracts::kShellMenus) mnemonics_.push_back(menu.mnemonic);

  // 分组与顺序按菜单里的位置；不在任何菜单里的命令放在最后一组
  std::set<std::string> placed;
  for (const auto& menu : contracts::kShellMenus) {
    Group group{menu.id, menu.label, {}};
    for (const auto& item : menuItems) {
      if (item.menu != menu.id || item.command.empty() || commands.find(item.command) == commands.end() || !placed.insert(item.command).second) continue;
      group.commands.push_back(item.command);
    }
    if (!group.commands.empty()) groups_.push_back(std::move(group));
  }
  Group other{kOtherGroupId, kOtherGroupLabel, {}};
  for (const auto& [id, command] : commands) {
    if (placed.insert(id).second) other.commands.push_back(id);
  }
  if (!other.commands.empty()) groups_.push_back(std::move(other));
  for (const Group& group : groups_) {
    for (const std::string& id : group.commands) order_.push_back({id, commands.at(id).defaultShortcut});
  }

  overrides_ = state_.get().layout.keybindings;
  recompute();
}

std::string KeybindingsController::shortcutOf(const std::string& commandId) const {
  const auto found = effective_.find(commandId);
  return found == effective_.end() ? std::string() : found->second;
}

std::optional<std::string> KeybindingsController::commandFor(const std::string& shortcut) const {
  if (shortcut.empty()) return std::nullopt;
  for (const auto& [id, used] : effective_) {
    if (used == shortcut) return id;
  }
  return std::nullopt;
}

contracts::KeybindingsView KeybindingsController::view() const {
  contracts::KeybindingsView result;
  for (const Group& group : groups_) {
    contracts::KeybindingGroupView groupView{group.id, group.label, {}};
    for (const std::string& id : group.commands) {
      contracts::KeybindingRowView row;
      row.id = id;
      row.label = labelOf(id);
      row.shortcut = shortcutOf(id);
      row.changed = row.shortcut != defaultOf(id);
      row.capturing = id == capturing_;
      if (id == errorCommand_) row.error = errorText_;
      result.anyChanged = result.anyChanged || row.changed;
      groupView.rows.push_back(std::move(row));
    }
    result.groups.push_back(std::move(groupView));
  }
  return result;
}

void KeybindingsController::startCapture(const std::string& commandId) {
  if (commands_.find(commandId) == commands_.end()) return;
  capturing_ = commandId;
  errorCommand_.clear();
  errorText_.clear();
  notify();
}

void KeybindingsController::cancelCapture() {
  if (capturing_.empty()) return;
  capturing_.clear();
  notify();
}

bool KeybindingsController::shortcutKeyPressed(int virtualKey, unsigned modifiers) {
  if (capturing_.empty()) return false;
  if (virtualKey == kVirtualKeyEscape) {
    cancelCapture();
    return true;
  }
  // 只按了修饰键等不能构成快捷键的键：继续等
  const auto shortcut = shared::shortcutFromKey(virtualKey, modifiers);
  if (!shortcut) return true;
  const std::string commandId = capturing_;
  capturing_.clear();
  assign(commandId, *shortcut);
  return true;
}

void KeybindingsController::clear(const std::string& commandId) { assign(commandId, std::string()); }

void KeybindingsController::reset(const std::string& commandId) {
  if (commands_.find(commandId) == commands_.end()) return;
  errorCommand_.clear();
  errorText_.clear();
  const std::string defaultShortcut = defaultOf(commandId);
  if (const auto owner = commandFor(defaultShortcut); owner && *owner != commandId) {
    fail(commandId, "默认的 " + defaultShortcut + " 已用于“" + labelOf(*owner) + "”");
    return;
  }
  if (overrides_.erase(commandId) == 0 && shortcutOf(commandId) == defaultShortcut) {
    notify();
    return;
  }
  apply();
}

void KeybindingsController::resetAll() {
  capturing_.clear();
  errorCommand_.clear();
  errorText_.clear();
  overrides_.clear();
  apply();
}

std::function<void()> KeybindingsController::subscribe(std::function<void()> handler) {
  const int id = nextListener_++;
  listeners_[id] = std::move(handler);
  return [this, id] { listeners_.erase(id); };
}

void KeybindingsController::assign(const std::string& commandId, const std::string& shortcut) {
  if (commands_.find(commandId) == commands_.end()) return;
  errorCommand_.clear();
  errorText_.clear();
  if (!shortcut.empty()) {
    if (const auto reason = shared::checkShortcut(shortcut, mnemonics_)) {
      fail(commandId, *reason);
      return;
    }
    if (const auto owner = commandFor(shortcut); owner && *owner != commandId) {
      fail(commandId, shortcut + " 已用于“" + labelOf(*owner) + "”");
      return;
    }
  }
  if (shortcut == defaultOf(commandId)) overrides_.erase(commandId);
  else overrides_[commandId] = shortcut;
  apply();
}

void KeybindingsController::fail(const std::string& commandId, const std::string& text) {
  errorCommand_ = commandId;
  errorText_ = text;
  notify();
}

void KeybindingsController::recompute() {
  // 命令已不存在的修改与和默认相同的修改先丢弃，再算实际的快捷键
  for (auto it = overrides_.begin(); it != overrides_.end();) {
    if (commands_.find(it->first) == commands_.end() || it->second == defaultOf(it->first)) it = overrides_.erase(it);
    else ++it;
  }
  effective_ = shared::resolveShortcuts(order_, overrides_, mnemonics_);
  // 写法不合格或与别的修改重复、没有被采用的修改也丢弃
  for (auto it = overrides_.begin(); it != overrides_.end();) {
    if (shortcutOf(it->first) != it->second) it = overrides_.erase(it);
    else ++it;
  }
}

void KeybindingsController::apply() {
  recompute();
  shared::ShellLayout layout = state_.get().layout;
  layout.keybindings = overrides_;
  state_.updateLayout(layout);
  if (onChanged_) onChanged_();
  notify();
}

void KeybindingsController::notify() {
  // 回调里可能取消订阅，先复制一份再逐个调用
  const auto listeners = listeners_;
  for (const auto& [id, listener] : listeners) {
    if (listeners_.find(id) != listeners_.end()) listener();
  }
}

std::string KeybindingsController::labelOf(const std::string& commandId) const {
  const auto found = commands_.find(commandId);
  if (found == commands_.end()) return commandId;
  if (!found->second.settingsTitle.empty()) return found->second.settingsTitle;
  return found->second.title ? found->second.title() : commandId;
}

std::string KeybindingsController::defaultOf(const std::string& commandId) const {
  const auto found = commands_.find(commandId);
  return found == commands_.end() ? std::string() : found->second.defaultShortcut;
}

}  // namespace ember::win32::logic
