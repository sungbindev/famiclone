/*
 * ------------------------------------------------------------------------
 * 名称：ppu-um6576-test.cpp
 * 说明：6576 PPU 的测试：寄存器读写与副作用、垂直消隐与 NMI 的时间、帧发布、4 色与 16 色背景、精灵、遮罩位、$2008 的单页名称表。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：显存用一块 64K 的内存挂在 16 位的 PPU 总线上；时钟用 PAL 的每点 5 个主时钟周期，每行 1705 个主时钟周期。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include "contracts/hardware/bus-device.hpp"
#include "logic/hardware/bus/mapped-bus.hpp"
#include "logic/hardware/lines/wired-or-line.hpp"
#include "logic/hardware/ppu-um6576/ppu-um6576.hpp"
#include "logic/hardware/scheduler/master-scheduler.hpp"

namespace {

namespace hw = ember::contracts::hardware;
using ember::logic::hardware::MappedBus;
using ember::logic::hardware::MasterScheduler;
using ember::logic::hardware::PpuUm6576;
using ember::logic::hardware::WiredOrIrqLine;

/** 一行的主时钟周期数。 */
constexpr hw::MasterTime LINE = 341 * 5;

/** 64K 显存。 */
class Vram final : public hw::BusDevice {
 public:
  Vram() : bytes(0x10000, 0) {}
  hw::BusRead read(std::uint16_t address) override { return hw::driveBus(bytes[address]); }
  void write(std::uint16_t address, std::uint8_t value) override { bytes[address] = value; }
  std::vector<std::uint8_t> bytes;
};

struct PpuBench {
  PpuBench() : ppu(scheduler, bus, nmi, hw::ClockDomain{5}, 0) {
    bus.mount(0x0000, 0xFFFF, vram);
    ppu.reset(true);
  }
  /** 推进指定行数，之后第 0 到累计行数减 1 行都已处理（第 n 行的事件在 n 乘以行长的时刻，所以每次少走 1 个主时钟周期）；每次至少推进 1 行。 */
  void runLines(int lines) {
    totalLines_ += lines;
    scheduler.advance(totalLines_ * LINE - 1 - scheduler.now());
  }
  /** 画面里一个像素（0xAABBGGRR）。 */
  [[nodiscard]] std::uint32_t pixel(unsigned x, unsigned y) const { return ppu.frame().pixels[y * 256 + x]; }

  MasterScheduler scheduler;
  WiredOrIrqLine nmi;
  MappedBus<hw::PpuBus, 16> bus;
  Vram vram;
  PpuUm6576 ppu;

 private:
  int totalLines_{0};
};

constexpr std::uint32_t WHITE = 0xFFECEEEC;
constexpr std::uint32_t BLACK = 0xFF000000;

TEST(PpuUm6576, StatusLowBitsAreTheLastWrittenValue) {
  PpuBench bench;
  bench.ppu.write(0x2003, 0x1F);
  EXPECT_EQ(bench.ppu.read(0x2002).value & 0x1F, 0x1F);
  bench.ppu.write(0x2000, 0x00);
  EXPECT_EQ(bench.ppu.read(0x2002).value & 0x1F, 0x00);
}

TEST(PpuUm6576, VerticalBlankIsSetOnLine241AndClearedOnTheLastLine) {
  PpuBench bench;
  bench.runLines(241);
  EXPECT_EQ(bench.ppu.status() & 0x80, 0) << "第 241 行还没有开始";
  bench.runLines(1);
  EXPECT_EQ(bench.ppu.status() & 0x80, 0x80);
  bench.runLines(69);
  EXPECT_EQ(bench.ppu.status() & 0x80, 0x80);
  bench.runLines(1);
  EXPECT_EQ(bench.ppu.status() & 0xE0, 0x00) << "预渲染行清除位 7、6、5";
}

TEST(PpuUm6576, ReadingStatusClearsVerticalBlankButNotTheOtherFlags) {
  PpuBench bench;
  bench.runLines(242);
  EXPECT_EQ(bench.ppu.read(0x2002).value & 0xE0, 0x80);
  EXPECT_EQ(bench.ppu.read(0x2002).value & 0xE0, 0x00);
}

