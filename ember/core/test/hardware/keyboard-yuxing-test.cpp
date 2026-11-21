/*
 * ------------------------------------------------------------------------
 * 名称：keyboard-yuxing-test.cpp
 * 说明：裕兴学习机键盘的测试：行选择的低 8 位与高 6 位、被选中各行按列取或、键位表、左右修饰键共用一个位置、复位与全部松开。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <cstdint>

#include "logic/hardware/keyboard-yuxing/keyboard-yuxing.hpp"

namespace {

using ember::logic::hardware::KeyboardYuxing;

TEST(KeyboardYuxing, NothingIsSelectedAfterConstruction) {
  KeyboardYuxing keyboard;
  keyboard.setKey("Space", true);
  EXPECT_EQ(keyboard.rowSelect(), 0);
  EXPECT_EQ(keyboard.read(), 0) << "没有选中任何行时读出 0";
}

TEST(KeyboardYuxing, AKeyShowsUpOnlyWhenItsRowIsSelected) {
  KeyboardYuxing keyboard;
  // 空格在第 4 行第 7 列。
  keyboard.setKey("Space", true);
  keyboard.selectLow(0x10);
  EXPECT_EQ(keyboard.read(), 0x80);
  keyboard.selectLow(0x20);
  EXPECT_EQ(keyboard.read(), 0);
  keyboard.selectLow(0x00);
  EXPECT_EQ(keyboard.read(), 0);
}

TEST(KeyboardYuxing, TheHighRegisterSelectsRowsEightToThirteen) {
  KeyboardYuxing keyboard;
  // Tab 在第 13 行第 6 列，Insert 在第 12 行第 5 列。
  keyboard.setKey("Tab", true);
  keyboard.setKey("Insert", true);
  keyboard.selectHigh(0x20);
  EXPECT_EQ(keyboard.read(), 0x40);
  keyboard.selectHigh(0x10);
  EXPECT_EQ(keyboard.read(), 0x20);
  keyboard.selectHigh(0x30);
  EXPECT_EQ(keyboard.read(), 0x60) << "选中多行时按列取或";
  keyboard.selectHigh(0xC0);
  EXPECT_EQ(keyboard.read(), 0) << "高位寄存器只取低 6 位，位 6、7 不选行";
}

TEST(KeyboardYuxing, TheLowAndHighRegistersAreIndependent) {
  KeyboardYuxing keyboard;
  keyboard.selectLow(0x01);
  keyboard.selectHigh(0x01);
  EXPECT_EQ(keyboard.rowSelect(), 0x0101);
  keyboard.selectLow(0x00);
  EXPECT_EQ(keyboard.rowSelect(), 0x0100);
  keyboard.selectHigh(0x00);
  EXPECT_EQ(keyboard.rowSelect(), 0);
}

TEST(KeyboardYuxing, SeveralKeysInTheSelectedRowsAreCombinedByColumn) {
  KeyboardYuxing keyboard;
  // 第 4 行：F8 列 0、字母 Z 列 5、空格列 7；第 0 行：Esc 列 0。
  keyboard.setKey("F8", true);
  keyboard.setKey("KeyZ", true);
  keyboard.setKey("Space", true);
  keyboard.setKey("Escape", true);
  keyboard.selectLow(0x11);
  EXPECT_EQ(keyboard.read(), 0xA1);
}

TEST(KeyboardYuxing, ReleasingAKeyOrAllKeysClearsTheMatrix) {
  KeyboardYuxing keyboard;
  keyboard.selectLow(0xFF);
  keyboard.selectHigh(0x3F);
  keyboard.setKey("KeyA", true);
  keyboard.setKey("KeyQ", true);
  EXPECT_EQ(keyboard.read(), 0x14);
  keyboard.setKey("KeyA", false);
  EXPECT_EQ(keyboard.read(), 0x04);
  keyboard.releaseAll();
  EXPECT_EQ(keyboard.read(), 0);
}

TEST(KeyboardYuxing, TheLeftAndRightModifiersShareAPositionAndStayDownUntilBothAreReleased) {
  KeyboardYuxing keyboard;
  // Shift 在第 0 行第 7 列，Ctrl 在第 10 行第 5 列，Alt 在第 10 行第 6 列。
  keyboard.selectLow(0xFF);
  keyboard.selectHigh(0x3F);
  keyboard.setKey("ShiftLeft", true);
  keyboard.setKey("ShiftRight", true);
  EXPECT_EQ(keyboard.read(), 0x80);
  keyboard.setKey("ShiftLeft", false);
  EXPECT_EQ(keyboard.read(), 0x80) << "右 Shift 还按着";
  keyboard.setKey("ShiftRight", false);
  EXPECT_EQ(keyboard.read(), 0);
  keyboard.setKey("ControlRight", true);
  keyboard.setKey("AltRight", true);
  EXPECT_EQ(keyboard.read(), 0x60);
}

TEST(KeyboardYuxing, UnknownKeysAreIgnoredAndReported) {
  KeyboardYuxing keyboard;
  EXPECT_FALSE(keyboard.setKey("MediaPlayPause", true));
  EXPECT_TRUE(keyboard.setKey("KeyA", true));
  keyboard.selectLow(0xFF);
  keyboard.selectHigh(0x3F);
  EXPECT_EQ(keyboard.read(), 0x10);
}

TEST(KeyboardYuxing, ResetClearsTheRowSelectionButKeepsTheKeys) {
  KeyboardYuxing keyboard;
  keyboard.setKey("Space", true);
  keyboard.selectLow(0x10);
  keyboard.reset();
  EXPECT_EQ(keyboard.rowSelect(), 0);
  keyboard.selectLow(0x10);
  EXPECT_EQ(keyboard.read(), 0x80);
}

TEST(KeyboardYuxing, EveryKeyOfTheMatrixHasItsOwnPosition) {
  // 把整个键位表按键位名走一遍：每个键独占一个位置（左右修饰键除外），按下后只有它所在的位置有变化。
  const char* const codes[] = {"Escape", "F9", "Digit7", "KeyR", "KeyA", "ShiftLeft", "NumpadEnter", "NumpadMultiply", "NumpadDivide", "ArrowUp", "Backspace", "F12", "NumpadAdd", "NumLock",
                               "ArrowLeft", "ArrowDown", "ArrowRight", "Numpad7", "Numpad8", "NumpadSubtract", "Numpad9", "F11", "End", "PageDown", "F8", "Digit6", "KeyE", "BracketRight",
                               "KeyL", "KeyZ", "KeyN", "Space", "F7", "Digit5", "KeyW", "BracketLeft", "KeyK", "Delete", "KeyB", "Slash", "F6", "Digit4", "KeyQ", "KeyP", "KeyJ", "Backslash",
                               "KeyV", "Period", "F5", "Digit3", "Equal", "KeyO", "KeyH", "Quote", "KeyC", "Comma", "F4", "Digit2", "Minus", "KeyI", "KeyG", "Semicolon", "KeyX", "KeyM", "F3",
                               "Digit1", "Digit0", "KeyU", "KeyF", "CapsLock", "MetaLeft", "F2", "Backquote", "Digit9", "KeyY", "KeyD", "ControlLeft", "AltLeft", "ScrollLock", "F1", "Enter",
                               "Digit8", "KeyT", "KeyS", "MetaRight", "ContextMenu", "NumpadDecimal", "F10", "Numpad4", "Numpad6", "Numpad5", "Insert", "Home", "PageUp", "Numpad0", "Numpad1",
                               "Numpad3", "Numpad2", "PrintScreen", "Tab", "Pause"};
  // 矩阵一共 14 行 8 列 112 个位置，其中 11 个是空位，所以有 101 个键位。
  EXPECT_EQ(sizeof(codes) / sizeof(codes[0]), 101U);
  for (const char* code : codes) {
    KeyboardYuxing keyboard;
    keyboard.selectLow(0xFF);
    keyboard.selectHigh(0x3F);
    ASSERT_TRUE(keyboard.setKey(code, true)) << code;
    const std::uint8_t value = keyboard.read();
    EXPECT_NE(value, 0) << code;
    EXPECT_EQ(value & (value - 1), 0) << code << " 应该只占一列";
  }
}

}  // namespace
