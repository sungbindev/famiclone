/*
 * ------------------------------------------------------------------------
 * 名称：ppu-2c02-test.cpp
 * 说明：2C02 PPU 的单元测试：寄存器读写与镜像、读缓冲与调色板、滚动寄存器、vblank 与 NMI、精灵评估与命中、帧缓冲格式、帧长度。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：测试台只接 PPU、调度器、NMI 线与一块 16K 的平面显存，不用 CPU；每次推进一个点（4 个主时钟周期）再追赶。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <vector>

#include "contracts/hardware/bus-device.hpp"
#include "logic/hardware/bus/mapped-bus.hpp"
#include "logic/hardware/lines/wired-or-line.hpp"
#include "logic/hardware/ppu-2c02/ppu-2c02.hpp"
#include "logic/hardware/scheduler/master-scheduler.hpp"
#include "shared/errors/core-error.hpp"

namespace {

namespace hw = ember::contracts::hardware;
using ember::logic::hardware::MasterScheduler;
using ember::logic::hardware::PpuAddressBus;
using ember::logic::hardware::Ppu2c02;
using ember::logic::hardware::WiredOrIrqLine;

/** 覆盖 $0000-$3EFF 的平面显存（图案表与名称表不分镜像）。 */
class FlatVram final : public hw::BusDevice {
 public:
  FlatVram() : bytes(0x4000, 0) {}

  hw::BusRead read(std::uint16_t address) override { return hw::driveBus(bytes[address]); }

  void write(std::uint16_t address, std::uint8_t value) override { bytes[address] = value; }

  std::vector<std::uint8_t> bytes;
};

/** PPU 测试台。 */
class PpuRig {
 public:
  explicit PpuRig(Ppu2c02::Options options = {262, true}) : ppu(scheduler, bus, nmi, hw::ClockDomain{4}, 0, options) {
    bus.mount(0x0000, 0x3EFF, vram);
    ppu.reset(true);
  }

  /** 每次推进一个点，直到下一个要执行的点是 (line, dot)。 */
  void runTo(int line, int dot) {
    for (int guard = 0; guard < 3 * 341 * 262; ++guard) {
      if (ppu.scanline() == line && ppu.dot() == dot) return;
      step();
    }
    FAIL() << "没有到达 (" << line << "," << dot << ")";
  }

  /** 执行一个点。 */
  void step() {
    scheduler.advance(4);
    ppu.catchUp(scheduler.now());
  }

  std::uint8_t read(std::uint16_t address) { return ppu.read(address).value; }

  void write(std::uint16_t address, std::uint8_t value) { ppu.write(address, value); }

  /** 经 $2006、$2007 向 PPU 地址空间写一个字节。 */
  void writeVram(std::uint16_t address, std::uint8_t value) {
    write(0x2006, static_cast<std::uint8_t>(address >> 8));
    write(0x2006, static_cast<std::uint8_t>(address & 0xFF));
    write(0x2007, value);
  }

  /** 把 $2006 设到指定地址。 */
  void setAddress(std::uint16_t address) {
    write(0x2006, static_cast<std::uint8_t>(address >> 8));
    write(0x2006, static_cast<std::uint8_t>(address & 0xFF));
  }

  MasterScheduler scheduler;
  PpuAddressBus bus;
  WiredOrIrqLine nmi;
  FlatVram vram;
  Ppu2c02 ppu;
};

/** 一个像素的四个字节（内存顺序）。 */
std::array<std::uint8_t, 4> pixelBytes(const hw::FrameBuffer& frame, std::size_t index) {
  std::array<std::uint8_t, 4> bytes{};
  std::memcpy(bytes.data(), &frame.pixels[index], 4);
  return bytes;
}

// ---------------------------------------------------------------- 寄存器

TEST(Ppu2c02Registers, AddressesMirrorEveryEightBytes) {
  PpuRig rig;
  // $3FFB 是 $2003 的镜像，$200C 是 $2004 的镜像。
  rig.write(0x3FFB, 0x10);
  rig.write(0x200C, 0x77);
  EXPECT_EQ(rig.ppu.oam()[0x10], 0x77);
  // 写 $2004 后地址加 1，读 $2004 经镜像地址读出，不改变地址。
  rig.write(0x2003, 0x10);
  EXPECT_EQ(rig.read(0x2FFC), 0x77);
  EXPECT_EQ(rig.read(0x2004), 0x77);
}

TEST(Ppu2c02Registers, WriteOnlyRegistersReadTheLastValueOnTheBus) {
  PpuRig rig;
  rig.write(0x2000, 0x5A);
  EXPECT_EQ(rig.read(0x2000), 0x5A);
  EXPECT_EQ(rig.read(0x2001), 0x5A);
  EXPECT_EQ(rig.read(0x2005), 0x5A);
  rig.write(0x2001, 0xA5);
  EXPECT_EQ(rig.read(0x2003), 0xA5);
}