TEST(PpuUm6576, NmiFollowsVerticalBlankWhenEnabled) {
  PpuBench bench;
  bench.runLines(242);
  EXPECT_FALSE(bench.nmi.active()) << "$2000 位 7 为 0，不发 NMI";
  bench.ppu.write(0x2000, 0x80);
  EXPECT_TRUE(bench.nmi.active()) << "在垂直消隐期间使能，NMI 立即有效";
  (void)bench.ppu.read(0x2002);
  EXPECT_FALSE(bench.nmi.active());
}

TEST(PpuUm6576, NmiRaisesOnLine241WhenAlreadyEnabled) {
  PpuBench bench;
  bench.ppu.write(0x2000, 0x80);
  bench.runLines(241);
  EXPECT_FALSE(bench.nmi.active());
  bench.runLines(1);
  EXPECT_TRUE(bench.nmi.active());
}

TEST(PpuUm6576, FramePublishedAfterTheLastVisibleLine) {
  PpuBench bench;
  EXPECT_EQ(bench.ppu.frame().width, 0U) << "没有完成过帧时宽高为 0";
  bench.runLines(240);
  EXPECT_EQ(bench.ppu.frameCount(), 0U);
  bench.runLines(1);
  EXPECT_EQ(bench.ppu.frameCount(), 1U);
  EXPECT_EQ(bench.ppu.frame().width, 256U);
  EXPECT_EQ(bench.ppu.frame().height, 240U);
  bench.runLines(312);
  EXPECT_EQ(bench.ppu.frameCount(), 2U) << "每帧 312 行";
}

TEST(PpuUm6576, DataPortReadsAreBufferedAndAddressIncrements) {
  PpuBench bench;
  bench.vram.bytes[0x0100] = 0x11;
  bench.vram.bytes[0x0101] = 0x22;
  bench.ppu.write(0x2006, 0x01);
  bench.ppu.write(0x2006, 0x00);
  EXPECT_EQ(bench.ppu.read(0x2007).value, 0x00) << "第一次读是缓冲里的旧值";
  EXPECT_EQ(bench.ppu.read(0x2007).value, 0x11);
  EXPECT_EQ(bench.ppu.read(0x2007).value, 0x22);
}

TEST(PpuUm6576, DataPortWritesGoToVramAndIncrementBy32WhenSelected) {
  PpuBench bench;
  bench.ppu.write(0x2000, 0x04);
  bench.ppu.write(0x2006, 0x80);
  bench.ppu.write(0x2006, 0x00);
  bench.ppu.write(0x2007, 0xAA);
  bench.ppu.write(0x2007, 0xBB);
  EXPECT_EQ(bench.vram.bytes[0x8000], 0xAA);
  EXPECT_EQ(bench.vram.bytes[0x8020], 0xBB);
  EXPECT_EQ(bench.ppu.vramAddress(), 0x8040);
}

TEST(PpuUm6576, SpriteAttributeReadbackForcesBitsFourToTwo) {
  PpuBench bench;
  bench.ppu.write(0x2003, 0x02);
  bench.ppu.write(0x2004, 0x00);
  bench.ppu.write(0x2003, 0x02);
  EXPECT_EQ(bench.ppu.read(0x2004).value, 0x1C);
  bench.ppu.write(0x2003, 0x01);
  bench.ppu.write(0x2004, 0x00);
  bench.ppu.write(0x2003, 0x01);
  EXPECT_EQ(bench.ppu.read(0x2004).value, 0x00) << "只有属性字节（偏移 2）受影响";
}

TEST(PpuUm6576, PaletteKeepsSixBitsAndReadsWithBitSix) {
  PpuBench bench;
  bench.ppu.write(0x2040, 0xFF);
  EXPECT_EQ(bench.ppu.read(0x2040).value, 0x7F);
  bench.ppu.write(0x207F, 0x15);
  EXPECT_EQ(bench.ppu.read(0x207F).value, 0x55);
}

