/*
 * ------------------------------------------------------------------------
 * 名称：pyramid-board-test.cpp
 * 说明：金字塔学习机主板的测试：PEC-9588 与 PEC-586 的程序窗口规则、启动映像的拼法、镜像、汉字模式、$5100 与 $5500 的磁带线、工作 RAM、字符 RAM 与名称表。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：测试用的 ROM 每个 8K 页的所有字节都等于页号（0 到 63），所以读到的值就是窗口当前指向的页号。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <cstdint>
#include <functional>
#include <vector>

#include "bootstrap/machines/pyramid/pyramid-board.hpp"
#include "contracts/hardware/hardware-error.hpp"
#include "shared/errors/core-error.hpp"

namespace {

using ember::bootstrap::PyramidBoard;
namespace hw = ember::contracts::hardware;

/** 记录写入、给定读回电平的磁带线。 */
class FakeTapeLine final : public hw::TapeLine {
 public:
  bool level() override { return level_; }
  void drive(bool high) override { driven.push_back(high); }
  void setComparator(const hw::TapeComparator&) override {}
  void setEdgeListener(std::function<void(bool, std::uint32_t)>) override {}

  bool level_{false};
  std::vector<bool> driven;
};

/** 每个 8K 页的字节都等于页号的 512K ROM。 */
std::vector<std::uint8_t> pagedRom() {
  std::vector<std::uint8_t> rom(PyramidBoard::ROM_SIZE);
  for (std::size_t index = 0; index < rom.size(); ++index) rom[index] = static_cast<std::uint8_t>(index / PyramidBoard::PAGE_SIZE);
  return rom;
}

struct Rig {
  explicit Rig(PyramidBoard::Model model) : board(model, pagedRom(), tape) {}
  std::uint8_t peek(std::uint16_t address) { return board.cpuSide().peek(address).value; }
  void write(std::uint16_t address, std::uint8_t value) { board.cpuSide().write(address, value); }
  FakeTapeLine tape;
  PyramidBoard board;
};

TEST(PyramidBoard, TheRomMustBe512Kilobytes) {
  FakeTapeLine tape;
  EXPECT_THROW(PyramidBoard(PyramidBoard::Model::Pec9588, std::vector<std::uint8_t>(1024), tape), ember::shared::CoreError);
}

TEST(PyramidBoard, Pec9588StartsWithBlockZeroAndTheLastBlock) {
  Rig rig(PyramidBoard::Model::Pec9588);
  EXPECT_EQ(rig.peek(0x8000), 0);
  EXPECT_EQ(rig.peek(0xA000), 1);
  EXPECT_EQ(rig.peek(0xC000), 62);
  EXPECT_EQ(rig.peek(0xE000), 63);
}

TEST(PyramidBoard, Pec9588SelectsTheLowWindowFromRegisterZero) {
  Rig rig(PyramidBoard::Model::Pec9588);
  rig.write(0x5000, 0x03);
  EXPECT_EQ(rig.peek(0x8000), 6);
  EXPECT_EQ(rig.peek(0xBFFF), 7);
  EXPECT_EQ(rig.peek(0xC000), 62);
  rig.write(0x5000, 0x23);
  EXPECT_EQ(rig.peek(0x8000), 38) << "块 $13：第二个 256K";
  EXPECT_EQ(rig.peek(0xC000), 62);
  rig.write(0x5000, 0x43);
  EXPECT_EQ(rig.peek(0x8000), 6);
  EXPECT_EQ(rig.peek(0xC000), 60) << "$C000 换成块 30";
  rig.write(0x5000, 0x63);
  EXPECT_EQ(rig.peek(0x8000), 38);
  EXPECT_EQ(rig.peek(0xC000), 60);
  rig.write(0x5000, 0x6C);
  EXPECT_EQ(rig.peek(0x8000), 56);
  EXPECT_EQ(rig.peek(0xC000), 56) << "reg0 不小于 $6C 时 $C000 是块 28";
}

TEST(PyramidBoard, Pec9588KeepsTheWindowsWhenBitFourIsSet) {
  Rig rig(PyramidBoard::Model::Pec9588);
  rig.write(0x5000, 0x43);
  rig.write(0x5000, 0x53);
  EXPECT_EQ(rig.peek(0x8000), 6);
  EXPECT_EQ(rig.peek(0xC000), 60);
}

TEST(PyramidBoard, AnyRegisterWriteRecomputesFromRegisterZero) {
  Rig rig(PyramidBoard::Model::Pec9588);
  rig.write(0x5000, 0x02);
  rig.write(0x4200, 0xFF);
  EXPECT_EQ(rig.board.registers()[2], 0xFF);
  EXPECT_EQ(rig.board.registers()[0], 0x02);
  EXPECT_EQ(rig.peek(0x8000), 4);
}

