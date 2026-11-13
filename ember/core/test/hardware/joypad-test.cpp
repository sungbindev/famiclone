/*
 * ------------------------------------------------------------------------
 * 名称：joypad-test.cpp
 * 说明：标准手柄与 2A03 手柄口的测试：锁存与移位、读完 8 位后读到 1、锁存线为高时重复读 A、两个口共用锁存线、只驱动低 5 位、未插设备读 0。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include "contracts/hardware/button-input.hpp"
#include "contracts/hardware/hardware-error.hpp"
#include "logic/hardware/bus/mapped-bus.hpp"
#include "logic/hardware/joypad/joypad.hpp"
#include "logic/hardware/pad-2a03/pad-2a03.hpp"
#include "shared/errors/core-error.hpp"

namespace {

namespace hw = ember::contracts::hardware;
using ember::logic::hardware::CpuAddressBus;
using ember::logic::hardware::Joypad;
using ember::logic::hardware::Pad2a03;

/** 读 $4016 或 $4017 的数据位 0，连续读 count 次。 */
std::vector<int> readBits(Pad2a03& pad, std::uint16_t address, int count) {
  std::vector<int> bits;
  for (int i = 0; i < count; ++i) bits.push_back(pad.read(address).value & 1);
  return bits;
}

/** 写 $4016：先置高再置低，锁存当前按键。 */
void latch(Pad2a03& pad) {
  pad.write(0x4016, 1);
  pad.write(0x4016, 0);
}

TEST(Joypad, ShiftsOutAButtonsInOrderThenReadsOnes) {
  Pad2a03 pad;
  Joypad joypad;
  pad.port(0).plug(&joypad);
  joypad.setButtons(hw::pad_button::A | hw::pad_button::START | hw::pad_button::RIGHT);
  latch(pad);
  // 顺序：A、B、选择、开始、上、下、左、右。
  EXPECT_EQ(readBits(pad, 0x4016, 8), (std::vector<int>{1, 0, 0, 1, 0, 0, 0, 1}));
  // 移空后读到 1。
  EXPECT_EQ(readBits(pad, 0x4016, 3), (std::vector<int>{1, 1, 1}));
}

TEST(Joypad, ButtonMaskBitsMatchTheShiftOrder) {
  Pad2a03 pad;
  Joypad joypad;
  pad.port(0).plug(&joypad);
  for (int bit = 0; bit < 8; ++bit) {
    joypad.setButtons(static_cast<std::uint8_t>(1U << bit));
    latch(pad);
    std::vector<int> expected(8, 0);
    expected[static_cast<std::size_t>(bit)] = 1;
    EXPECT_EQ(readBits(pad, 0x4016, 8), expected) << "按键位 " << bit;
  }
}

TEST(Joypad, ButtonsChangedAfterTheLatchDoNotAffectTheCurrentReadout) {
  Pad2a03 pad;
  Joypad joypad;
  pad.port(0).plug(&joypad);
  joypad.setButtons(hw::pad_button::A);
  latch(pad);
  joypad.setButtons(hw::pad_button::B);
  EXPECT_EQ(readBits(pad, 0x4016, 2), (std::vector<int>{1, 0}));
  latch(pad);
  EXPECT_EQ(readBits(pad, 0x4016, 2), (std::vector<int>{0, 1}));
}

TEST(Joypad, WhileTheLatchLineIsHighReadsKeepReturningTheAButton) {
  Pad2a03 pad;
  Joypad joypad;
  pad.port(0).plug(&joypad);
  joypad.setButtons(hw::pad_button::A);
  pad.write(0x4016, 1);
  EXPECT_EQ(readBits(pad, 0x4016, 4), (std::vector<int>{1, 1, 1, 1}));
  // 锁存线为高时按键变化立即可见。
  joypad.setButtons(0);
  EXPECT_EQ(readBits(pad, 0x4016, 2), (std::vector<int>{0, 0}));
}

TEST(Joypad, TheLatchLineIsSharedByBothPortsAndEachPortHasItsOwnShiftRegister) {
  Pad2a03 pad;
  Joypad first;
  Joypad second;
  pad.port(0).plug(&first);
  pad.port(1).plug(&second);
  first.setButtons(hw::pad_button::A);
  second.setButtons(hw::pad_button::B);
  latch(pad);
  EXPECT_EQ(readBits(pad, 0x4016, 3), (std::vector<int>{1, 0, 0}));
  EXPECT_EQ(readBits(pad, 0x4017, 3), (std::vector<int>{0, 1, 0}));
  // 只读一个口不影响另一个口的移位位置。
  EXPECT_EQ(readBits(pad, 0x4016, 1), (std::vector<int>{0}));
  EXPECT_EQ(readBits(pad, 0x4017, 1), (std::vector<int>{0}));
}

TEST(Joypad, WritingToAddress4017DoesNotTouchTheLatch) {
  Pad2a03 pad;
  Joypad joypad;
  pad.port(0).plug(&joypad);
  joypad.setButtons(hw::pad_button::A);
  pad.write(0x4017, 1);
  pad.write(0x4017, 0);
  // 没有锁存过，A 键不会被读到。
  EXPECT_EQ(readBits(pad, 0x4016, 2), (std::vector<int>{0, 0}));
}

TEST(Pad2a03, OnlyTheLowFiveDataBitsAreDriven) {
  Pad2a03 pad;
  Joypad joypad;
  pad.port(0).plug(&joypad);
  joypad.setButtons(hw::pad_button::A);
  latch(pad);
  const hw::BusRead result = pad.read(0x4016);
  EXPECT_EQ(result.drivenMask, 0x1F);
  EXPECT_EQ(result.value, 0x01);

  // 经总线读取时高 3 位保持总线上的值（读 $4016 的指令最后取到的是地址高字节 $40）。
  CpuAddressBus bus;
  bus.mount(0x4016, 0x4017, pad);
  latch(pad);
  bus.write(0x1234, 0x40);
  EXPECT_EQ(bus.read(0x4016), 0x41);
}

TEST(Pad2a03, UnpluggedPortsReadZeroAndPluggingNullUnplugs) {
  Pad2a03 pad;
  EXPECT_EQ(pad.read(0x4016).value, 0);
  Joypad joypad;
  pad.port(1).plug(&joypad);
  joypad.setButtons(hw::pad_button::A);
  pad.write(0x4016, 1);
  EXPECT_EQ(pad.read(0x4017).value, 1);
  pad.port(1).plug(nullptr);
  EXPECT_EQ(pad.read(0x4017).value, 0);
}

TEST(Pad2a03, OnlyTwoPortsExist) {
  Pad2a03 pad;
  EXPECT_NO_THROW(pad.port(0));
  EXPECT_NO_THROW(pad.port(1));
  try {
    pad.port(2);
    FAIL() << "应当抛出异常";
  } catch (const ember::shared::CoreError& error) {
    EXPECT_EQ(error.code(), hw::ERROR_PORT_NOT_FOUND);
  }
}

}  // namespace
