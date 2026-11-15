/*
 * ------------------------------------------------------------------------
 * 名称：keyboard-fc-test.cpp
 * 说明：Family BASIC 键盘的测试：按 Family BASIC 的扫描流程读整个矩阵、行列计数器的规则、使能位、第 10 行的检测值、键位表（JIS 位置）与同一个键位名对应多个位置、复位与全部松开。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：扫描流程是 Family BASIC 用的：写 $05 复位，随后每行写 $04 读列 0、写 $06 读列 1。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <array>
#include <cstdint>

#include "logic/hardware/keyboard-fc/keyboard-fc.hpp"

namespace {

using ember::logic::hardware::KeyboardFc;

/** 按 Family BASIC 的流程读整个矩阵：scan[行][列] 是 $4017 位 1-4（按下为 0）。 */
std::array<std::array<std::uint8_t, 2>, 9> scan(KeyboardFc& keyboard) {
  std::array<std::array<std::uint8_t, 2>, 9> result{};
  keyboard.write(0x05);
  for (unsigned row = 0; row < 9; ++row) {
    keyboard.write(0x04);
    result[row][0] = keyboard.read();
    keyboard.write(0x06);
    result[row][1] = keyboard.read();
  }
  return result;
}

constexpr std::uint8_t RELEASED = 0x1E;

TEST(KeyboardFc, WithNoKeyPressedEveryPositionReadsOne) {
  KeyboardFc keyboard;
  for (const auto& row : scan(keyboard)) {
    EXPECT_EQ(row[0], RELEASED);
    EXPECT_EQ(row[1], RELEASED);
  }
}

TEST(KeyboardFc, ReadsZeroUntilEnabled) {
  KeyboardFc keyboard;
  keyboard.setKey("KeyA", true);
  EXPECT_EQ(keyboard.read(), 0) << "上电后没有使能";
  keyboard.write(0x05);
  keyboard.write(0x00);
  EXPECT_EQ(keyboard.read(), 0) << "位 2 为 0 时读出恒为 0";
}

TEST(KeyboardFc, APressedKeyShowsUpAtItsRowColumnAndBit) {
  KeyboardFc keyboard;
  // 第 6 行列 0 的位 4 是 A；第 6 行列 1 的位 1 是 X；第 8 行列 1 的位 2 是空格。
  keyboard.setKey("KeyA", true);
  keyboard.setKey("KeyX", true);
  keyboard.setKey("Space", true);
  const auto matrix = scan(keyboard);
  EXPECT_EQ(matrix[6][0], 0x0E);
  EXPECT_EQ(matrix[6][1], 0x1C);
  EXPECT_EQ(matrix[8][1], 0x1A);
  EXPECT_EQ(matrix[0][0], RELEASED);
  EXPECT_EQ(matrix[7][1], RELEASED);
}

TEST(KeyboardFc, ReleasingAKeyOrAllKeysRestoresTheMatrix) {
  KeyboardFc keyboard;
  keyboard.setKey("KeyQ", true);
  keyboard.setKey("F1", true);
  EXPECT_EQ(scan(keyboard)[7][0], 0x14) << "Q 在位 3，F1 在位 1";
  keyboard.setKey("KeyQ", false);
  EXPECT_EQ(scan(keyboard)[7][0], 0x1C);
  keyboard.releaseAll();
  EXPECT_EQ(scan(keyboard)[7][0], RELEASED);
}

TEST(KeyboardFc, TheRowCounterOnlyAdvancesWhenTheColumnLineFalls) {
  KeyboardFc keyboard;
  keyboard.write(0x05);
  EXPECT_EQ(keyboard.row(), 0U);
  keyboard.write(0x04);
  EXPECT_EQ(keyboard.row(), 0U) << "列线本来就是低，没有下降沿";
  keyboard.write(0x06);
  EXPECT_EQ(keyboard.row(), 0U);
  keyboard.write(0x04);
  EXPECT_EQ(keyboard.row(), 1U);
  keyboard.write(0x06);
  keyboard.write(0x06);
  keyboard.write(0x04);
  EXPECT_EQ(keyboard.row(), 2U);
}

TEST(KeyboardFc, TheResetBitReturnsToRowZeroAndTheTenthRowWrapsAround) {
  KeyboardFc keyboard;
  keyboard.write(0x05);
  for (unsigned step = 0; step < 4; ++step) {
    keyboard.write(0x06);
    keyboard.write(0x04);
  }
  EXPECT_EQ(keyboard.row(), 4U);
  keyboard.write(0x05);
  EXPECT_EQ(keyboard.row(), 0U);
  for (unsigned step = 0; step < 9; ++step) {
    keyboard.write(0x06);
    keyboard.write(0x04);
  }
  EXPECT_EQ(keyboard.row(), 9U);
  EXPECT_EQ(keyboard.read(), RELEASED) << "第 10 行没有键，使能时读出全 1（游戏用它检测键盘是否存在）";
  keyboard.write(0x06);
  keyboard.write(0x04);
  EXPECT_EQ(keyboard.row(), 0U) << "从第 10 行再加 1 回到第 0 行";
}

