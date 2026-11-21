/*
 * ------------------------------------------------------------------------
 * 名称：mouse-yuxing-test.cpp
 * 说明：裕兴鼠标的测试：数据包的编码、逐位移出的节奏（上升沿装包与移位）、位 2 为 1 时不移位、数据位读出的条件与取反、位移按帧锁存与限幅、按键、复位。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：读程序的流程与学习机一致：写 $4016 为 0，写 1（上升沿），读数据位，如此重复。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <array>
#include <cstdint>

#include "logic/hardware/mouse-yuxing/mouse-yuxing.hpp"

namespace {

using ember::logic::hardware::MouseYuxing;

/** 一个完整的数据包解出来的三个字节。 */
using Packet = std::array<std::uint8_t, 3>;

/** 写 0、写 1 产生一次上升沿。 */
void pulse(MouseYuxing& mouse, std::uint8_t high = 0x01) {
  mouse.write(0x00);
  mouse.write(high);
}

/** 读一个数据包：先把没读完的旧包移空，之后第一次上升沿装包并读第 1 位，再每次上升沿移出下一位；每个字节是 起始位、7 个数据位、停止位，读出的位是取反的。 */
Packet readPacket(MouseYuxing& mouse) {
  while (mouse.bitsLeft() > 0) pulse(mouse);
  Packet bytes{};
  for (unsigned index = 0; index < MouseYuxing::PACKET_BITS; ++index) {
    pulse(mouse);
    const unsigned bit = mouse.read();
    const unsigned position = index % 9;
    if (position == 0) {
      EXPECT_EQ(bit, 1U) << "起始位取反后读 1，位 " << index;
    } else if (position == 8) {
      EXPECT_EQ(bit, 0U) << "停止位取反后读 0，位 " << index;
    } else {
      bytes[index / 9] = static_cast<std::uint8_t>((bytes[index / 9] << 1) | (bit ^ 1U));
    }
  }
  return bytes;
}

TEST(MouseYuxing, AnIdleMouseSendsAPacketOfOnlyTheSyncBit) {
  MouseYuxing mouse;
  mouse.syncFrame();
  EXPECT_EQ(readPacket(mouse), (Packet{0x40, 0x00, 0x00}));
}

TEST(MouseYuxing, ReadsZeroUntilTheClockIsHighAndAPacketIsLoaded) {
  MouseYuxing mouse;
  EXPECT_EQ(mouse.read(), 0);
  mouse.write(0x01);  // 第一次上升沿装包
  EXPECT_EQ(mouse.bitsLeft(), MouseYuxing::PACKET_BITS);
  EXPECT_EQ(mouse.read(), 1) << "起始位";
  mouse.write(0x00);
  EXPECT_EQ(mouse.read(), 0) << "时钟为低时不给数据";
}

TEST(MouseYuxing, EachRisingEdgeShiftsOneBitAndTheLastBitIsFollowedByAFreshPacket) {
  MouseYuxing mouse;
  mouse.syncFrame();
  pulse(mouse);
  for (unsigned index = 1; index < MouseYuxing::PACKET_BITS; ++index) {
    pulse(mouse);
    EXPECT_EQ(mouse.bitsLeft(), MouseYuxing::PACKET_BITS - index);
  }
  EXPECT_EQ(mouse.bitsLeft(), 1U) << "最后一位还在";
  pulse(mouse);
  EXPECT_EQ(mouse.bitsLeft(), 0U);
  EXPECT_EQ(mouse.read(), 0) << "移空之后读 0";
  pulse(mouse);
  EXPECT_EQ(mouse.bitsLeft(), MouseYuxing::PACKET_BITS) << "再一次上升沿装入新包";
}

TEST(MouseYuxing, WritesWithBitTwoSetDoNotShift) {
  MouseYuxing mouse;
  mouse.write(0x01);
  const unsigned before = mouse.bitsLeft();
  pulse(mouse, 0x05);
  pulse(mouse, 0x07);
  EXPECT_EQ(mouse.bitsLeft(), before);
}