TEST(Ppu2c02Registers, StatusReadClearsVblankAndTheWriteToggleAndKeepsLowBitsFromTheBus) {
  PpuRig rig;
  rig.runTo(241, 5);
  rig.write(0x2005, 0x00);
  EXPECT_TRUE(rig.ppu.writeToggle());
  rig.write(0x2003, 0x1F);
  const std::uint8_t first = rig.read(0x2002);
  EXPECT_EQ(first & 0xE0, 0x80);
  EXPECT_EQ(first & 0x1F, 0x1F);
  EXPECT_FALSE(rig.ppu.writeToggle());
  EXPECT_EQ(rig.read(0x2002) & 0x80, 0);
}

TEST(Ppu2c02Registers, DataReadsAreBufferedAndPaletteReadsAreNot) {
  PpuRig rig;
  rig.writeVram(0x2000, 0x11);
  rig.writeVram(0x2001, 0x22);
  rig.setAddress(0x2000);
  EXPECT_EQ(rig.read(0x2007), 0x00);  // 缓冲里的旧值
  EXPECT_EQ(rig.read(0x2007), 0x11);
  EXPECT_EQ(rig.read(0x2007), 0x22);

  // 调色板直接读出，不经缓冲；缓冲里装入的是调色板下面的名称表（$2F00 起）。
  rig.writeVram(0x2F00, 0x33);
  rig.writeVram(0x3F00, 0x15);
  rig.setAddress(0x3F00);
  EXPECT_EQ(rig.read(0x2007) & 0x3F, 0x15);
  rig.setAddress(0x2000);
  EXPECT_EQ(rig.read(0x2007), 0x33);
}

TEST(Ppu2c02Registers, PaletteMirrors) {
  PpuRig rig;
  rig.writeVram(0x3F10, 0x21);
  rig.setAddress(0x3F00);
  EXPECT_EQ(rig.read(0x2007), 0x21);
  rig.writeVram(0x3F04, 0x12);
  rig.setAddress(0x3F14);
  EXPECT_EQ(rig.read(0x2007), 0x12);
  // $3F05 与 $3F15 是不同的项。
  rig.writeVram(0x3F05, 0x05);
  rig.writeVram(0x3F15, 0x15);
  rig.setAddress(0x3F05);
  EXPECT_EQ(rig.read(0x2007), 0x05);
  // $3F20 起每 32 字节镜像一次。
  rig.setAddress(0x3F20);
  EXPECT_EQ(rig.read(0x2007), 0x21);
}

TEST(Ppu2c02Registers, AddressIncrementFollowsTheControlBit) {
  PpuRig rig;
  rig.setAddress(0x2000);
  rig.read(0x2007);
  EXPECT_EQ(rig.ppu.vramAddress(), 0x2001);
  rig.write(0x2000, 0x04);
  rig.read(0x2007);
  EXPECT_EQ(rig.ppu.vramAddress(), 0x2021);
}

TEST(Ppu2c02Registers, OamDataWriteIncrementsTheAddressAndReadMasksTheAttributeBits) {
  PpuRig rig;
  rig.write(0x2003, 0x00);
  for (int i = 0; i < 4; ++i) rig.write(0x2004, 0xFF);
  EXPECT_EQ(rig.ppu.oam()[3], 0xFF);
  rig.write(0x2003, 0x02);
  EXPECT_EQ(rig.read(0x2004), 0xE3);
  rig.write(0x2003, 0x03);
  EXPECT_EQ(rig.read(0x2004), 0xFF);
}

// ---------------------------------------------------------------- 滚动

TEST(Ppu2c02Scroll, ControlAndScrollWritesFollowTheLoopyModel) {
  PpuRig rig;
  rig.write(0x2000, 0x03);
  EXPECT_EQ(rig.ppu.tempAddress(), 0x0C00);
  rig.write(0x2005, 0x7D);  // 粗 X=15，精细 X=5
  EXPECT_EQ(rig.ppu.fineX(), 5);
  EXPECT_TRUE(rig.ppu.writeToggle());
  rig.write(0x2005, 0x5E);  // 精细 Y=6，粗 Y=11
  EXPECT_FALSE(rig.ppu.writeToggle());
  EXPECT_EQ(rig.ppu.tempAddress(), 0x6000 | 0x0C00 | (11 << 5) | 15);
}

TEST(Ppu2c02Scroll, AddressWritesLoadVOnTheSecondWriteAndIgnoreBit14) {
  PpuRig rig;
  rig.write(0x2006, 0xFF);
  EXPECT_EQ(rig.ppu.tempAddress(), 0x3F00);
  EXPECT_EQ(rig.ppu.vramAddress(), 0x0000);
  rig.write(0x2006, 0x34);
  EXPECT_EQ(rig.ppu.tempAddress(), 0x3F34);
  EXPECT_EQ(rig.ppu.vramAddress(), 0x3F34);
  EXPECT_FALSE(rig.ppu.writeToggle());
}