TEST(PpuUm6576, Register2008ReadsBack) {
  PpuBench bench;
  bench.ppu.write(0x2008, 0x8D);
  EXPECT_EQ(bench.ppu.read(0x2008).value, 0x8D);
}

TEST(PpuUm6576, UnusedAddressesDoNotDriveTheBus) {
  PpuBench bench;
  EXPECT_EQ(bench.ppu.read(0x2010).drivenMask, 0);
  EXPECT_EQ(bench.ppu.read(0x2009).value, 0);
  EXPECT_EQ(bench.ppu.read(0x2009).drivenMask, 0xFF);
}

TEST(PpuUm6576, Address3FFFReadsSixtyEightLikeTheRealMachine) {
  PpuBench bench;
  EXPECT_EQ(bench.ppu.read(0x3FFF).value, 0x68);
  EXPECT_EQ(bench.ppu.read(0x3FFF).drivenMask, 0xFF);
  EXPECT_EQ(bench.ppu.read(0x3FFE).drivenMask, 0) << "其他未用地址仍然不驱动总线";
}

/** 在名称表页 1（$0800）放一个图块，画面左上角显示它的第 0 行。 */
void setUpFourColorTile(PpuBench& bench, std::uint8_t mask) {
  bench.vram.bytes[0x0800] = 0x01;  // 图块号 1，调色板组 0
  bench.vram.bytes[0x0801] = 0x00;
  bench.vram.bytes[0x0010] = 0xFF;  // 图块 1 第 0 行的低位面全为 1
  bench.ppu.pokePalette(0, 0x0F);
  bench.ppu.pokePalette(1, 0x30);
  bench.ppu.write(0x2000, 0x01);
  bench.ppu.write(0x2001, mask);
}

TEST(PpuUm6576, FourColorBackgroundDrawsTheTile) {
  PpuBench bench;
  setUpFourColorTile(bench, 0x0A);
  bench.runLines(241);
  EXPECT_EQ(bench.pixel(0, 0), WHITE);
  EXPECT_EQ(bench.pixel(7, 0), WHITE);
  EXPECT_EQ(bench.pixel(8, 0), BLACK);
  EXPECT_EQ(bench.pixel(0, 1), BLACK) << "第 1 行的图案是 0，显示调色板第 0 项";
}

TEST(PpuUm6576, LeftEightPixelsAreMaskedWhenBackgroundLeftBitIsClear) {
  PpuBench bench;
  setUpFourColorTile(bench, 0x08);
  bench.runLines(241);
  EXPECT_EQ(bench.pixel(0, 0), BLACK);
  EXPECT_EQ(bench.pixel(7, 0), BLACK);
}

TEST(PpuUm6576, BackgroundOffShowsColorZero) {
  PpuBench bench;
  setUpFourColorTile(bench, 0x00);
  bench.runLines(241);
  EXPECT_EQ(bench.pixel(0, 0), 0xFF545454) << "背景关闭时画面是颜色 $00";
}

TEST(PpuUm6576, SixteenColorModeUsesFourPlanes) {
  PpuBench bench;
  bench.vram.bytes[0x0800] = 0x02;
  bench.vram.bytes[0x0801] = 0x00;
  bench.vram.bytes[0x0020 + 0] = 0xFF;   // 位面 0
  bench.vram.bytes[0x0020 + 16] = 0xFF;  // 位面 2：颜色号 1 + 4 = 5
  bench.ppu.pokePalette(0, 0x0F);
  bench.ppu.pokePalette(5, 0x30);
  bench.ppu.write(0x2008, 0x80);
  bench.ppu.write(0x2000, 0x01);
  bench.ppu.write(0x2001, 0x0A);
  bench.runLines(241);
  EXPECT_EQ(bench.pixel(0, 0), WHITE);
  EXPECT_EQ(bench.pixel(8, 0), BLACK);
}

