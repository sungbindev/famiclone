/*
 * ------------------------------------------------------------------------
 * 名称：plq-test.cpp
 * 说明：验证 plq 插件（mapper 172、PRG 16K 的声像教学卡）：用合成的卡带检查 PAL 逻辑寄存器、RAM 与 ROM 的读写；用真实的 plq.nes 与真实的声像磁带检查磁带驱动画面。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-16
 * 备注：真实 ROM 在 reference/rom，磁带在 reference/tape（m4a，由 FFmpeg 库解码）；缺任何一个时对应用例跳过；设置环境变量 EMBER_REAL_ROM_DUMP 为目录时，把关键时刻的画面存成 PPM 供人工查看。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "../support/real-rom-support.hpp"
#include "contracts/hardware/tape-deck.hpp"
#include "infra/media/disk-tape-store.hpp"

namespace {

namespace hw = ember::contracts::hardware;
using ember::test::firstTape;
using ember::test::readReferenceFile;
using ember::test::RealRomBench;

/** 合成的 PRG 16K 卡带（mapper 172）：复位向量指向 $E000，那里是一条死循环，不会碰卡上的寄存器。 */
std::vector<std::uint8_t> idleCard() {
  std::vector<std::uint8_t> file = {0x4E, 0x45, 0x53, 0x1A, 0x01, 0x00, 0xC0, 0xA0, 0, 0, 0, 0, 0, 0, 0, 0};
  std::vector<std::uint8_t> prg(16384, 0xEA);
  prg[0x2000] = 0x4C;  // JMP $E000
  prg[0x2001] = 0x00;
  prg[0x2002] = 0xE0;
  prg[0x3FFC] = 0x00;  // 复位向量 $E000
  prg[0x3FFD] = 0xE0;
  file.insert(file.end(), prg.begin(), prg.end());
  return file;
}

TEST(PlqCard, TheLogicRegisterLatchesWritesAndTheMemoryIsMappedLikeTheCard) {
  RealRomBench bench({"plq"});
  bench.boot(idleCard());
  ASSERT_TRUE(bench.run(2));
  std::uint8_t initial = 0;
  std::uint8_t afterFirst = 0;
  std::uint8_t afterSecond = 0;
  std::uint8_t afterTape = 0;
  std::uint8_t latchRead = 0;
  std::uint8_t ram = 0;
  std::uint8_t rom = 0;
  std::uint8_t romAfterWrite = 0;
  bench.runtime->useMachine([&](hw::Machine& machine) {
    auto& bus = machine.cartridgeSlot().cpuSide();
    initial = bus.read(0xA000).value;
    bus.write(0x8000, 0x0A);  // 地址位 1、2 都是 0：低 4 位直接锁存，其余位保持
    afterFirst = bus.read(0xA000).value;
    bus.write(0x8000, 0x05);
    afterSecond = bus.read(0xA000).value;
    bus.write(0x8006, 0x00);  // 地址位 1、2 都是 1、数据位 0 为 0：位 7 取磁带电平，磁带没有播放时是 0
    afterTape = bus.read(0xA000).value;
    latchRead = bus.read(0x8000).value;
    bus.write(0xC123, 0x5A);
    ram = bus.read(0xC123).value;
    rom = bus.read(0xE000).value;
    bus.write(0xE000, 0x00);
    romAfterWrite = bus.read(0xE000).value;
  });
  EXPECT_EQ(initial, 0xFF);
  EXPECT_EQ(afterFirst, 0xFA);
  EXPECT_EQ(afterSecond, 0xF5);
  EXPECT_EQ(afterTape, 0x00);
  EXPECT_EQ(latchRead, 0x00);
  EXPECT_EQ(ram, 0x5A);
  EXPECT_EQ(rom, 0x4C);
  EXPECT_EQ(romAfterWrite, 0x4C) << "$E000 起是 ROM，写入应被丢弃";
  EXPECT_TRUE(bench.errors->entries.empty());
}

TEST(PlqCard, BootsAndTheTapeDrivesTheScreen) {
  const auto rom = readReferenceFile("rom/plq.nes");
  const auto tape = firstTape();
  if (rom.empty() || tape.empty()) GTEST_SKIP() << "没有找到 plq.nes 或声像磁带";
  RealRomBench bench({"plq"});
  bench.boot(rom);
  const bool ran = bench.run(300);
  ASSERT_TRUE(ran);
  bench.dump("plq-0-boot");
  EXPECT_TRUE(bench.errors->entries.empty());

  ember::infra::media::DiskTapeStore store(nullptr);
  const auto tapePath = tape.u8string();
  auto source = store.open(std::string(tapePath.begin(), tapePath.end()));
  EXPECT_TRUE(source->stereo());
  bench.runtime->useMachine([&](hw::Machine& machine) {
    machine.tape()->load(std::move(source), "tape");
    machine.tape()->play();
  });
  const auto title = bench.runtime->captureFrame();
  for (int second = 1; second <= 8; ++second) {
    ASSERT_TRUE(bench.run(300));
    bench.dump("plq-play-" + std::to_string(second * 5));
  }
  EXPECT_FALSE(ember::test::sameFrame(title, bench.runtime->captureFrame())) << "磁带放出的数据线驱动了卡带，画面应该从标题换成课文";
  EXPECT_TRUE(bench.errors->entries.empty());
}

}  // namespace
