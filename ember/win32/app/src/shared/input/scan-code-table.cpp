/*
 * ------------------------------------------------------------------------
 * 名称：scan-code-table.cpp
 * 说明：扫描码对照表的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "shared/input/scan-code-table.hpp"

namespace ember::win32::shared {

namespace {

/** 无前缀的扫描码，下标是扫描码；没有收录的为空。 */
constexpr std::string_view kPlainCodes[] = {
    "",           "Escape",    "Digit1",    "Digit2",     "Digit3",         "Digit4",     "Digit5",       "Digit6",    "Digit7",     "Digit8",        "Digit9",
    "Digit0",     "Minus",     "Equal",     "Backspace",  "Tab",            "KeyQ",       "KeyW",         "KeyE",      "KeyR",       "KeyT",          "KeyY",
    "KeyU",       "KeyI",      "KeyO",      "KeyP",       "BracketLeft",    "BracketRight", "Enter",      "ControlLeft", "KeyA",     "KeyS",          "KeyD",
    "KeyF",       "KeyG",      "KeyH",      "KeyJ",       "KeyK",           "KeyL",       "Semicolon",    "Quote",     "Backquote",  "ShiftLeft",     "Backslash",
    "KeyZ",       "KeyX",      "KeyC",      "KeyV",       "KeyB",           "KeyN",       "KeyM",         "Comma",     "Period",     "Slash",         "ShiftRight",
    "NumpadMultiply", "AltLeft", "Space",   "CapsLock",   "F1",             "F2",         "F3",           "F4",        "F5",         "F6",            "F7",
    "F8",         "F9",        "F10",       "NumLock",    "ScrollLock",     "Numpad7",    "Numpad8",      "Numpad9",   "NumpadSubtract", "Numpad4",   "Numpad5",
    "Numpad6",    "NumpadAdd", "Numpad1",   "Numpad2",    "Numpad3",        "Numpad0",    "NumpadDecimal"};
constexpr std::uint16_t kPlainCount = sizeof(kPlainCodes) / sizeof(kPlainCodes[0]);

}  // namespace

std::optional<std::string_view> scanCodeToKeyCode(std::uint16_t make, bool extended, bool prefixE1) {
  if (prefixE1) return std::nullopt;
  if (extended) {
    switch (make) {
      case 0x1c: return "NumpadEnter";
      case 0x1d: return "ControlRight";
      case 0x35: return "NumpadDivide";
      case 0x37: return "PrintScreen";
      case 0x38: return "AltRight";
      case 0x47: return "Home";
      case 0x48: return "ArrowUp";
      case 0x49: return "PageUp";
      case 0x4b: return "ArrowLeft";
      case 0x4d: return "ArrowRight";
      case 0x4f: return "End";
      case 0x50: return "ArrowDown";
      case 0x51: return "PageDown";
      case 0x52: return "Insert";
      case 0x53: return "Delete";
      case 0x5b: return "MetaLeft";
      case 0x5c: return "MetaRight";
      case 0x5d: return "ContextMenu";
      default: return std::nullopt;
    }
  }
  if (make < kPlainCount) {
    const std::string_view name = kPlainCodes[make];
    if (!name.empty()) return name;
    return std::nullopt;
  }
  switch (make) {
    case 0x56: return "IntlBackslash";
    case 0x57: return "F11";
    case 0x58: return "F12";
    default: return std::nullopt;
  }
}

unsigned commandModifierBit(std::string_view code) {
  if (code == "ControlLeft") return 1u << 0;
  if (code == "ControlRight") return 1u << 1;
  if (code == "AltLeft") return 1u << 2;
  if (code == "AltRight") return 1u << 3;
  if (code == "MetaLeft") return 1u << 4;
  if (code == "MetaRight") return 1u << 5;
  return 0;
}

bool isShellKey(std::string_view code, unsigned commandModifiers) {
  if (commandModifierBit(code) != 0 || code == "ShiftLeft" || code == "ShiftRight") return false;
  if (code == "Tab" || code == "Escape") return true;
  // F1 到 F24：F 加数字
  const bool functionKey = code.size() >= 2 && code.front() == 'F' && code.find_first_not_of("0123456789", 1) == std::string_view::npos;
  return functionKey || commandModifiers != 0;
}

bool isAlwaysShellKey(std::string_view code) { return code == "F9" || code == "F10" || code == "F11" || code == "F12"; }

}  // namespace ember::win32::shared