TEST(PpuUm6576, SinglePageNameTableStartsAtTwoThousand) {
  PpuBench bench;
  bench.vram.bytes[0x2000] = 0x01;
  bench.vram.bytes[0x2001] = 0x00;
  bench.vram.bytes[0x0010] = 0xFF;
  bench.ppu.pokePalette(0, 0x0F);
  bench.ppu.pokePalette(1, 0x30);
  bench.ppu.write(0x2008, 0x01);
  bench.ppu.write(0x2000, 0x03);  // 单页模式下 $2000 位 1-0 不起作用
  bench.ppu.write(0x2001, 0x0A);
  bench.runLines(241);
  EXPECT_EQ(bench.pixel(0, 0), WHITE);
}

TEST(PpuUm6576, SpriteIsDrawnWithItsPalette) {
  PpuBench bench;
  bench.vram.bytes[0x0020] = 0xFF;  // 图块 2 第 0 行的低位面
  bench.ppu.pokePalette(0, 0x0F);
  bench.ppu.pokePalette(16 + 1, 0x30);
  bench.ppu.pokeOam(0, 0x00);  // y 字节 0：顶端在第 1 行
  bench.ppu.pokeOam(1, 2);
  bench.ppu.pokeOam(2, 0x00);
  bench.ppu.pokeOam(3, 16);
  bench.ppu.write(0x2001, 0x16);
  bench.runLines(241);
  EXPECT_EQ(bench.pixel(16, 1), WHITE);
  EXPECT_EQ(bench.pixel(23, 1), WHITE);
  EXPECT_EQ(bench.pixel(24, 1), 0xFF545454) << "背景关闭，其余像素是颜色 $00";
}

TEST(PpuUm6576, SpriteZeroHitIsSetOnOverlapWithOpaqueBackground) {
  PpuBench bench;
  setUpFourColorTile(bench, 0x1E);
  bench.vram.bytes[0x0011] = 0xFF;  // 图块 1 的第 1 行也不透明
  bench.vram.bytes[0x0020] = 0xFF;
  bench.ppu.pokePalette(17, 0x16);
  bench.ppu.pokeOam(0, 0x00);
  bench.ppu.pokeOam(1, 2);
  bench.ppu.pokeOam(3, 0);
  bench.runLines(2);
  EXPECT_EQ(bench.ppu.status() & 0x40, 0x40);
  bench.runLines(311);
  EXPECT_EQ(bench.ppu.status() & 0x40, 0x00) << "预渲染行清除";
}

TEST(PpuUm6576, MoreThanEightSpritesOnALineSetTheOverflowFlag) {
  PpuBench bench;
  bench.ppu.write(0x2001, 0x10);
  for (int sprite = 0; sprite < 9; ++sprite) {
    bench.ppu.pokeOam(static_cast<std::size_t>(sprite) * 4 + 0, 0x00);
    bench.ppu.pokeOam(static_cast<std::size_t>(sprite) * 4 + 3, static_cast<std::uint8_t>(sprite * 8));
  }
  for (int sprite = 9; sprite < 64; ++sprite) bench.ppu.pokeOam(static_cast<std::size_t>(sprite) * 4 + 0, 0xF0);
  bench.runLines(2);
  EXPECT_EQ(bench.ppu.status() & 0x20, 0x20);
}

TEST(PpuUm6576, MonochromeBitStripsTheHue) {
  PpuBench bench;
  setUpFourColorTile(bench, 0x0B);
  bench.ppu.pokePalette(1, 0x21);  // 淡蓝色，黑白后变成色号 $20（白）
  bench.runLines(241);
  EXPECT_EQ(bench.pixel(0, 0), WHITE);
}

TEST(PpuUm6576, EmphasisDimsTheOtherChannels) {
  PpuBench bench;
  setUpFourColorTile(bench, 0x4A);  // 增强红
  bench.runLines(241);
  EXPECT_EQ(bench.pixel(0, 0), 0xFF000000U | (236U * 3 / 4) << 16 | (238U * 3 / 4) << 8 | 236U);
}

TEST(PpuUm6576, ResetStartsANewFrameSequence) {
  PpuBench bench;
  bench.runLines(100);
  bench.ppu.reset(false);
  EXPECT_EQ(bench.ppu.nextLine(), 0);
  bench.scheduler.advance(LINE - 1);
  EXPECT_EQ(bench.ppu.nextLine(), 1);
}

}  // namespace