TEST(Ppu2c02Scroll, HorizontalBitsAreCopiedAtDot257AndVerticalBitsInThePreRenderLine) {
  PpuRig rig;
  rig.write(0x2000, 0x01);
  rig.write(0x2005, 0x7D);
  rig.write(0x2005, 0x5E);
  const std::uint16_t t = rig.ppu.tempAddress();
  rig.write(0x2001, 0x08);

  rig.runTo(0, 258);
  EXPECT_EQ(rig.ppu.vramAddress() & 0x041F, t & 0x041F);

  rig.runTo(261, 279);
  EXPECT_NE(rig.ppu.vramAddress() & 0x7BE0, t & 0x7BE0);
  rig.runTo(261, 305);
  EXPECT_EQ(rig.ppu.vramAddress() & 0x7BE0, t & 0x7BE0);
}

TEST(Ppu2c02Scroll, CoarseXWrapsIntoTheNextNametableAndFineYIncrementsAtDot256) {
  PpuRig rig;
  rig.write(0x2001, 0x08);
  // 第 8、16……256 点各递增一次粗 X，共 32 次：回到 0 并翻到下一个名称表；第 256 点同时递增精细 Y。
  rig.runTo(0, 257);
  EXPECT_EQ(rig.ppu.vramAddress(), 0x1400);
  // 第 257 点从 t 复制横向位。
  rig.runTo(0, 258);
  EXPECT_EQ(rig.ppu.vramAddress(), 0x1000);
}

TEST(Ppu2c02Scroll, CoarseYWrapsAt29IntoTheNextNametableRow) {
  PpuRig rig;
  // 精细 Y=7、粗 Y=29：递增一行后粗 Y 回到 0，精细 Y 回到 0，并翻转纵向名称表。
  rig.write(0x2005, 0x00);
  rig.write(0x2005, 0xEF);
  EXPECT_EQ(rig.ppu.tempAddress(), 0x7000 | (29 << 5));
  rig.write(0x2001, 0x08);
  rig.runTo(261, 305);
  EXPECT_EQ(rig.ppu.vramAddress() & 0x7BE0, 0x7000 | (29 << 5));
  rig.runTo(0, 257);
  EXPECT_EQ(rig.ppu.vramAddress() & 0x7BE0, 0x0800);
}

TEST(Ppu2c02Scroll, DataAccessWhileRenderingIncrementsCoarseXAndY) {
  PpuRig rig;
  rig.write(0x2001, 0x08);
  rig.runTo(50, 150);
  const std::uint16_t before = rig.ppu.vramAddress();
  rig.read(0x2007);
  // 渲染期间访问 $2007 不是加 1，而是同时递增粗 X 与 Y。
  EXPECT_NE(rig.ppu.vramAddress(), static_cast<std::uint16_t>(before + 1));
  EXPECT_EQ((rig.ppu.vramAddress() >> 12) & 7, ((before >> 12) & 7) + 1);
}

// ---------------------------------------------------------------- vblank 与 NMI

TEST(Ppu2c02Vblank, FlagIsSetAtDot1OfLine241AndClearedAtDot1OfThePreRenderLine) {
  PpuRig rig;
  rig.runTo(241, 1);
  EXPECT_FALSE(rig.ppu.vblankFlag());
  rig.step();
  EXPECT_TRUE(rig.ppu.vblankFlag());
  rig.runTo(261, 1);
  EXPECT_TRUE(rig.ppu.vblankFlag());
  rig.step();
  EXPECT_FALSE(rig.ppu.vblankFlag());
}

TEST(Ppu2c02Vblank, NmiLineFollowsVblankAndTheEnableBit) {
  PpuRig rig;
  rig.runTo(241, 2);
  EXPECT_FALSE(rig.nmi.active());  // 没有使能
  rig.write(0x2000, 0x80);
  EXPECT_TRUE(rig.nmi.active());  // 在 vblank 期间使能：立即有效
  rig.write(0x2000, 0x00);
  EXPECT_FALSE(rig.nmi.active());
  rig.write(0x2000, 0x80);
  rig.read(0x2002);  // 读 $2002 清 vblank，NMI 线随之无效
  EXPECT_FALSE(rig.nmi.active());
  rig.runTo(261, 2);
  EXPECT_FALSE(rig.nmi.active());
}

TEST(Ppu2c02Vblank, NmiLineRisesWhenVblankStartsWithTheEnableBitSet) {
  PpuRig rig;
  rig.write(0x2000, 0x80);
  rig.runTo(241, 1);
  EXPECT_FALSE(rig.nmi.active());
  rig.step();
  EXPECT_TRUE(rig.nmi.active());
}

