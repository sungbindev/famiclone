/*
 * ------------------------------------------------------------------------
 * 名称：yuxing-text-test.cpp
 * 说明：裕兴学习机汉字显示电路的测试：开关、背景图案读取的认定（属性字节之后的两次图案表读取）、地址位 12 与位 3 的改写、精灵的读取与关闭时不改写、最近一次名称表字节的地址。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：按 PPU 的取指顺序喂地址：名称表字节、属性字节、图案低位平面、图案高位平面；精灵的取指是两次名称表字节再接两次图案表读取。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <cstdint>
#include <utility>

#include "logic/hardware/yuxing-text/yuxing-text.hpp"

namespace {

using ember::logic::hardware::YuxingText;

/** 模拟 PPU 取一个背景图块：给出名称表字节的地址与图案地址，返回两次图案表读取实际读的地址。 */
std::pair<std::uint16_t, std::uint16_t> fetchTile(YuxingText& circuit, std::uint16_t nametableAddress, std::uint16_t patternAddress) {
  circuit.observe(nametableAddress);
  circuit.observe(static_cast<std::uint16_t>((nametableAddress & 0x2C00) | 0x03C0));
  circuit.observe(patternAddress);
  const std::uint16_t low = circuit.remap(patternAddress);
  circuit.observe(static_cast<std::uint16_t>(patternAddress | 8));
  const std::uint16_t high = circuit.remap(static_cast<std::uint16_t>(patternAddress | 8));
  return {low, high};
}

TEST(YuxingText, DoesNotRewriteAnythingUntilTheTextModeIsTurnedOn) {
  YuxingText circuit;
  circuit.reset();
  EXPECT_EQ(fetchTile(circuit, 0x2021, 0x0123), (std::pair<std::uint16_t, std::uint16_t>{0x0123, 0x012B}));
  circuit.setTextMode(true);
  EXPECT_NE(fetchTile(circuit, 0x2021, 0x0123), (std::pair<std::uint16_t, std::uint16_t>{0x0123, 0x012B}));
  circuit.setTextMode(false);
  EXPECT_EQ(fetchTile(circuit, 0x2021, 0x0123), (std::pair<std::uint16_t, std::uint16_t>{0x0123, 0x012B}));
}

TEST(YuxingText, BothPlanesReadTheSameByteAndTheColumnParityPicksTheHalf) {
  YuxingText circuit;
  circuit.reset();
  circuit.setTextMode(true);
  // 偶数列取前 8 字节（位 3 为 0）。
  EXPECT_EQ(fetchTile(circuit, 0x2020, 0x0123), (std::pair<std::uint16_t, std::uint16_t>{0x0123, 0x0123}));
  // 奇数列取后 8 字节（位 3 为 1）。
  EXPECT_EQ(fetchTile(circuit, 0x2021, 0x0123), (std::pair<std::uint16_t, std::uint16_t>{0x012B, 0x012B}));
  EXPECT_EQ(fetchTile(circuit, 0x2021, 0x0120), (std::pair<std::uint16_t, std::uint16_t>{0x0128, 0x0128}));
}

TEST(YuxingText, TheLowerHalfOfTheScreenUsesPatternTableOne) {
  YuxingText circuit;
  circuit.reset();
  circuit.setTextMode(true);
  // 第 0 至 15 行（名称表地址位 9 为 0）用图案表 0，不论 PPU 给的图案地址在哪个表。
  EXPECT_EQ(fetchTile(circuit, 0x21E0, 0x1123).first, 0x0123);
  EXPECT_EQ(fetchTile(circuit, 0x2000, 0x0123).first, 0x0123);
  // 第 16 行起（位 9 为 1）用图案表 1。
  EXPECT_EQ(fetchTile(circuit, 0x2200, 0x0123).first, 0x1123);
  EXPECT_EQ(fetchTile(circuit, 0x23A1, 0x0123).first, 0x112B);
  // 另一个名称表里同样按位 9 判断。
  EXPECT_EQ(fetchTile(circuit, 0x2A00, 0x0044).first, 0x1044);
}

TEST(YuxingText, OnlyThePatternReadsRightAfterAnAttributeByteBelongToTheBackground) {
  YuxingText circuit;
  circuit.reset();
  circuit.setTextMode(true);
  // 精灵的取指：两次名称表字节（不是属性），再读两次图案表，不改写。
  circuit.observe(0x2221);
  circuit.observe(0x2221);
  circuit.observe(0x0130);
  EXPECT_EQ(circuit.remap(0x0130), 0x0130);
  circuit.observe(0x0138);
  EXPECT_EQ(circuit.remap(0x0138), 0x0138);
  // 背景图块的两次读取用完之后，再来的图案表读取不是背景的。
  static_cast<void>(fetchTile(circuit, 0x2021, 0x0123));
  circuit.observe(0x0140);
  EXPECT_EQ(circuit.remap(0x0140), 0x0140);
}

TEST(YuxingText, NametableAndPaletteAddressesAreNeverRewritten) {
  YuxingText circuit;
  circuit.reset();
  circuit.setTextMode(true);
  circuit.observe(0x2021);
  circuit.observe(0x23C0);
  EXPECT_EQ(circuit.remap(0x2021), 0x2021);
  EXPECT_EQ(circuit.remap(0x3F00), 0x3F00);
}

TEST(YuxingText, TheTileAddressIsTheLatestNametableByteAndNotTheAttributeByte) {
  YuxingText circuit;
  circuit.reset();
  circuit.observe(0x2345);
  EXPECT_EQ(circuit.tileAddress(), 0x2345);
  circuit.observe(0x23C5);
  EXPECT_EQ(circuit.tileAddress(), 0x2345) << "属性字节不改变图块位置";
}

TEST(YuxingText, ResetTurnsTheTextModeOff) {
  YuxingText circuit;
  circuit.setTextMode(true);
  circuit.reset();
  EXPECT_EQ(fetchTile(circuit, 0x2201, 0x0123), (std::pair<std::uint16_t, std::uint16_t>{0x0123, 0x012B}));
  EXPECT_FALSE(circuit.textMode());
}

}  // namespace