TEST(PyramidBoard, Pec586StartsWithTheBootImageMadeOfTheBlockTails) {
  Rig rig(PyramidBoard::Model::Pec586);
  const auto& image = rig.board.bootImage();
  for (std::size_t k = 0; k < 16; ++k) {
    EXPECT_EQ(image[k * 0x800], 32 + 2 * k) << "块 " << 16 + k << " 偏移 $1C00 的 1K";
    EXPECT_EQ(image[k * 0x800 + 0x3FF], 32 + 2 * k);
    EXPECT_EQ(image[k * 0x800 + 0x400], 33 + 2 * k) << "块 " << 16 + k << " 偏移 $3C00 的 1K";
    EXPECT_EQ(image[k * 0x800 + 0x7FF], 33 + 2 * k);
  }
  EXPECT_EQ(rig.peek(0x8000), image[0]);
  EXPECT_EQ(rig.peek(0xA030), 40) << "复位向量 $A030 落在启动映像里";
  EXPECT_EQ(rig.peek(0xFFFA), 63);
}

TEST(PyramidBoard, Pec586WholeBankModeMapsA32KilobyteRomBank) {
  Rig rig(PyramidBoard::Model::Pec586);
  rig.write(0x5000, 0x13);
  EXPECT_EQ(rig.peek(0x8000), 12);
  EXPECT_EQ(rig.peek(0xA000), 13);
  EXPECT_EQ(rig.peek(0xC000), 14);
  EXPECT_EQ(rig.peek(0xE000), 15);
}

TEST(PyramidBoard, Pec586SwapsOnlyTheFirstEightKilobytesAndKeepsTheBootImageAbove) {
  Rig rig(PyramidBoard::Model::Pec586);
  rig.write(0x5000, 0x45);
  EXPECT_EQ(rig.peek(0x8000), 37);
  EXPECT_EQ(rig.peek(0xA000), 40);
  EXPECT_EQ(rig.peek(0xFFFA), 63);
  rig.write(0x5000, 0x6A);
  EXPECT_EQ(rig.peek(0x8000), 58);
  EXPECT_EQ(rig.peek(0xA000), 40);
  rig.write(0x5000, 0x20);
  EXPECT_EQ(rig.peek(0x8000), rig.board.bootImage()[0]) << "其余的值回到整块启动映像";
  rig.write(0x5000, 0x00);
  EXPECT_EQ(rig.peek(0xA000), 40);
}

TEST(PyramidBoard, Pec586ResetGoesBackToTheBootImage) {
  Rig rig(PyramidBoard::Model::Pec586);
  rig.write(0x5000, 0x13);
  rig.board.reset(false);
  EXPECT_EQ(rig.board.registers()[0], 0);
  EXPECT_EQ(rig.peek(0x8000), rig.board.bootImage()[0]);
}

TEST(PyramidBoard, WritesToTheProgramWindowAreIgnored) {
  Rig rig(PyramidBoard::Model::Pec9588);
  rig.write(0x8000, 0x77);
  EXPECT_EQ(rig.peek(0x8000), 0);
  Rig boot(PyramidBoard::Model::Pec586);
  const std::uint8_t before = boot.peek(0x8000);
  boot.write(0x8000, 0x77);
  EXPECT_EQ(boot.peek(0x8000), before);
}

TEST(PyramidBoard, RegisterZeroBitsThreeAndFourBothSetMeanHorizontalMirroring) {
  Rig rig(PyramidBoard::Model::Pec9588);
  EXPECT_FALSE(rig.board.horizontalMirroring());
  rig.write(0x5000, 0x18);
  EXPECT_TRUE(rig.board.horizontalMirroring());
  rig.write(0x5000, 0x08);
  EXPECT_FALSE(rig.board.horizontalMirroring());
  rig.write(0x5000, 0x10);
  EXPECT_FALSE(rig.board.horizontalMirroring());
}

TEST(PyramidBoard, TheNametableFollowsTheMirroring) {
  Rig rig(PyramidBoard::Model::Pec9588);
  rig.board.videoSide().write(0x2000, 0xA1);
  EXPECT_EQ(rig.board.videoSide().peek(0x2800).value, 0xA1) << "垂直镜像：$2000 与 $2800 同页";
  EXPECT_NE(rig.board.videoSide().peek(0x2400).value, 0xA1);
  rig.write(0x5000, 0x18);
  rig.board.videoSide().write(0x2000, 0xB2);
  EXPECT_EQ(rig.board.videoSide().peek(0x2400).value, 0xB2) << "水平镜像：$2000 与 $2400 同页";
  EXPECT_EQ(rig.board.videoSide().peek(0x3000).value, 0xB2) << "$3000 起是名称表的镜像";
}