TEST(KeyboardFc, ResetClearsTheCountersAndTheEnableButKeepsTheKeys) {
  KeyboardFc keyboard;
  keyboard.setKey("KeyA", true);
  keyboard.write(0x05);
  keyboard.write(0x06);
  keyboard.write(0x04);
  ASSERT_EQ(keyboard.row(), 1U);
  keyboard.reset();
  EXPECT_EQ(keyboard.row(), 0U);
  EXPECT_EQ(keyboard.read(), 0);
  EXPECT_EQ(scan(keyboard)[6][0], 0x0E) << "按键保持";
}

TEST(KeyboardFc, KeyNamesFollowTheJisLayoutPositions) {
  KeyboardFc keyboard;
  // JIS：@ 在 BracketLeft（第 1 行列 0 位 2），[ 在 BracketRight（第 0 行列 0 位 3），] 在 Backslash（第 0 行列 0 位 4），: 在 Quote（第 1 行列 0 位 3）。
  keyboard.setKey("BracketLeft", true);
  keyboard.setKey("BracketRight", true);
  keyboard.setKey("Backslash", true);
  keyboard.setKey("Quote", true);
  const auto matrix = scan(keyboard);
  EXPECT_EQ(matrix[1][0], 0x1E & ~(1 << 2) & ~(1 << 3) & 0x1E);
  EXPECT_EQ(matrix[0][0], 0x1E & ~(1 << 3) & ~(1 << 4) & 0x1E);
}

TEST(KeyboardFc, SomeKeysAnswerToMoreThanOneKeyName) {
  KeyboardFc keyboard;
  for (const char* code : {"Enter", "NumpadEnter"}) {
    keyboard.setKey(code, true);
    EXPECT_EQ(scan(keyboard)[0][0], 0x1A) << code;
    keyboard.setKey(code, false);
  }
  for (const char* code : {"Backspace", "Delete"}) {
    keyboard.setKey(code, true);
    EXPECT_EQ(scan(keyboard)[8][1], 0x16) << code << "：DEL 在第 8 行列 1 的位 3";
    keyboard.setKey(code, false);
  }
  for (const char* code : {"ShiftLeft"}) {
    keyboard.setKey(code, true);
    EXPECT_EQ(scan(keyboard)[7][1], 0x1C) << code;
    keyboard.setKey(code, false);
  }
}

TEST(KeyboardFc, AnUnknownKeyIsIgnored) {
  KeyboardFc keyboard;
  EXPECT_FALSE(keyboard.setKey("MediaPlayPause", true));
  EXPECT_TRUE(keyboard.setKey("KeyA", true));
  for (const auto& row : scan(keyboard)) {
    EXPECT_TRUE(row[0] == RELEASED || row[0] == 0x0E);
  }
}

TEST(KeyboardFc, EveryKeyOfTheMatrixCanBeReached) {
  // 72 个键：每个位置恰好有一个键名，按下后只有它自己读到 0。
  const char* const names[9][2][4] = {
      {{"Backslash", "BracketRight", "Enter", "F8"}, {"Pause", "IntlYen", "ShiftRight", "AltRight"}},
      {{"Semicolon", "Quote", "BracketLeft", "F7"}, {"Equal", "Minus", "Slash", "IntlRo"}},
      {{"KeyK", "KeyL", "KeyO", "F6"}, {"Digit0", "KeyP", "Comma", "Period"}},
      {{"KeyJ", "KeyU", "KeyI", "F5"}, {"Digit8", "Digit9", "KeyN", "KeyM"}},
      {{"KeyH", "KeyG", "KeyY", "F4"}, {"Digit6", "Digit7", "KeyV", "KeyB"}},
      {{"KeyD", "KeyR", "KeyT", "F3"}, {"Digit4", "Digit5", "KeyC", "KeyF"}},
      {{"KeyA", "KeyS", "KeyW", "F2"}, {"Digit3", "KeyE", "KeyZ", "KeyX"}},
      {{"ControlLeft", "KeyQ", "Escape", "F1"}, {"Digit2", "Digit1", "AltLeft", "ShiftLeft"}},
      {{"ArrowLeft", "ArrowRight", "ArrowUp", "Home"}, {"Insert", "Backspace", "Space", "ArrowDown"}}};
  for (unsigned row = 0; row < 9; ++row) {
    for (unsigned column = 0; column < 2; ++column) {
      for (unsigned index = 0; index < 4; ++index) {
        KeyboardFc keyboard;
        ASSERT_TRUE(keyboard.setKey(names[row][column][index], true)) << names[row][column][index];
        const auto matrix = scan(keyboard);
        const std::uint8_t expected = static_cast<std::uint8_t>(RELEASED & ~(1U << (4 - index)));
        for (unsigned r = 0; r < 9; ++r) {
          for (unsigned c = 0; c < 2; ++c) EXPECT_EQ(matrix[r][c], r == row && c == column ? expected : RELEASED) << names[row][column][index] << " 行 " << r << " 列 " << c;
        }
      }
    }
  }
}

}  // namespace
