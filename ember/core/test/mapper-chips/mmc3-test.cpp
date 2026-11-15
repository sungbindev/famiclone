/*
 * ------------------------------------------------------------------------
 * 名称：mmc3-test.cpp
 * 说明：MMC3 芯片的测试：程序页与图案页的页号（含选择位的模式与反转）、名称表镜像位、A12 边沿的 IRQ 计数（滤波、重载、允许与撤销）、上电复位。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-16
 * 备注：芯片只给页号，页号落到哪块存储由使用它的板子决定，板子的换算在使用它的主机型号的测试里（如 sb2000-fc-mode-test.cpp）。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <cstdint>

#include "logic/hardware/lines/wired-or-line.hpp"
#include "logic/mapper-chips/mmc3/mmc3.hpp"

namespace {

using ember::logic::hardware::WiredOrIrqLine;
using ember::logic::mapper_chips::Mmc3;

/** 芯片测试台。 */
struct Rig {
  Rig() : chip(irq) {}

  /** 写页寄存器 R0-R7，保持选择位。 */
  void setRegister(unsigned index, std::uint8_t value) {
    chip.writeRegister(0x8000, static_cast<std::uint8_t>((chip.selector() & 0xC0U) | index));
    chip.writeRegister(0x8001, value);
  }

  /** 设置 $8000 的选择位（位 7 反转图案、位 6 程序模式），选中的寄存器号为 0。 */
  void setSelectorBits(std::uint8_t bits) { chip.writeRegister(0x8000, bits); }

  /** 一次 A12 上升沿：先有足够多的低电平访问，再变高。 */
  void clockEdge() {
    for (unsigned i = 0; i < Mmc3::A12_LOW_ACCESSES; ++i) chip.onVideoAddress(0x0000);
    chip.onVideoAddress(0x1000);
  }

  WiredOrIrqLine irq;
  Mmc3 chip;
};

TEST(Mmc3, ProgramBanksFollowTheRegistersAndTheModeBit) {
  Rig rig;
  // 上电值：R6、R7 为 0，$C000 与 $E000 固定为倒数第 2 页与最后一页（页号线全为高）。
  EXPECT_EQ(rig.chip.programBank(0x8000), 0U);
  EXPECT_EQ(rig.chip.programBank(0xA000), 0U);
  EXPECT_EQ(rig.chip.programBank(0xC000), Mmc3::SECOND_LAST_PROGRAM_BANK);
  EXPECT_EQ(rig.chip.programBank(0xE000), Mmc3::LAST_PROGRAM_BANK);
  EXPECT_EQ(rig.chip.programBank(0xFFFC), Mmc3::LAST_PROGRAM_BANK);

  rig.setRegister(6, 5);
  rig.setRegister(7, 9);
  EXPECT_EQ(rig.chip.programBank(0x8123), 5U);
  EXPECT_EQ(rig.chip.programBank(0xA000), 9U);
  EXPECT_EQ(rig.chip.programBank(0xC000), Mmc3::SECOND_LAST_PROGRAM_BANK);

  // 选择位 6：$8000 固定为倒数第 2 页，$C000 由 R6 选。
  rig.setSelectorBits(0x40);
  EXPECT_EQ(rig.chip.programBank(0x8000), Mmc3::SECOND_LAST_PROGRAM_BANK);
  EXPECT_EQ(rig.chip.programBank(0xC000), 5U);
  EXPECT_EQ(rig.chip.programBank(0xE000), Mmc3::LAST_PROGRAM_BANK);

  // 程序页号是 6 位。
  rig.setSelectorBits(0x00);
  rig.setRegister(6, 0xFF);
  EXPECT_EQ(rig.chip.programBank(0x8000), 0x3FU);
}

TEST(Mmc3, CharacterBanksFollowTheRegistersAndTheInversionBit) {
  Rig rig;
  rig.setRegister(0, 4);
  rig.setRegister(1, 10);
  rig.setRegister(2, 21);
  rig.setRegister(3, 22);
  rig.setRegister(4, 23);
  rig.setRegister(5, 24);
  // 不反转：$0000-$07FF 是 R0 的 2K，$0800-$0FFF 是 R1 的 2K，$1000-$1FFF 是 R2-R5 的 1K。
  EXPECT_EQ(rig.chip.characterBank(0x0000), 4U);
  EXPECT_EQ(rig.chip.characterBank(0x0400), 5U);
  EXPECT_EQ(rig.chip.characterBank(0x0800), 10U);
  EXPECT_EQ(rig.chip.characterBank(0x0C10), 11U);
  EXPECT_EQ(rig.chip.characterBank(0x1000), 21U);
  EXPECT_EQ(rig.chip.characterBank(0x1400), 22U);
  EXPECT_EQ(rig.chip.characterBank(0x1800), 23U);
  EXPECT_EQ(rig.chip.characterBank(0x1FFF), 24U);
  // 反转：两个半区对调。
  rig.setSelectorBits(0x80);
  EXPECT_EQ(rig.chip.characterBank(0x0000), 21U);
  EXPECT_EQ(rig.chip.characterBank(0x0C00), 24U);
  EXPECT_EQ(rig.chip.characterBank(0x1000), 4U);
  EXPECT_EQ(rig.chip.characterBank(0x1400), 5U);
  EXPECT_EQ(rig.chip.characterBank(0x1800), 10U);
  // R0、R1 取偶数页，R2-R5 取全部 8 位。
  rig.setSelectorBits(0x00);
  rig.setRegister(0, 0xFF);
  EXPECT_EQ(rig.chip.characterBank(0x0000), 0xFEU);
  EXPECT_EQ(rig.chip.characterBank(0x0400), 0xFFU);
  rig.setRegister(2, 0xFF);
  EXPECT_EQ(rig.chip.characterBank(0x1000), 0xFFU);
}

