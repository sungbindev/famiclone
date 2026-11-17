/*
 * ------------------------------------------------------------------------
 * 名称：mmc2-test.cpp
 * 说明：MMC2 与 MMC4 芯片的测试：寄存器分到页号、两个锁存器按图案地址切换（下一次访问才生效、页号变化标志）、MMC2 与 MMC4 的锁存地址范围与程序页大小、镜像位、上电复位。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-18
 * 备注：芯片只给页号；页号落到哪块存储由使用它的板子（卡带插件）决定。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <cstdint>

#include "logic/hardware/mapper-chips/mmc2/mmc2.hpp"

namespace {

using ember::contracts::hardware::BankSpace;
using ember::contracts::hardware::CircuitMirroring;
using ember::logic::hardware::mapper_chips::Mmc2;

TEST(Mmc2, ProgramPagesAreEightKilobytesWithTheLastThreeFixedAndTheLeftChosenByA000) {
  Mmc2 chip(Mmc2::Model::Mmc2);
  chip.reset();
  chip.writeRegister(0xA000, 0xF3);
  EXPECT_EQ(chip.bankSize(BankSpace::Program, 0x8000), 8192U);
  EXPECT_EQ(chip.bank(BankSpace::Program, 0x8000), 3U);
  EXPECT_EQ(chip.bank(BankSpace::Program, 0xA000), 0x3DU);
  EXPECT_EQ(chip.bank(BankSpace::Program, 0xC000), 0x3EU);
  EXPECT_EQ(chip.bank(BankSpace::Program, 0xE000), 0x3FU);
}

TEST(Mmc4, ProgramPagesAreSixteenKilobytesWithTheLastFixed) {
  Mmc2 chip(Mmc2::Model::Mmc4);
  chip.reset();
  chip.writeRegister(0xA000, 0x05);
  EXPECT_EQ(chip.bankSize(BankSpace::Program, 0x8000), 16384U);
  EXPECT_EQ(chip.bank(BankSpace::Program, 0x8000), 5U);
  EXPECT_EQ(chip.bank(BankSpace::Program, 0xC000), 0x3FU);
}

TEST(Mmc2, ALatchChangeTakesEffectOnTheNextAccessAndRaisesTheBankChangeFlag) {
  Mmc2 chip(Mmc2::Model::Mmc2);
  chip.reset();
  chip.writeRegister(0xB000, 5);
  chip.writeRegister(0xC000, 6);
  chip.writeRegister(0xD000, 7);
  chip.writeRegister(0xE000, 8);
  // 上电两个锁存器都选第二个页。
  EXPECT_EQ(chip.bank(BankSpace::Character, 0x0000), 6U);
  EXPECT_EQ(chip.bank(BankSpace::Character, 0x1000), 8U);
  EXPECT_EQ(chip.bankSize(BankSpace::Character, 0), 4096U);

  chip.onVideoAddress(0x0FD8);
  EXPECT_EQ(chip.bank(BankSpace::Character, 0x0000), 6U);
  EXPECT_FALSE(chip.takeBankChange());
  chip.onVideoAddress(0x0000);
  EXPECT_EQ(chip.bank(BankSpace::Character, 0x0000), 5U);
  EXPECT_TRUE(chip.takeBankChange());
  EXPECT_FALSE(chip.takeBankChange());
}

TEST(Mmc2, LeftLatchAddressesDifferBetweenMmc2AndMmc4) {
  Mmc2 mmc2(Mmc2::Model::Mmc2);
  Mmc2 mmc4(Mmc2::Model::Mmc4);
  for (Mmc2* chip : {&mmc2, &mmc4}) {
    chip->reset();
    chip->writeRegister(0xB000, 1);
    chip->writeRegister(0xC000, 2);
  }
  for (Mmc2* chip : {&mmc2, &mmc4}) {
    chip->onVideoAddress(0x0FDD);
    chip->onVideoAddress(0x0000);
  }
  EXPECT_EQ(mmc2.bank(BankSpace::Character, 0x0000), 2U);
  EXPECT_EQ(mmc4.bank(BankSpace::Character, 0x0000), 1U);
  // 右半区两个型号都认 $1FD8-$1FDF、$1FE8-$1FEF。
  for (Mmc2* chip : {&mmc2, &mmc4}) {
    chip->writeRegister(0xD000, 3);
    chip->writeRegister(0xE000, 4);
    chip->onVideoAddress(0x1FDF);
    chip->onVideoAddress(0x0000);
    EXPECT_EQ(chip->bank(BankSpace::Character, 0x1000), 3U);
    chip->onVideoAddress(0x1FE8);
    chip->onVideoAddress(0x0000);
    EXPECT_EQ(chip->bank(BankSpace::Character, 0x1000), 4U);
  }
}

TEST(Mmc2, WritingAPageRegisterAppliesTheCurrentLatchAtOnce) {
  Mmc2 chip(Mmc2::Model::Mmc2);
  chip.reset();
  chip.onVideoAddress(0x1FD8);
  chip.onVideoAddress(0x0000);
  chip.writeRegister(0xD000, 9);
  EXPECT_EQ(chip.bank(BankSpace::Character, 0x1000), 9U);
}

TEST(Mmc2, MirroringIsOnlyReportedAfterF000IsWritten) {
  Mmc2 chip(Mmc2::Model::Mmc2);
  chip.reset();
  EXPECT_FALSE(chip.mirroring().has_value());
  chip.writeRegister(0xF000, 1);
  EXPECT_EQ(chip.mirroring(), CircuitMirroring::Horizontal);
  chip.writeRegister(0xF000, 0);
  EXPECT_EQ(chip.mirroring(), CircuitMirroring::Vertical);
  chip.reset();
  EXPECT_FALSE(chip.mirroring().has_value());
}

}  // namespace