TEST(Ppu2c02Vblank, ReadingStatusOneDotBeforeVblankSuppressesTheFlagAndTheNmi) {
  PpuRig rig;
  rig.write(0x2000, 0x80);
  rig.runTo(241, 1);
  // 此时下一个要执行的点正是 (241,1)：读到 0，且本帧不置位。
  EXPECT_EQ(rig.read(0x2002) & 0x80, 0);
  rig.step();
  EXPECT_FALSE(rig.ppu.vblankFlag());
  EXPECT_FALSE(rig.nmi.active());
  // 下一帧恢复正常。
  rig.runTo(0, 5);
  rig.runTo(241, 2);
  EXPECT_TRUE(rig.ppu.vblankFlag());
  EXPECT_TRUE(rig.nmi.active());
}

TEST(Ppu2c02Vblank, SchedulerEventsRaiseAndLowerTheNmiLineWithoutAnyRegisterAccess) {
  PpuRig rig;
  rig.write(0x2000, 0x80);
  // 不经 PPU 的寄存器访问，只推进时钟：调度事件会在 vblank 起止的点上改变 NMI 线。
  rig.scheduler.advance(4 * (241 * 341 + 1) + 2);
  EXPECT_TRUE(rig.nmi.active());
  rig.scheduler.advance(4 * (20 * 341));
  EXPECT_FALSE(rig.nmi.active());
}

TEST(Ppu2c02Frame, LengthIs89342DotsAndOddFramesWithBackgroundAre89341) {
  const auto measure = [](std::uint8_t mask) {
    PpuRig rig;
    rig.write(0x2001, mask);
    std::vector<hw::MasterTime> publishTimes;
    std::uint64_t lastCount = 0;
    while (publishTimes.size() < 5) {
      rig.step();
      if (rig.ppu.frameCount() != lastCount) {
        lastCount = rig.ppu.frameCount();
        publishTimes.push_back(rig.scheduler.now());
      }
    }
    std::vector<hw::MasterTime> lengths;
    for (std::size_t i = 1; i < publishTimes.size(); ++i) lengths.push_back((publishTimes[i] - publishTimes[i - 1]) / 4);
    return lengths;
  };
  for (const hw::MasterTime length : measure(0x00)) EXPECT_EQ(length, 89342);
  const std::vector<hw::MasterTime> withBackground = measure(0x08);
  // 两帧一个周期：奇数帧少一个点。
  EXPECT_EQ(withBackground[0] + withBackground[1], 89342 + 89341);
  EXPECT_EQ(withBackground[0], withBackground[2]);
  // 只开精灵时不跳点。
  for (const hw::MasterTime length : measure(0x10)) EXPECT_EQ(length, 89342);
}

// ---------------------------------------------------------------- 精灵

/** 把一个精灵写进 OAM（渲染关闭时）。 */
void writeSprite(PpuRig& rig, int index, std::uint8_t y, std::uint8_t tile, std::uint8_t attribute, std::uint8_t x) {
  rig.write(0x2003, static_cast<std::uint8_t>(index * 4));
  rig.write(0x2004, y);
  rig.write(0x2004, tile);
  rig.write(0x2004, attribute);
  rig.write(0x2004, x);
  // 评估从 OAM 地址指向的精灵开始，所以写完后把地址清零。
  rig.write(0x2003, 0x00);
}

TEST(Ppu2c02Sprites, EvaluationCopiesSpritesInRangeToTheSecondaryOam) {
  PpuRig rig;
  writeSprite(rig, 0, 10, 5, 2, 20);
  writeSprite(rig, 1, 12, 6, 3, 30);
  writeSprite(rig, 2, 40, 7, 0, 50);
  rig.write(0x2001, 0x10);

  rig.runTo(9, 258);
  EXPECT_EQ(rig.ppu.lineSpriteCount(), 0);
  rig.runTo(10, 258);
  ASSERT_EQ(rig.ppu.lineSpriteCount(), 1);
  EXPECT_EQ(rig.ppu.secondaryOam()[0], 10);
  EXPECT_EQ(rig.ppu.secondaryOam()[1], 5);
  EXPECT_EQ(rig.ppu.secondaryOam()[2], 2);
  EXPECT_EQ(rig.ppu.secondaryOam()[3], 20);
  EXPECT_EQ(rig.ppu.secondaryOam()[4], 0xFF);
  rig.runTo(12, 258);
  EXPECT_EQ(rig.ppu.lineSpriteCount(), 2);
  EXPECT_EQ(rig.ppu.secondaryOam()[4], 12);
  // 8 行高：Y=10 的精灵在第 10-17 行被评估，Y=12 的在第 12-19 行。
  rig.runTo(18, 258);
  EXPECT_EQ(rig.ppu.lineSpriteCount(), 1);
  rig.runTo(20, 258);
  EXPECT_EQ(rig.ppu.lineSpriteCount(), 0);
}