TEST(Mmc3, MirrorBitFollowsTheEvenAddressesOfTheMirrorRegister) {
  Rig rig;
  EXPECT_FALSE(rig.chip.horizontalMirroring()) << "上电后垂直镜像";
  rig.chip.writeRegister(0xA000, 1);
  EXPECT_TRUE(rig.chip.horizontalMirroring());
  rig.chip.writeRegister(0xA001, 0);  // PRG RAM 保护，忽略
  EXPECT_TRUE(rig.chip.horizontalMirroring());
  rig.chip.writeRegister(0xBFFE, 0);  // 偶地址都是镜像寄存器
  EXPECT_FALSE(rig.chip.horizontalMirroring());
}

TEST(Mmc3, IrqCountsA12RisesAndHoldsUntilAcknowledged) {
  Rig rig;
  rig.chip.writeRegister(0xC000, 2);  // 锁存值
  rig.chip.writeRegister(0xC001, 0);  // 下一次计数时重载
  rig.chip.writeRegister(0xE001, 0);  // 允许
  rig.clockEdge();                    // 装入 2
  EXPECT_EQ(rig.chip.irqCounter(), 2);
  EXPECT_FALSE(rig.irq.active());
  rig.clockEdge();  // 1
  EXPECT_FALSE(rig.irq.active());
  rig.clockEdge();  // 0：拉起 IRQ
  EXPECT_EQ(rig.chip.irqCounter(), 0);
  EXPECT_TRUE(rig.irq.active());
  rig.clockEdge();  // 计数器为 0 时重载，IRQ 仍然保持
  EXPECT_EQ(rig.chip.irqCounter(), 2);
  EXPECT_TRUE(rig.irq.active());
  rig.chip.writeRegister(0xE000, 0);  // 关闭并撤销
  EXPECT_FALSE(rig.irq.active());
  EXPECT_FALSE(rig.chip.irqEnabled());
  rig.clockEdge();
  rig.clockEdge();
  EXPECT_FALSE(rig.irq.active()) << "关闭后计数到 0 也不产生 IRQ";
}

TEST(Mmc3, ALatchOfZeroRaisesTheIrqOnEveryEdge) {
  Rig rig;
  rig.chip.writeRegister(0xC000, 0);
  rig.chip.writeRegister(0xE001, 0);
  rig.clockEdge();
  EXPECT_TRUE(rig.irq.active());
}

TEST(Mmc3, ShortLowPeriodsAndRepeatedHighAccessesDoNotClock) {
  Rig rig;
  rig.chip.writeRegister(0xC000, 5);
  rig.chip.writeRegister(0xC001, 0);
  rig.clockEdge();
  ASSERT_EQ(rig.chip.irqCounter(), 5);
  // 低电平只有 2 次访问（图块取名称与属性）：不计数。
  rig.chip.onVideoAddress(0x2000);
  rig.chip.onVideoAddress(0x23C0);
  rig.chip.onVideoAddress(0x1000);
  EXPECT_EQ(rig.chip.irqCounter(), 5);
  // 一直是高电平：不计数。
  for (int i = 0; i < 10; ++i) rig.chip.onVideoAddress(0x1FF0);
  EXPECT_EQ(rig.chip.irqCounter(), 5);
  // 名称表镜像区 $3000-$3EFF 的 A12 也是 1。
  for (unsigned i = 0; i < Mmc3::A12_LOW_ACCESSES; ++i) rig.chip.onVideoAddress(0x2000);
  rig.chip.onVideoAddress(0x3000);
  EXPECT_EQ(rig.chip.irqCounter(), 4);
}

TEST(Mmc3, DisablingTheIrqFromTheBoardClearsTheRequest) {
  Rig rig;
  rig.chip.writeRegister(0xC000, 0);
  rig.chip.writeRegister(0xE001, 0);
  rig.clockEdge();
  ASSERT_TRUE(rig.irq.active());
  rig.chip.disableIrq();
  EXPECT_FALSE(rig.irq.active());
  EXPECT_FALSE(rig.chip.irqEnabled());
}

TEST(Mmc3, ResetClearsEverything) {
  Rig rig;
  rig.setRegister(6, 7);
  rig.chip.writeRegister(0xA000, 1);
  rig.chip.writeRegister(0xC000, 9);
  rig.chip.writeRegister(0xE001, 0);
  rig.chip.reset();
  EXPECT_EQ(rig.chip.registerValue(6), 0);
  EXPECT_FALSE(rig.chip.horizontalMirroring());
  EXPECT_EQ(rig.chip.irqLatch(), 0);
  EXPECT_FALSE(rig.chip.irqEnabled());
}

}  // namespace
