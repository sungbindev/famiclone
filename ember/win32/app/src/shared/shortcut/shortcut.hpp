/*
 * ------------------------------------------------------------------------
 * 名称：shortcut.hpp
 * 说明：快捷键的写法与检查：把按键事件写成“Ctrl+Shift+K”这样的文字，检查登记的默认快捷键与用户设置的快捷键是否合格，算出每条命令实际使用的快捷键。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：写法与组件库的 shortcutFromEvent 一致：修饰键按 Ctrl、Shift、Alt 排列，再接一个按键；检查规则：
 *       写法合格、不与保留组合（Esc、Alt+F4、Ctrl+A 等）和菜单的 Alt+助记字母冲突、必须含 Ctrl 或 Alt（F1 至 F24 除外）。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace ember::win32::shared {

/** 修饰键的位，取值与控件库的 Modifier 一致。 */
inline constexpr unsigned kShortcutShift = 1;
inline constexpr unsigned kShortcutCtrl = 2;
inline constexpr unsigned kShortcutAlt = 4;

/**
 * 按键事件写成快捷键文字。
 * @param virtualKey Windows 虚拟键码。
 * @param modifiers 修饰键组合（kShortcutShift 等的和）。
 * @returns 快捷键文字；修饰键本身、Win 键与不认识的键没有值。
 */
[[nodiscard]] std::optional<std::string> shortcutFromKey(int virtualKey, unsigned modifiers);

/** 快捷键文字里是否含 Ctrl 或 Alt。 */
[[nodiscard]] bool shortcutHasCtrlOrAlt(const std::string& shortcut);

/** 快捷键文字的写法是否合格：修饰键按 Ctrl、Shift、Alt 排列，最后是一个合格的按键名；不看保留组合与必须含 Ctrl 或 Alt。 */
[[nodiscard]] bool isValidShortcutText(const std::string& shortcut);

/**
 * 检查快捷键（登记的默认快捷键与用户设置的快捷键用同一套规则）。
 * @param shortcut 快捷键文字。
 * @param menuMnemonics 菜单的助记字母，Alt 加它们不能作为快捷键。
 * @returns 合格时没有值，不合格时是原因。
 */
[[nodiscard]] std::optional<std::string> checkShortcut(const std::string& shortcut, const std::vector<char>& menuMnemonics);

/** 一条命令与它的默认快捷键（没有时为空）。 */
struct CommandShortcut {
  std::string id;
  std::string defaultShortcut;
};

/**
 * 算出每条命令实际使用的快捷键。
 * 用户的修改（空串表示清除）在命令不存在、写法或规则不合格、与前面已采用的修改重复时不采用；没有被修改的命令用默认快捷键，默认快捷键被别的命令的修改占用时没有快捷键。
 * @param commands 全部命令，按这个顺序处理。
 * @param overrides 用户的修改，命令标识到快捷键。
 * @param menuMnemonics 菜单的助记字母。
 * @returns 命令标识到实际快捷键（空串表示没有），每条命令都有一项。
 */
[[nodiscard]] std::map<std::string, std::string> resolveShortcuts(const std::vector<CommandShortcut>& commands, const std::map<std::string, std::string>& overrides, const std::vector<char>& menuMnemonics);

}  // namespace ember::win32::shared