TEST(Ppu2c02Sprites, TallSpritesAreSixteenRowsHigh) {
  PpuRig rig;
  writeSprite(rig, 0, 10, 5, 0, 20);
  rig.write(0x2000, 0x20);
  rig.write(0x2001, 0x10);
  rig.runTo(25, 258);
  EXPECT_EQ(rig.ppu.lineSpriteCount(), 1);
  rig.runTo(26, 258);
  EXPECT_EQ(rig.ppu.lineSpriteCount(), 0);
}

TEST(Ppu2c02Sprites, NineSpritesOnALineKeepEightAndSetTheOverflowFlag) {
  PpuRig rig;
  for (int i = 0; i < 8; ++i) writeSprite(rig, i, 50, static_cast<std::uint8_t>(i), 0, static_cast<std::uint8_t>(i * 8));
  rig.write(0x2001, 0x10);
  rig.runTo(50, 258);
  EXPECT_EQ(rig.ppu.lineSpriteCount(), 8);
  EXPECT_FALSE(rig.ppu.spriteOverflow());

  PpuRig rig9;
  for (int i = 0; i < 9; ++i) writeSprite(rig9, i, 50, static_cast<std::uint8_t>(i), 0, static_cast<std::uint8_t>(i * 8));
  rig9.write(0x2001, 0x10);
  rig9.runTo(50, 258);
  EXPECT_EQ(rig9.ppu.lineSpriteCount(), 8);
  EXPECT_TRUE(rig9.ppu.spriteOverflow());
  // 溢出标志在预渲染行清除。
  rig9.runTo(261, 2);
  EXPECT_FALSE(rig9.ppu.spriteOverflow());
}

/** 图案表 0 的第 1 块是实心图块，名称表 0 全用它，用于命中测试。 */
void prepareSolidBackground(PpuRig& rig) {
  for (int i = 0; i < 16; ++i) rig.vram.bytes[16 + static_cast<std::size_t>(i)] = 0xFF;
  for (int i = 0; i < 960; ++i) rig.vram.bytes[0x2000 + static_cast<std::size_t>(i)] = 1;
  rig.writeVram(0x3F00, 0x0F);
  rig.writeVram(0x3F01, 0x30);
  rig.writeVram(0x3F11, 0x16);
  // 装入调色板后把 v 与 t 清零，否则渲染会从调色板地址开始取数。
  rig.setAddress(0x0000);
}

TEST(Ppu2c02Sprites, SpriteZeroHitIsSetWhenTheOpaquePixelsOverlap) {
  PpuRig rig;
  prepareSolidBackground(rig);
  writeSprite(rig, 0, 30, 1, 0, 40);
  rig.write(0x2001, 0x1E);
  rig.runTo(31, 41);  // 下一个要执行的点画第 40 列
  EXPECT_FALSE(rig.ppu.spriteZeroHit());
  rig.step();
  EXPECT_TRUE(rig.ppu.spriteZeroHit());
  EXPECT_EQ(rig.read(0x2002) & 0x40, 0x40);
  rig.runTo(261, 2);
  EXPECT_FALSE(rig.ppu.spriteZeroHit());
}

TEST(Ppu2c02Sprites, LeftClippingHidesTheHitInTheFirstEightColumns) {
  PpuRig rig;
  prepareSolidBackground(rig);
  writeSprite(rig, 0, 30, 1, 0, 4);
  rig.write(0x2001, 0x18);  // 背景与精灵开启，左 8 列被遮罩
  rig.runTo(31, 9);
  EXPECT_FALSE(rig.ppu.spriteZeroHit());
  rig.runTo(31, 10);  // 第 8 列起不再遮罩
  EXPECT_TRUE(rig.ppu.spriteZeroHit());
}

TEST(Ppu2c02Sprites, NoHitInTheLastColumn) {
  PpuRig rig;
  prepareSolidBackground(rig);
  writeSprite(rig, 0, 30, 1, 0, 255);
  rig.write(0x2001, 0x1E);
  rig.runTo(38, 340);
  EXPECT_FALSE(rig.ppu.spriteZeroHit());
}

TEST(Ppu2c02Sprites, ASpriteWithYZeroIsFirstDrawnOnLineOne) {
  PpuRig rig;
  prepareSolidBackground(rig);
  writeSprite(rig, 0, 0, 1, 0, 40);
  rig.write(0x2001, 0x1E);
  // 精灵在第 0 行被评估，到第 1 行才画；预渲染行不评估精灵，所以第 0 行没有精灵。
  rig.runTo(0, 258);
  EXPECT_EQ(rig.ppu.lineSpriteCount(), 1);
  rig.runTo(261, 258);
  EXPECT_EQ(rig.ppu.lineSpriteCount(), 0);
}

// ---------------------------------------------------------------- 帧缓冲格式

TEST(Ppu2c02FrameBuffer, HasNoFrameBeforeTheFirstOneIsComplete) {
  PpuRig rig;
  const hw::FrameBuffer frame = rig.ppu.frame();
  EXPECT_EQ(frame.width, 0U);
  EXPECT_EQ(frame.height, 0U);
  EXPECT_TRUE(frame.pixels.empty());
}

