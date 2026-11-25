/*
 * ------------------------------------------------------------------------
 * 名称：keyboard-apple2.cpp
 * 说明：苹果 II 类机器键盘的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-25
 * 备注：无
 * ------------------------------------------------------------------------
 */

#include "logic/hardware/keyboard-apple2/keyboard-apple2.hpp"

#include <algorithm>
#include <array>

namespace ember::logic::hardware {

namespace {

/** 一个键位：不按 Shift 与按 Shift 的 ASCII 码。 */
struct KeyEntry {
  std::string_view code;
  std::uint8_t plain;
  std::uint8_t shifted;
};

constexpr std::array<KeyEntry, 32> SYMBOL_KEYS{{
    {"Digit1", '1', '!'},        {"Digit2", '2', '@'},     {"Digit3", '3', '#'},      {"Digit4", '4', '$'},     {"Digit5", '5', '%'},         {"Digit6", '6', '^'},
    {"Digit7", '7', '&'},        {"Digit8", '8', '*'},     {"Digit9", '9', '('},      {"Digit0", '0', ')'},     {"Minus", '-', '_'},          {"Equal", '=', '+'},
    {"BracketLeft", '[', '{'},   {"BracketRight", ']', '}'}, {"Backslash", '\\', '|'}, {"Semicolon", ';', ':'}, {"Quote", '\'', '"'},         {"Comma", ',', '<'},
    {"Period", '.', '>'},        {"Slash", '/', '?'},      {"Backquote", '`', '~'},   {"Space", ' ', ' '},      {"Enter", 0x0D, 0x0D},        {"NumpadEnter", 0x0D, 0x0D},
    {"Escape", 0x1B, 0x1B},      {"Tab", 0x09, 0x09},      {"Backspace", 0x08, 0x08}, {"Delete", 0x7F, 0x7F},   {"ArrowLeft", 0x08, 0x08},    {"ArrowRight", 0x15, 0x15},
    {"ArrowUp", 0x0B, 0x0B},     {"ArrowDown", 0x0A, 0x0A},
}};

/** 是 KeyA 到 KeyZ 就返回字母下标（0 到 25），否则 -1。 */
int letterIndex(std::string_view code) {
  if (code.size() == 4 && code.substr(0, 3) == "Key" && code[3] >= 'A' && code[3] <= 'Z') return code[3] - 'A';
  return -1;
}

const KeyEntry* findSymbol(std::string_view code) {
  const auto found = std::find_if(SYMBOL_KEYS.begin(), SYMBOL_KEYS.end(), [code](const KeyEntry& entry) { return entry.code == code; });
  return found == SYMBOL_KEYS.end() ? nullptr : &*found;
}

}  // namespace

int KeyboardApple2::asciiFor(std::string_view code) const {
  const bool control = controlLeft_ || controlRight_;
  const int letter = letterIndex(code);
  if (letter >= 0) {
    if (control) return letter + 1;
    const bool upper = capsLock_ || shiftDown();
    return (upper ? 'A' : 'a') + letter;
  }
  const KeyEntry* entry = findSymbol(code);
  if (entry == nullptr) return -1;
  int value = shiftDown() ? entry->shifted : entry->plain;
  // Ctrl 与 @ 到 _ 之间的字符组合产生控制码。
  if (control && value >= 0x40 && value <= 0x5F) value &= 0x1F;
  return value;
}

bool KeyboardApple2::setKey(std::string_view code, bool down) {
  if (code == "ShiftLeft") {
    shiftLeft_ = down;
    return true;
  }
  if (code == "ShiftRight") {
    shiftRight_ = down;
    return true;
  }
  if (code == "ControlLeft") {
    controlLeft_ = down;
    return true;
  }
  if (code == "ControlRight") {
    controlRight_ = down;
    return true;
  }
  if (code == "AltLeft") {
    openApple_ = down;
    return true;
  }
  if (code == "AltRight") {
    solidApple_ = down;
    return true;
  }
  if (code == "CapsLock") {
    if (down) capsLock_ = !capsLock_;
    return true;
  }
  const int value = asciiFor(code);
  if (value < 0) return false;
  const auto position = std::find(down_.begin(), down_.end(), code);
  if (!down) {
    if (position != down_.end()) down_.erase(position);
    return true;
  }
  if (position == down_.end()) down_.emplace_back(code);
  latch_ = static_cast<std::uint8_t>(value & 0x7F);
  strobe_ = true;
  return true;
}

void KeyboardApple2::releaseAll() {
  down_.clear();
  shiftLeft_ = false;
  shiftRight_ = false;
  controlLeft_ = false;
  controlRight_ = false;
  openApple_ = false;
  solidApple_ = false;
}

void KeyboardApple2::powerOn() {
  releaseAll();
  latch_ = 0;
  strobe_ = false;
  capsLock_ = true;
}

}  // namespace ember::logic::hardware