TEST(PyramidBoard, TextModeFollowsBitSevenAndRewritesBackgroundPatternReads) {
  Rig rig(PyramidBoard::Model::Pec9588);
  rig.board.videoSide().write(0x0123, 0xAA);
  rig.board.videoSide().write(0x012B, 0xBB);
  const auto fetch = [&] {
    static_cast<void>(rig.board.videoSide().read(0x2021));
    static_cast<void>(rig.board.videoSide().read(0x23C0));
    return rig.board.videoSide().read(0x0123).value;
  };
  EXPECT_EQ(fetch(), 0xAA);
  rig.write(0x5000, 0x80);
  EXPECT_TRUE(rig.board.textMode());
  EXPECT_EQ(fetch(), 0xBB) << "奇数列取每格的后 8 字节";
  rig.write(0x5000, 0x00);
  EXPECT_FALSE(rig.board.textMode());
  EXPECT_EQ(fetch(), 0xAA);
}

TEST(PyramidBoard, StatusRegisterReadsTenAndTheTapeLineInBitTwo) {
  Rig rig(PyramidBoard::Model::Pec9588);
  EXPECT_EQ(rig.board.cpuSide().read(0x5500).value, 0x10);
  rig.tape.level_ = true;
  EXPECT_EQ(rig.board.cpuSide().read(0x5500).value, 0x14);
  EXPECT_EQ(rig.board.cpuSide().peek(0x5500).value, 0x10) << "调试读不碰磁带机";
  EXPECT_EQ(rig.board.cpuSide().read(0x5500).drivenMask, 0xFF);
}

TEST(PyramidBoard, Pec586ReadsTheTapeInBitTwoOfRegister5300InsteadOf5500) {
  Rig rig(PyramidBoard::Model::Pec586);
  rig.tape.level_ = true;
  const hw::BusRead tapeRead = rig.board.cpuSide().read(0x5300);
  EXPECT_EQ(tapeRead.drivenMask, 0x04);
  EXPECT_EQ(tapeRead.value & 0x04, 0x04);
  rig.tape.level_ = false;
  EXPECT_EQ(rig.board.cpuSide().read(0x5300).value & 0x04, 0);
  rig.tape.level_ = true;
  EXPECT_EQ(rig.board.cpuSide().read(0x5500).value, 0x10) << "586 的 $5500 不带磁带信号";
  EXPECT_EQ(rig.board.cpuSide().peek(0x5300).value & 0x04, 0) << "调试读不碰磁带机";
  Rig other(PyramidBoard::Model::Pec9588);
  other.tape.level_ = true;
  EXPECT_EQ(other.board.cpuSide().read(0x5300).drivenMask, 0) << "9588 的 $5300 是开放总线";
}

TEST(PyramidBoard, OtherLowAddressesReadTheOpenBus) {
  Rig rig(PyramidBoard::Model::Pec9588);
  EXPECT_EQ(rig.board.cpuSide().read(0x5000).drivenMask, 0);
  EXPECT_EQ(rig.board.cpuSide().read(0x4020).drivenMask, 0);
}

TEST(PyramidBoard, WritingBitZeroOfRegister5100DrivesTheTape) {
  Rig rig(PyramidBoard::Model::Pec9588);
  rig.write(0x5100, 0x01);
  rig.write(0x5100, 0x02);
  rig.write(0x5000, 0x01);
  EXPECT_EQ(rig.tape.driven, (std::vector<bool>{true, false}));
}

TEST(PyramidBoard, Pec586DrivesTheTapeWithBitTwoOfRegister5100) {
  Rig rig(PyramidBoard::Model::Pec586);
  rig.write(0x5100, 0x04);
  rig.write(0x5100, 0x01);
  rig.write(0x5100, 0x03);
  rig.write(0x5100, 0x00);
  EXPECT_EQ(rig.tape.driven, (std::vector<bool>{true, false, false, false}));
}

TEST(PyramidBoard, WorkRamIsReadableAndWritableAtSixThousand) {
  Rig rig(PyramidBoard::Model::Pec9588);
  rig.write(0x6000, 0x5A);
  rig.write(0x7FFF, 0xA5);
  EXPECT_EQ(rig.peek(0x6000), 0x5A);
  EXPECT_EQ(rig.peek(0x7FFF), 0xA5);
  rig.board.reset(false);
  EXPECT_EQ(rig.peek(0x6000), 0x5A) << "软复位不清工作 RAM";
  rig.board.reset(true);
  EXPECT_EQ(rig.peek(0x6000), 0) << "上电清零";
}

}  // namespace