/** 运行到下一帧完成。 */
void runOneFrame(PpuRig& rig) {
  const std::uint64_t target = rig.ppu.frameCount() + 1;
  for (int guard = 0; guard < 2 * 341 * 262 && rig.ppu.frameCount() < target; ++guard) rig.step();
}

/** 渲染关闭时，设置背景色（$3F00）并把 v 清零（v 指向调色板时输出的是 v 指向的那一项）。 */
void setBackdrop(PpuRig& rig, std::uint8_t paletteEntry) {
  rig.writeVram(0x3F00, paletteEntry);
  rig.setAddress(0x0000);
}

/** 整个画面都是背景色时，第 index 个像素的四个字节。 */
std::array<std::uint8_t, 4> backdropPixel(std::uint8_t paletteEntry, std::uint8_t mask) {
  PpuRig rig;
  setBackdrop(rig, paletteEntry);
  rig.write(0x2001, mask);
  runOneFrame(rig);
  return pixelBytes(rig.ppu.frame(), 5000);
}

TEST(Ppu2c02FrameBuffer, PixelsAreRgbaBytesWithOpaqueAlpha) {
  PpuRig rig;
  setBackdrop(rig, 0x30);
  runOneFrame(rig);
  hw::FrameBuffer frame = rig.ppu.frame();
  ASSERT_EQ(frame.width, 256U);
  ASSERT_EQ(frame.height, 240U);
  ASSERT_EQ(frame.pixels.size(), 256U * 240U);
  EXPECT_EQ(pixelBytes(frame, 0), (std::array<std::uint8_t, 4>{255, 255, 255, 255}));
  EXPECT_EQ(pixelBytes(frame, 256 * 240 - 1), (std::array<std::uint8_t, 4>{255, 255, 255, 255}));

  setBackdrop(rig, 0x0F);
  runOneFrame(rig);
  frame = rig.ppu.frame();
  EXPECT_EQ(pixelBytes(frame, 12345), (std::array<std::uint8_t, 4>{0, 0, 0, 255}));

  // 红色系的颜色号 $16：内存字节顺序为 R、G、B、A，所以第 0 个字节（R）大于第 2 个字节（B）；蓝色系 $12 相反。
  const auto red = backdropPixel(0x16, 0x00);
  EXPECT_GT(red[0], red[2]);
  EXPECT_EQ(red[3], 255);
  const auto blue = backdropPixel(0x12, 0x00);
  EXPECT_GT(blue[2], blue[0]);
  EXPECT_EQ(blue[3], 255);
}

TEST(Ppu2c02FrameBuffer, FramesAreDoubleBuffered) {
  PpuRig rig;
  setBackdrop(rig, 0x30);
  runOneFrame(rig);
  const hw::FrameBuffer first = rig.ppu.frame();
  const std::uint32_t* firstData = first.pixels.data();
  ASSERT_EQ(pixelBytes(first, 0), (std::array<std::uint8_t, 4>{255, 255, 255, 255}));

  // 改背景色后开始画下一帧：已发布的那一帧保持不变。
  setBackdrop(rig, 0x0F);
  rig.runTo(120, 0);
  EXPECT_EQ(rig.ppu.frame().pixels.data(), firstData);
  EXPECT_EQ(pixelBytes(rig.ppu.frame(), 0), (std::array<std::uint8_t, 4>{255, 255, 255, 255}));
  runOneFrame(rig);
  EXPECT_NE(rig.ppu.frame().pixels.data(), firstData);
  EXPECT_EQ(pixelBytes(rig.ppu.frame(), 0), (std::array<std::uint8_t, 4>{0, 0, 0, 255}));
}

TEST(Ppu2c02FrameBuffer, GrayscaleAndEmphasisChangeTheColors) {
  // 灰度：颜色号与 $30 相与，$16 变成 $10。
  EXPECT_EQ(backdropPixel(0x16, 0x01), backdropPixel(0x10, 0x00));
  EXPECT_NE(backdropPixel(0x16, 0x00), backdropPixel(0x10, 0x00));
  // 颜色强调：$30 白色被衰减；$0F 黑色不受影响。
  EXPECT_NE(backdropPixel(0x30, 0x20), backdropPixel(0x30, 0x00));
  EXPECT_NE(backdropPixel(0x30, 0xE0), backdropPixel(0x30, 0x20));
  EXPECT_EQ(backdropPixel(0x0F, 0xE0), backdropPixel(0x0F, 0x00));
}

