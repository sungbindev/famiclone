/*
 * ------------------------------------------------------------------------
 * 名称：shortcut.cpp
 * 说明：快捷键写法与检查的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：虚拟键码用数值写（A 至 Z 与数字键就是字符码，F1 为 0x70），不包含系统头文件。
 * ------------------------------------------------------------------------
 */

#include "shared/shortcut/shortcut.hpp"

#include <algorithm>
#include <cctype>
#include <iterator>
#include <set>
#include <string_view>

namespace ember::win32::shared {

namespace {

/** 保留给系统与文本编辑的组合。 */
const char* const kReserved[] = {"Esc", "Alt+F4", "Ctrl+A", "Ctrl+C", "Ctrl+V", "Ctrl+X", "Ctrl+Z", "Ctrl+Y"};

/** 虚拟键码对应的按键名；没有对应的为空。 */
std::string keyName(int virtualKey) {
  if ((virtualKey >= 'A' && virtualKey <= 'Z') || (virtualKey >= '0' && virtualKey <= '9')) return std::string(1, static_cast<char>(virtualKey));
  if (virtualKey >= 0x70 && virtualKey <= 0x87) return "F" + std::to_string(virtualKey - 0x70 + 1);
  switch (virtualKey) {
    case 0x1b: return "Esc";
    case 0x20: return "Space";
    case 0x0d: return "Enter";
    case 0x09: return "Tab";
    case 0x08: return "Backspace";
    case 0x2e: return "Delete";
    case 0x2d: return "Insert";
    case 0x24: return "Home";
    case 0x23: return "End";
    case 0x21: return "PageUp";
    case 0x22: return "PageDown";
    case 0x26: return "Up";
    case 0x28: return "Down";
    case 0x25: return "Left";
    case 0x27: return "Right";
    case 0xba: return ";";
    case 0xbb: return "=";
    case 0xbc: return ",";
    case 0xbd: return "-";
    case 0xbe: return ".";
    case 0xbf: return "/";
    case 0xc0: return "`";
    case 0xdb: return "[";
    case 0xdc: return "\\";
    case 0xdd: return "]";
    case 0xde: return "'";
    default: return "";
  }
}

/** 是否是合格的按键名：单个大写字母或数字、F1 至 F24、命名键、标点。 */
bool isValidKeyName(const std::string& key) {
  if (key.size() == 1) {
    const char c = key[0];
    return (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || std::string_view("`-=[]\\;',./").find(c) != std::string_view::npos;
  }
  static const char* const named[] = {"Space", "Enter", "Tab", "Backspace", "Delete", "Insert", "Home", "End", "PageUp", "PageDown", "Up", "Down", "Left", "Right"};
  if (std::any_of(std::begin(named), std::end(named), [&key](const char* name) { return key == name; })) return true;
  if (key.size() >= 2 && key[0] == 'F') {
    const std::string digits = key.substr(1);
    if (!std::all_of(digits.begin(), digits.end(), [](unsigned char c) { return std::isdigit(c) != 0; }) || digits[0] == '0') return false;
    const int number = std::stoi(digits);
    return number >= 1 && number <= 24;
  }
  return false;
}

/** 是否是 F1 至 F24。 */
bool isFunctionKey(const std::string& key) { return key.size() >= 2 && key[0] == 'F' && isValidKeyName(key); }

/** 把快捷键文字拆成各段。 */
std::vector<std::string> split(const std::string& text) {
  std::vector<std::string> parts;
  std::size_t start = 0;
  while (true) {
    const std::size_t plus = text.find('+', start);
    // 按键本身是“+”不在合格的按键名里，所以每个加号都是分隔符；末尾的加号会得到空段，由合格检查拒绝
    if (plus == std::string::npos) {
      parts.push_back(text.substr(start));
      return parts;
    }
    parts.push_back(text.substr(start, plus - start));
    start = plus + 1;
  }
}

}  // namespace

std::optional<std::string> shortcutFromKey(int virtualKey, unsigned modifiers) {
  const std::string key = keyName(virtualKey);
  if (key.empty()) return std::nullopt;
  std::string text;
  if ((modifiers & kShortcutCtrl) != 0) text += "Ctrl+";
  if ((modifiers & kShortcutShift) != 0) text += "Shift+";
  if ((modifiers & kShortcutAlt) != 0) text += "Alt+";
  return text + key;
}

bool shortcutHasCtrlOrAlt(const std::string& shortcut) {
  const auto parts = split(shortcut);
  return std::find(parts.begin(), parts.end(), "Ctrl") != parts.end() || std::find(parts.begin(), parts.end(), "Alt") != parts.end();
}

bool isValidShortcutText(const std::string& shortcut) {
  const auto parts = split(shortcut);
  // 修饰键必须按 Ctrl、Shift、Alt 的顺序，最后一段是按键
  std::size_t index = 0;
  for (const char* modifier : {"Ctrl", "Shift", "Alt"}) {
    if (index + 1 < parts.size() && parts[index] == modifier) ++index;
  }
  return index + 1 == parts.size() && isValidKeyName(parts.back());
}

std::optional<std::string> checkShortcut(const std::string& shortcut, const std::vector<char>& menuMnemonics) {
  if (!isValidShortcutText(shortcut)) return "写法不合格，应写成 Ctrl+Shift+K 这样的形式（修饰键按 Ctrl、Shift、Alt 排列）";
  const auto parts = split(shortcut);
  if (std::any_of(std::begin(kReserved), std::end(kReserved), [&shortcut](const char* reserved) { return shortcut == reserved; })) return "留给系统、文本编辑或菜单，不能使用";
  if (std::any_of(menuMnemonics.begin(), menuMnemonics.end(), [&shortcut](char mnemonic) { return shortcut == std::string("Alt+") + mnemonic; })) return "留给系统、文本编辑或菜单，不能使用";
  if (!shortcutHasCtrlOrAlt(shortcut) && !isFunctionKey(parts.back())) return "必须包含 Ctrl 或 Alt（F1 至 F24 除外）";
  return std::nullopt;
}

std::map<std::string, std::string> resolveShortcuts(const std::vector<CommandShortcut>& commands, const std::map<std::string, std::string>& overrides, const std::vector<char>& menuMnemonics) {
  std::map<std::string, std::string> result;
  // 被用户的修改占用的快捷键
  std::set<std::string> taken;
  for (const CommandShortcut& command : commands) {
    const auto found = overrides.find(command.id);
    if (found == overrides.end()) continue;
    const std::string& shortcut = found->second;
    if (!shortcut.empty() && (checkShortcut(shortcut, menuMnemonics).has_value() || !taken.insert(shortcut).second)) continue;
    result[command.id] = shortcut;
  }
  for (const CommandShortcut& command : commands) {
    if (result.find(command.id) != result.end()) continue;
    result[command.id] = taken.count(command.defaultShortcut) != 0 ? std::string() : command.defaultShortcut;
  }
  return result;
}

}  // namespace ember::win32::shared
