/*
 * ------------------------------------------------------------------------
 * 名称：keyboard-pyramid-test.cpp
 * 说明：金字塔学习机键盘的测试：行列计数器的命令字、逐列读键、第 12 行之后回到第 0 行、键位表与同一个键位对应多个键、复位与全部松开。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：扫描流程与学习机的程序一致：写 $01、写 $02，连读 8 次得到当前行的 8 列，写 $07 换到下一行。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <initializer_list>

#include "logic/hardware/keyboard-pyramid/keyboard-pyramid.hpp"

namespace {

using ember::logic::hardware::KeyboardPyramid;

/** 按学习机的流程读整个矩阵：scan[行] 的位 n 是列 n 的键（按下为 1）。 */
std::array<std::uint8_t, KeyboardPyramid::ROWS> scan(KeyboardPyramid& keyboard) {
  std::array<std::uint8_t, KeyboardPyramid::ROWS> result{};
  keyboard.write(0x01);
  for (unsigned row = 0; row < KeyboardPyramid::ROWS; ++row) {
    keyboard.write(0x02);
    std::uint8_t bits = 0;
    for (unsigned column = 0; column < KeyboardPyramid::COLUMNS; ++column) {
      if ((keyboard.read() & KeyboardPyramid::DATA_MASK) != 0) bits = static_cast<std::uint8_t>(bits | (1U << column));
    }
    result[row] = bits;
    keyboard.write(0x07);
  }
  return result;
}

TEST(KeyboardPyramid, WithNoKeyPressedEveryPositionReadsZero) {
  KeyboardPyramid keyboard;
  for (const std::uint8_t row : scan(keyboard)) EXPECT_EQ(row, 0);
}

TEST(KeyboardPyramid, OnlyBitOneIsDriven) {
  KeyboardPyramid keyboard;
  EXPECT_EQ(KeyboardPyramid::DATA_MASK, 0x02);
  keyboard.setKey("Space", true);
  keyboard.write(0x01);
  keyboard.write(0x02);
  for (unsigned column = 0; column < 8; ++column) EXPECT_EQ(keyboard.read() & ~0x02, 0);
}

TEST(KeyboardPyramid, APressedKeyShowsUpAtItsRowAndColumn) {
  KeyboardPyramid keyboard;
  // 第 0 行列 6 是空格，第 2 行列 4 是 A，第 11 行列 7 是上光标。
  keyboard.setKey("Space", true);
  keyboard.setKey("KeyA", true);
  keyboard.setKey("ArrowUp", true);
  const auto matrix = scan(keyboard);
  EXPECT_EQ(matrix[0], 1U << 6);
  EXPECT_EQ(matrix[2], 1U << 4);
  EXPECT_EQ(matrix[11], 1U << 7);
  for (unsigned row : {1U, 3U, 4U, 5U, 6U, 7U, 8U, 9U, 10U, 12U}) EXPECT_EQ(matrix[row], 0) << "第 " << row << " 行";
}

TEST(KeyboardPyramid, ReleasingAKeyOrAllKeysRestoresTheMatrix) {
  KeyboardPyramid keyboard;
  keyboard.setKey("KeyQ", true);
  keyboard.setKey("KeyW", true);
  keyboard.setKey("KeyQ", false);
  EXPECT_EQ(scan(keyboard)[3], 1U << 1);
  keyboard.releaseAll();
  for (const std::uint8_t row : scan(keyboard)) EXPECT_EQ(row, 0);
}

TEST(KeyboardPyramid, EachReadAdvancesTheColumnAndBeyondColumnSevenReadsZero) {
  KeyboardPyramid keyboard;
  keyboard.setKey("Escape", true);  // 第 0 行列 7
  keyboard.write(0x01);
  keyboard.write(0x02);
  for (unsigned column = 0; column < 7; ++column) EXPECT_EQ(keyboard.read(), 0);
  EXPECT_EQ(keyboard.column(), 7U);
  EXPECT_EQ(keyboard.read(), 0x02);
  EXPECT_EQ(keyboard.read(), 0) << "列超过 7 没有键";
  keyboard.write(0x02);
  EXPECT_EQ(keyboard.column(), 0U) << "写 $02 回到第 0 列";
}

TEST(KeyboardPyramid, CommandsOneAndZeroGoToRowZeroAndSixAndSevenAdvanceTheRow) {
  KeyboardPyramid keyboard;
  keyboard.write(0x06);
  keyboard.write(0x07);
  EXPECT_EQ(keyboard.row(), 2U);
  keyboard.write(0x00);
  EXPECT_EQ(keyboard.row(), 0U);
  keyboard.write(0x07);
  keyboard.write(0x01);
  EXPECT_EQ(keyboard.row(), 0U);
}