TEST(Ppu2c02FrameBuffer, SpritesAreDrawnInFrontOfOrBehindTheBackgroundAndFlipHorizontally) {
  PpuRig rig;
  prepareSolidBackground(rig);
  // 精灵图块 2：只有最左边一列有像素（颜色 1），水平翻转后在最右边一列。
  for (int i = 0; i < 8; ++i) rig.vram.bytes[32 + static_cast<std::size_t>(i)] = 0x80;
  writeSprite(rig, 1, 100, 2, 0x00, 100);  // 在背景前
  writeSprite(rig, 2, 100, 2, 0x40, 120);  // 水平翻转
  writeSprite(rig, 3, 100, 2, 0x20, 140);  // 在背景后
  rig.write(0x2001, 0x1E);
  runOneFrame(rig);
  runOneFrame(rig);
  const hw::FrameBuffer frame = rig.ppu.frame();
  const auto white = pixelBytes(frame, 105 * 256 + 50);  // 背景（$3F01，白）
  // 精灵颜色 $3F11（$16，红）与背景色比较。
  PpuRig reference;
  setBackdrop(reference, 0x16);
  runOneFrame(reference);
  const auto red = pixelBytes(reference.ppu.frame(), 0);
  ASSERT_NE(red, white);

  EXPECT_EQ(pixelBytes(frame, 105 * 256 + 100), red);    // 精灵最左一列
  EXPECT_EQ(pixelBytes(frame, 105 * 256 + 107), white);  // 精灵最右一列是透明的
  EXPECT_EQ(pixelBytes(frame, 105 * 256 + 120), white);  // 翻转后最左一列透明
  EXPECT_EQ(pixelBytes(frame, 105 * 256 + 127), red);    // 翻转后最右一列
  EXPECT_EQ(pixelBytes(frame, 105 * 256 + 140), white);  // 在背景后：背景不透明，看到背景
}

// ---------------------------------------------------------------- 变体：PAL 时序

TEST(Ppu2c02Variants, PalFramesHave312LinesAndNeverSkipADot) {
  PpuRig rig(Ppu2c02::Options{312, false});
  rig.write(0x2001, 0x08);  // 背景开启：NTSC 的奇数帧会少一个点
  // 测试台的第一次推进会连续执行时刻 0 与 4 的两个点，第 0 帧的步数因此少 1，不计入。
  for (int frame = 0; frame < 4; ++frame) {
    int steps = 0;
    do {
      rig.step();
      ++steps;
    } while (!(rig.ppu.scanline() == 0 && rig.ppu.dot() == 0) && steps < 400000);
    if (frame > 0) EXPECT_EQ(steps, 312 * 341) << "第 " << frame << " 帧";
  }
}

TEST(Ppu2c02Variants, PalVblankStartsOnLine241AndEndsOnTheLastLine) {
  PpuRig rig(Ppu2c02::Options{312, false});
  rig.write(0x2000, 0x80);
  rig.runTo(241, 2);
  EXPECT_TRUE(rig.ppu.vblankFlag());
  EXPECT_TRUE(rig.nmi.active());
  rig.runTo(310, 0);
  EXPECT_TRUE(rig.ppu.vblankFlag());
  rig.runTo(311, 2);
  EXPECT_FALSE(rig.ppu.vblankFlag());
  EXPECT_FALSE(rig.nmi.active());
}

TEST(Ppu2c02Variants, RejectsAFrameThatIsTooShort) {
  MasterScheduler scheduler;
  PpuAddressBus bus;
  WiredOrIrqLine nmi;
  EXPECT_THROW(Ppu2c02(scheduler, bus, nmi, hw::ClockDomain{4}, 0, Ppu2c02::Options{241, true}), ember::shared::CoreError);
  EXPECT_THROW(Ppu2c02(scheduler, bus, nmi, hw::ClockDomain{4}, 0, Ppu2c02::Options{312, false, 312}), ember::shared::CoreError) << "每帧的行数必须大于 vblank 起始行";
  EXPECT_THROW(Ppu2c02(scheduler, bus, nmi, hw::ClockDomain{4}, 0, Ppu2c02::Options{312, false, 240}), ember::shared::CoreError) << "vblank 起始行不能小于 241";
}

TEST(Ppu2c02Variants, DendyVblankStartsOnLine291AndEndsOnTheLastLine) {
  PpuRig rig(Ppu2c02::Options{312, false, 291});
  rig.write(0x2000, 0x80);
  rig.runTo(260, 0);
  EXPECT_FALSE(rig.ppu.vblankFlag());
  EXPECT_FALSE(rig.nmi.active());
  rig.runTo(291, 2);
  EXPECT_TRUE(rig.ppu.vblankFlag());
  EXPECT_TRUE(rig.nmi.active());
  rig.runTo(311, 2);
  EXPECT_FALSE(rig.ppu.vblankFlag());
  EXPECT_FALSE(rig.nmi.active());
}

// ---------------------------------------------------------------- 变体：背景图案表的附加选择