TEST(MouseYuxing, HoldingTheClockHighDoesNotShiftAgain) {
  MouseYuxing mouse;
  mouse.write(0x00);
  mouse.write(0x01);
  const unsigned before = mouse.bitsLeft();
  mouse.write(0x01);
  mouse.write(0x03);
  EXPECT_EQ(mouse.bitsLeft(), before) << "只有 0 到 1 的变化才移位";
}

TEST(MouseYuxing, MovementIsLatchedPerFrameAndEveryPacketOfThatFrameCarriesIt) {
  MouseYuxing mouse;
  mouse.move(5, -3, 0);
  mouse.move(2, 1, 0);
  // 没有锁存之前包里没有位移。
  EXPECT_EQ(readPacket(mouse), (Packet{0x40, 0x00, 0x00}));
  mouse.syncFrame();
  // x = 7，y = -2：第 1 个字节的位 3、2 是 x 的位 7、6（0），位 1、0 是 y 的位 7、6（3）；第 2 个字节是 y 的低 6 位，第 3 个字节是 x 的低 6 位。
  const Packet expected{0x43, 0x3E, 0x07};
  EXPECT_EQ(readPacket(mouse), expected);
  EXPECT_EQ(readPacket(mouse), expected) << "同一帧里的下一个包带同样的位移";
  mouse.syncFrame();
  EXPECT_EQ(readPacket(mouse), (Packet{0x40, 0x00, 0x00})) << "下一帧没有新的位移";
}

TEST(MouseYuxing, NegativeAndLargeMovementsAreEncodedAsSignedBytes) {
  MouseYuxing mouse;
  mouse.move(-100, 100, 0);
  mouse.syncFrame();
  // x = -100（$9C）：位 7、6 是 10；y = 100（$64）：位 7、6 是 01；低 6 位分别是 $1C 与 $24。
  EXPECT_EQ(readPacket(mouse), (Packet{static_cast<std::uint8_t>(0x40 | 0x08 | 0x01), 0x24, 0x1C}));
  mouse.move(1000, -1000, 0);
  mouse.syncFrame();
  // 位移限在 -128 到 127：x = 127（$7F），y = -128（$80）。
  EXPECT_EQ(readPacket(mouse), (Packet{static_cast<std::uint8_t>(0x40 | 0x04 | 0x02), 0x00, 0x3F}));
}

TEST(MouseYuxing, ButtonsAreLatchedWithTheFrame) {
  MouseYuxing mouse;
  mouse.move(0, 0, MouseYuxing::BUTTON_LEFT);
  EXPECT_EQ(readPacket(mouse)[0], 0x40) << "按键也是每帧锁存";
  mouse.syncFrame();
  EXPECT_EQ(readPacket(mouse)[0], 0x60);
  mouse.move(0, 0, MouseYuxing::BUTTON_RIGHT);
  mouse.syncFrame();
  EXPECT_EQ(readPacket(mouse)[0], 0x50);
  mouse.move(0, 0, MouseYuxing::BUTTON_LEFT | MouseYuxing::BUTTON_RIGHT);
  mouse.syncFrame();
  EXPECT_EQ(readPacket(mouse)[0], 0x70);
}

TEST(MouseYuxing, ResetForgetsThePacketMovementAndButtons) {
  MouseYuxing mouse;
  mouse.move(10, 10, MouseYuxing::BUTTON_LEFT);
  mouse.syncFrame();
  pulse(mouse);
  mouse.reset();
  EXPECT_EQ(mouse.bitsLeft(), 0U);
  EXPECT_EQ(mouse.read(), 0);
  mouse.syncFrame();
  EXPECT_EQ(readPacket(mouse), (Packet{0x40, 0x00, 0x00}));
}

}  // namespace