TEST(KeyboardPyramid, OtherCommandWordsDoNothing) {
  KeyboardPyramid keyboard;
  keyboard.write(0x06);
  static_cast<void>(keyboard.read());
  for (const std::uint8_t value : std::initializer_list<std::uint8_t>{0x03, 0x04, 0x05, 0x08, 0xFF}) keyboard.write(value);
  EXPECT_EQ(keyboard.row(), 1U);
  EXPECT_EQ(keyboard.column(), 1U);
}

TEST(KeyboardPyramid, TheRowWrapsAroundAfterTheLastRow) {
  KeyboardPyramid keyboard;
  for (unsigned step = 0; step < KeyboardPyramid::ROWS - 1; ++step) keyboard.write(0x07);
  EXPECT_EQ(keyboard.row(), KeyboardPyramid::ROWS - 1);
  keyboard.write(0x07);
  EXPECT_EQ(keyboard.row(), 0U);
}

TEST(KeyboardPyramid, TheTwoSidesOfAModifierShareOneKey) {
  KeyboardPyramid keyboard;
  keyboard.setKey("ShiftRight", true);
  EXPECT_EQ(scan(keyboard)[0], 1U << 0);
  keyboard.setKey("ShiftRight", false);
  keyboard.setKey("ShiftLeft", true);
  EXPECT_EQ(scan(keyboard)[0], 1U << 0);
  keyboard.setKey("ShiftLeft", false);
  keyboard.setKey("ControlRight", true);
  EXPECT_EQ(scan(keyboard)[0], 1U << 3);
  keyboard.setKey("ControlRight", false);
  keyboard.setKey("AltLeft", true);
  EXPECT_EQ(scan(keyboard)[0], 1U << 5);
}

TEST(KeyboardPyramid, AnUnknownKeyIsIgnored) {
  KeyboardPyramid keyboard;
  EXPECT_FALSE(keyboard.setKey("MediaPlayPause", true));
  EXPECT_TRUE(keyboard.setKey("KeyA", true));
  for (const std::uint8_t row : scan(keyboard)) EXPECT_NE(row, 0xFF);
}

TEST(KeyboardPyramid, ResetClearsTheCountersButKeepsTheKeys) {
  KeyboardPyramid keyboard;
  keyboard.setKey("KeyA", true);
  keyboard.write(0x07);
  static_cast<void>(keyboard.read());
  keyboard.reset();
  EXPECT_EQ(keyboard.row(), 0U);
  EXPECT_EQ(keyboard.column(), 0U);
  EXPECT_EQ(scan(keyboard)[2], 1U << 4);
}

TEST(KeyboardPyramid, EveryKeyOfTheMatrixCanBeReached) {
  // 键位表里每个键各按一次，矩阵里恰好多出这个键的一位（同一位置的左右键算一个）。
  const char* names[] = {"ShiftLeft", "Tab", "Backquote", "ControlLeft", "CapsLock", "AltLeft", "Space", "Escape", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12",
                         "KeyA", "KeyB", "KeyC", "KeyD", "KeyE", "KeyF", "KeyG", "KeyH", "KeyI", "KeyJ", "KeyK", "KeyL", "KeyM", "KeyN", "KeyO", "KeyP", "KeyQ", "KeyR", "KeyS", "KeyT", "KeyU",
                         "KeyV", "KeyW", "KeyX", "KeyY", "KeyZ", "Digit0", "Digit1", "Digit2", "Digit3", "Digit4", "Digit5", "Digit6", "Digit7", "Digit8", "Digit9", "Enter", "Backspace",
                         "Minus", "Equal", "BracketLeft", "BracketRight", "Backslash", "Semicolon", "Quote", "Comma", "Period", "Slash", "Delete", "End", "Insert", "PageDown", "PageUp", "Home",
                         "ArrowLeft", "ArrowRight", "ArrowUp", "ArrowDown", "PrintScreen", "ScrollLock", "Pause", "NumLock", "NumpadAdd", "NumpadSubtract", "NumpadMultiply", "NumpadDivide",
                         "NumpadDecimal", "NumpadEnter", "Numpad0", "Numpad1", "Numpad2", "Numpad3", "Numpad4", "Numpad5", "Numpad6", "Numpad7", "Numpad8", "Numpad9"};
  unsigned total = 0;
  for (const char* name : names) {
    KeyboardPyramid keyboard;
    ASSERT_TRUE(keyboard.setKey(name, true)) << name;
    unsigned bits = 0;
    for (const std::uint8_t row : scan(keyboard)) {
      for (unsigned column = 0; column < 8; ++column) bits += (row >> column) & 1U;
    }
    EXPECT_EQ(bits, 1U) << name;
    ++total;
  }
  EXPECT_EQ(total, sizeof(names) / sizeof(names[0]));
}

}  // namespace