/** 图案表 0 的第 1 块是实心图块（颜色 1），图案表 1 的第 1 块是空图块；名称表 0 与 2 全用第 1 块；背景色黑，颜色 1 白。 */
void prepareTwoTables(PpuRig& rig) {
  for (int i = 0; i < 8; ++i) rig.vram.bytes[16 + static_cast<std::size_t>(i)] = 0xFF;
  for (int i = 0; i < 960; ++i) {
    rig.vram.bytes[0x2000 + static_cast<std::size_t>(i)] = 1;
    rig.vram.bytes[0x2800 + static_cast<std::size_t>(i)] = 1;
  }
  rig.writeVram(0x3F00, 0x0F);
  rig.writeVram(0x3F01, 0x30);
  rig.setAddress(0x0000);
}

/** 渲染一整帧（背景开启、不遮罩左边），返回画面中央的像素。 */
std::array<std::uint8_t, 4> centerPixelOfNametable(PpuRig& rig, std::uint8_t control) {
  prepareTwoTables(rig);
  rig.write(0x2000, control);
  rig.write(0x2001, 0x0A);
  runOneFrame(rig);
  runOneFrame(rig);
  return pixelBytes(rig.ppu.frame(), 120 * 256 + 128);
}

TEST(Ppu2c02Variants, BackgroundTableOverrideSendsTheChosenNametablesToPatternTableOne) {
  const std::array<std::uint8_t, 4> white{255, 255, 255, 255};
  const std::array<std::uint8_t, 4> black{0, 0, 0, 255};

  PpuRig plain(Ppu2c02::Options{312, false, 291});
  EXPECT_EQ(centerPixelOfNametable(plain, 0x00), white);
  PpuRig plainTwo(Ppu2c02::Options{312, false, 291});
  EXPECT_EQ(centerPixelOfNametable(plainTwo, 0x02), white) << "没有回调时名称表 2 与名称表 0 一样用 $2000 位 4 选的图案表 0";

  Ppu2c02::Options options{312, false, 291};
  options.backgroundTableOverride = [](unsigned nametable) { return nametable == 2; };
  PpuRig forced(options);
  EXPECT_EQ(centerPixelOfNametable(forced, 0x00), white) << "名称表 0 不受影响";
  PpuRig forcedTwo(options);
  EXPECT_EQ(centerPixelOfNametable(forcedTwo, 0x02), black) << "名称表 2 的图块改取图案表 1（空图块）";
  PpuRig forcedTable(options);
  EXPECT_EQ(centerPixelOfNametable(forcedTable, 0x12), black) << "$2000 位 4 为 1 时本来就取图案表 1";
}

// ---------------------------------------------------------------- 变体：精灵取色后移

/** 只开精灵：图块 2 最左一列是颜色 1，图块 3 最左一列是颜色 2；背景色黑，精灵调色板 0 的第 1、2、3 项是红、绿、蓝。返回两个精灵最左一列的像素。 */
std::array<std::array<std::uint8_t, 4>, 2> spriteColumns(const Ppu2c02::Options& options) {
  PpuRig rig(options);
  for (int i = 0; i < 8; ++i) {
    rig.vram.bytes[32 + static_cast<std::size_t>(i)] = 0x80;
    rig.vram.bytes[48 + 8 + static_cast<std::size_t>(i)] = 0x80;
  }
  rig.writeVram(0x3F00, 0x0F);
  rig.writeVram(0x3F11, 0x16);
  rig.writeVram(0x3F12, 0x2A);
  rig.writeVram(0x3F13, 0x12);
  rig.setAddress(0x0000);
  writeSprite(rig, 0, 100, 2, 0x00, 100);
  writeSprite(rig, 1, 100, 3, 0x00, 120);
  rig.write(0x2001, 0x14);
  runOneFrame(rig);
  runOneFrame(rig);
  return {pixelBytes(rig.ppu.frame(), 105 * 256 + 100), pixelBytes(rig.ppu.frame(), 105 * 256 + 120)};
}

TEST(Ppu2c02Variants, SpritePaletteShiftMovesTheSpriteColorsByTwoEntries) {
  const auto red = backdropPixel(0x16, 0x00);
  const auto green = backdropPixel(0x2A, 0x00);
  const auto blue = backdropPixel(0x12, 0x00);
  const auto black = backdropPixel(0x0F, 0x00);

  const auto plain = spriteColumns(Ppu2c02::Options{312, false, 291});
  EXPECT_EQ(plain[0], red);
  EXPECT_EQ(plain[1], green);

  Ppu2c02::Options options{312, false, 291};
  bool shifted = true;
  options.spritePaletteShifted = [&shifted] { return shifted; };
  const auto moved = spriteColumns(options);
  EXPECT_EQ(moved[0], blue) << "颜色 1 取调色板的第 3 项";
  EXPECT_EQ(moved[1], black) << "颜色 2 落在下一个调色板的第 0 项，取背景色";

  shifted = false;
  EXPECT_EQ(spriteColumns(options), plain) << "回调返回 false 时与不设回调一样";
}

}  // namespace
