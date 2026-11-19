/*
 * ------------------------------------------------------------------------
 * 名称：shortcut.hpp
 * 说明：快捷键的写法与检查：把按键事件写成“Ctrl+Shift+K”这样的文字，检查登记的默认快捷键是否合格。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：写法与组件库的 shortcutFromEvent 一致：修饰键按 Ctrl、Shift、Alt 排列，再接一个按键；检查规则：
 *       写法合格、不与保留组合（Esc、Alt+F4、Ctrl+A 等）和菜单的 Alt+助记字母冲突、必须含 Ctrl 或 Alt（F1 至 F24 除外）。
 * ------------------------------------------------------------------------
 */

#pragma once

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

/**
 * 检查默认快捷键。
 * @param shortcut 快捷键文字。
 * @param menuMnemonics 菜单的助记字母，Alt 加它们不能作为快捷键。
 * @returns 合格时没有值，不合格时是原因。
 */
[[nodiscard]] std::optional<std::string> checkDefaultShortcut(const std::string& shortcut, const std::vector<char>& menuMnemonics);

}  // namespace ember::win32::shared
