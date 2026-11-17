/*
 * ------------------------------------------------------------------------
 * 名称：keyboard-sb97-test.cpp
 * 说明：小霸王 97 键盘的测试：扫描控制、矩阵的位置与半边、这台机器自己的键位表。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-17
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <cstdint>

#include "logic/hardware/keyboard-sb97/keyboard-sb97.hpp"

namespace {

using ember::logic::hardware::KeyboardSb97;

/** 前进到某个位置的前半边：固件的做法是每个位置先写 $04 读前半边、再写 $06 翻到后半边，再写 $04 进入下一个位置的前半边。 */
void goToFront(KeyboardSb97& keyboard, unsigned position) {
  keyboard.write(0x05);
  for (unsigned step = 1; step <= position; ++step) {
    keyboard.write(0x04);
    if (step < position) keyboard.write(0x06);
  }
}

/** 读某个位置某个半边的 4 个键（位 1-4）。 */
std::uint8_t readAt(KeyboardSb97& keyboard, unsigned position, bool front) {
  goToFront(keyboard, position);
  if (!front) keyboard.write(0x06);
  return keyboard.read();
}

TEST(KeyboardSb97, NoKeyReadsAllOnesInTheDataBits) {
  KeyboardSb97 keyboard;
  EXPECT_EQ(keyboard.read(), 0x1E) << "位置 0 没有键";
  for (unsigned position = 1; position <= 13; ++position) {
    if (position == 10) continue;
    EXPECT_EQ(readAt(keyboard, position, true), 0x1E) << position;
    EXPECT_EQ(readAt(keyboard, position, false), 0x1E) << position;
  }
}

TEST(KeyboardSb97, ScanControlMovesThePositionAndTheHalf) {
  KeyboardSb97 keyboard;
  keyboard.write(0x05);
  EXPECT_EQ(keyboard.position(), 0U);
  EXPECT_FALSE(keyboard.firstHalf());
  keyboard.write(0x04);
  EXPECT_EQ(keyboard.position(), 1U);
  EXPECT_TRUE(keyboard.firstHalf());
  keyboard.write(0x06);
  EXPECT_EQ(keyboard.position(), 1U);
  EXPECT_FALSE(keyboard.firstHalf());
  keyboard.write(0x01);  // 位 2 为 0：不是扫描控制
  keyboard.write(0x03);
  EXPECT_EQ(keyboard.position(), 1U);
  EXPECT_FALSE(keyboard.firstHalf());
  for (int i = 0; i < 12; ++i) keyboard.write(0x04);
  EXPECT_EQ(keyboard.position(), 13U);
  keyboard.write(0x04);
  EXPECT_EQ(keyboard.position(), 0U) << "超过 13 回到 0";
}

TEST(KeyboardSb97, APressedKeyClearsItsBitInThePositionAndHalfItSitsIn) {
  KeyboardSb97 keyboard;
  EXPECT_TRUE(keyboard.setKey("Digit4", true));
  EXPECT_EQ(readAt(keyboard, 1, true), 0x1C);
  EXPECT_EQ(readAt(keyboard, 1, false), 0x1E);
  EXPECT_TRUE(keyboard.setKey("KeyC", true));
  EXPECT_EQ(readAt(keyboard, 1, true), 0x0C);
  keyboard.setKey("Digit4", false);
  EXPECT_EQ(readAt(keyboard, 1, true), 0x0E);
  EXPECT_TRUE(keyboard.setKey("F2", true));
  EXPECT_EQ(readAt(keyboard, 1, false), 0x1C);
  keyboard.releaseAll();
  EXPECT_EQ(readAt(keyboard, 1, true), 0x1E);
  EXPECT_EQ(readAt(keyboard, 1, false), 0x1E);
}

TEST(KeyboardSb97, PositionTenHasOnlyAConstantKeyInTheSecondHalf) {
  KeyboardSb97 keyboard;
  EXPECT_EQ(readAt(keyboard, 10, true), 0x1E);
  EXPECT_EQ(readAt(keyboard, 10, false), 0x1C);
}

TEST(KeyboardSb97, TheKeyTableIsThisMachinesOwn) {
  unsigned position = 0;
  bool front = false;
  unsigned bit = 0;
  // 位置 6 的前半边第 4 个键是 Tab，位置 11 的前半边第 1 个键是左 Alt，位置 13 的前半边第 3 个键是 Pause。
  ASSERT_TRUE(KeyboardSb97::locate("Tab", position, front, bit));
  EXPECT_EQ(position, 6U);
  EXPECT_TRUE(front);
  EXPECT_EQ(bit, 3U);
  ASSERT_TRUE(KeyboardSb97::locate("AltLeft", position, front, bit));
  EXPECT_EQ(position, 11U);
  EXPECT_TRUE(front);
  EXPECT_EQ(bit, 0U);
  ASSERT_TRUE(KeyboardSb97::locate("Pause", position, front, bit));
  EXPECT_EQ(position, 13U);
  EXPECT_TRUE(front);
  EXPECT_EQ(bit, 2U);
  // Ctrl 在位置 6 的后半边，右边的 Ctrl 与 Alt 没有接。
  ASSERT_TRUE(KeyboardSb97::locate("ControlLeft", position, front, bit));
  EXPECT_EQ(position, 6U);
  EXPECT_FALSE(front);
  EXPECT_EQ(bit, 3U);
  EXPECT_FALSE(KeyboardSb97::locate("ControlRight", position, front, bit));
  EXPECT_FALSE(KeyboardSb97::locate("AltRight", position, front, bit));
}

TEST(KeyboardSb97, TheSpaceBarIsWiredToPositionNineAndPositionThirteen) {
  KeyboardSb97 keyboard;
  keyboard.setKey("Space", true);
  EXPECT_EQ(readAt(keyboard, 9, true), 0x0E);
  EXPECT_EQ(readAt(keyboard, 13, true), 0x0E);
  EXPECT_EQ(readAt(keyboard, 9, false), 0x1E);
}

TEST(KeyboardSb97, BothShiftKeysAreTheSameKeyAndUnknownKeysAreRejected) {
  KeyboardSb97 keyboard;
  EXPECT_TRUE(keyboard.setKey("ShiftLeft", true));
  EXPECT_EQ(readAt(keyboard, 8, false), 0x0E);
  keyboard.setKey("ShiftLeft", false);
  EXPECT_TRUE(keyboard.setKey("ShiftRight", true));
  EXPECT_EQ(readAt(keyboard, 8, false), 0x0E);
  EXPECT_FALSE(keyboard.setKey("F13", true));
  EXPECT_FALSE(keyboard.setKey("", true));
}

TEST(KeyboardSb97, ResetKeepsTheKeysButMovesTheScanBackToTheStart) {
  KeyboardSb97 keyboard;
  keyboard.setKey("KeyA", true);
  goToFront(keyboard, 6);
  keyboard.write(0x06);
  EXPECT_EQ(keyboard.read(), 0x1A);
  keyboard.reset();
  EXPECT_EQ(keyboard.position(), 0U);
  EXPECT_FALSE(keyboard.firstHalf());
  EXPECT_EQ(readAt(keyboard, 6, false), 0x1A) << "按键保持";
}

}  // namespace
