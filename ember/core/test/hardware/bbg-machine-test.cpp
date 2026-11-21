/*
 * ------------------------------------------------------------------------
 * 名称：bbg-machine-test.cpp
 * 说明：主机型号 bbg（步步高软驱一号与步步高 98）的测试：容量与存储译码（用合成的 BIOS 检查）、程序页与 DRAM 页寄存器、系统工作区、PPU 寄存器的影子写、字符页与名称表连接、Dendy 制式的 vblank、扫描线 IRQ 与分屏、MMC3 模式、语音与并口状态、输入；以及用真实 BIOS 开机的冒烟测试与在 DOS 里格式化软盘的测试（每个约半分钟）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-16
 * 备注：真实 BIOS 在仓库的 ember/roms/bbg1_bios.bin（128KB）与 bbg98_bios.bin（2048KB），都是没有文件头的纯 BIOS，文件不存在时跳过冒烟测试；
 *       设置环境变量 EMBER_BBG_DUMP 为一个文件路径时，冒烟测试把开机后的画面写成以它为前缀的 PPM 文件，供人工查看。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "bootstrap/machines/bbg/bbg.hpp"
#include "contracts/hardware/hardware-error.hpp"
#include "shared/errors/core-error.hpp"

namespace {

namespace hw = ember::contracts::hardware;
using ember::bootstrap::BbgMachine;
using ember::bootstrap::BbgVariant;

constexpr std::size_t PAGE = 0x2000;

/** 合成的 BIOS：每个 8K 页的第一个字节是页号，向量表在最后一页，RESET 指向 $C000、IRQ 指向 $C080，$C000 处是 CLI 后在 $00 上无限计数的程序，$C080 处是计数加 1 并确认中断的处理程序。 */
std::vector<std::uint8_t> syntheticBios(const BbgVariant& variant) {
  std::vector<std::uint8_t> bios(variant.romSize, 0xEA);
  const std::size_t pages = variant.romSize / PAGE;
  for (std::size_t page = 0; page < pages; ++page) bios[page * PAGE] = static_cast<std::uint8_t>(page);
  const std::size_t secondLast = (pages - 2) * PAGE;
  const std::size_t last = (pages - 1) * PAGE;
  // 向量表：NMI 与 RESET 指向 $C000，IRQ 指向 $C080。
  bios[last + 0x1FFA] = 0x00;
  bios[last + 0x1FFB] = 0xC0;
  bios[last + 0x1FFC] = 0x00;
  bios[last + 0x1FFD] = 0xC0;
  bios[last + 0x1FFE] = 0x80;
  bios[last + 0x1FFF] = 0xC0;
  const std::uint8_t program[] = {
      0x58,              // CLI
      0xE6, 0x00,        // loop: INC $00
      0x4C, 0x01, 0xC0   // JMP loop（$C001）
  };
  std::copy(std::begin(program), std::end(program), bios.begin() + static_cast<std::ptrdiff_t>(secondLast));
  const std::uint8_t handler[] = {
      0xE6, 0x10,        // INC $10
      0xAD, 0xB0, 0xFF,  // LDA $FFB0（确认中断）
      0x40               // RTI
  };
  std::copy(std::begin(handler), std::end(handler), bios.begin() + static_cast<std::ptrdiff_t>(secondLast) + 0x80);
  return bios;
}

/** 读真实的 BIOS：文件不存在或大小不对时返回空。 */
std::vector<std::uint8_t> realBios(const char* name, const BbgVariant& variant) {
  const std::filesystem::path path = std::filesystem::path(EMBER_TEST_DATA_DIR) / ".." / ".." / ".." / "roms" / name;
  std::ifstream stream(path, std::ios::binary);
  if (!stream) return {};
  std::vector<std::uint8_t> file((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
  if (file.size() != variant.romSize) return {};
  return file;
}

/** 读仓库 reference/floppy-images/bbg 里的盘片镜像；文件不存在时返回空。 */
std::vector<std::uint8_t> realFloppy(const std::filesystem::path& relative) {
  const std::filesystem::path path = std::filesystem::path(EMBER_TEST_DATA_DIR) / ".." / ".." / ".." / ".." / relative;
  std::ifstream stream(path, std::ios::binary);
  if (!stream) return {};
  return std::vector<std::uint8_t>((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
}

/** 把画面写成 PPM 文件。 */
void dumpFrame(const hw::FrameBuffer& picture, const std::string& path) {
  std::ofstream out(path, std::ios::binary);
  out << "P6\n256 240\n255\n";
  for (const std::uint32_t pixel : picture.pixels) {
    const char rgb[3] = {static_cast<char>(pixel & 0xFF), static_cast<char>((pixel >> 8) & 0xFF), static_cast<char>((pixel >> 16) & 0xFF)};
    out.write(rgb, 3);
  }
}

/** 环境变量 EMBER_BBG_DUMP 的值；没有设置时为空。 */
std::string dumpPath() {
#ifdef _MSC_VER
  char* value = nullptr;
  std::size_t length = 0;
  if (_dupenv_s(&value, &length, "EMBER_BBG_DUMP") != 0 || value == nullptr) return {};
  std::string path(value);
  std::free(value);
  return path;
#else
  const char* value = std::getenv("EMBER_BBG_DUMP");
  return value == nullptr ? std::string() : std::string(value);
#endif
}

std::uint8_t peek(BbgMachine& machine, std::uint16_t address) { return machine.peekCpuBus(address); }

void poke(BbgMachine& machine, std::uint16_t address, std::uint8_t value) {
  const std::uint8_t bytes[] = {value};
  machine.debugger()->writeMemory("cpu", address, bytes);
}

void pokeVideo(BbgMachine& machine, std::uint16_t address, std::uint8_t value) {
  const std::uint8_t bytes[] = {value};
  machine.debugger()->writeMemory("ppu", address, bytes);
}

std::uint8_t peekVideo(BbgMachine& machine, std::uint16_t address) {
  std::uint8_t value = 0;
  machine.debugger()->readMemory("ppu", address, std::span<std::uint8_t>(&value, 1));
  return value;
}

/** 单步到 PPU 走到指定行（之后再走 extra 条指令）。 */
void stepToLine(BbgMachine& machine, int line) {
  for (int guard = 0; guard < 400000 && machine.ppu().scanline() != line; ++guard) machine.debugger()->step(hw::StepUnit::Instruction, 1);
}

TEST(BbgMachine, RejectsABiosOfTheWrongSize) {
  try {
    BbgMachine machine(BbgVariant::floppyOne(), std::vector<std::uint8_t>(1024, 0));
    FAIL() << "应该抛出异常";
  } catch (const ember::shared::CoreError& error) {
    EXPECT_EQ(error.code(), hw::ERROR_INVALID_ARGUMENT);
  }
  try {
    BbgMachine machine(BbgVariant::model98(), std::vector<std::uint8_t>(BbgVariant::floppyOne().romSize, 0));
    FAIL() << "98 不能用软驱一号大小的 BIOS";
  } catch (const ember::shared::CoreError& error) {
    EXPECT_EQ(error.code(), hw::ERROR_INVALID_ARGUMENT);
  }
}

TEST(BbgMachine, TheTwoVariantsDifferInRomDramAndCharacterPages) {
  const BbgVariant& one = BbgVariant::floppyOne();
  const BbgVariant& ninetyEight = BbgVariant::model98();
  EXPECT_EQ(one.romSize, 128U * 1024);
  EXPECT_EQ(one.dramSize, 512U * 1024);
  EXPECT_EQ(one.characterPageMask, 0x1FU);
  EXPECT_EQ(ninetyEight.romSize, 2048U * 1024);
  EXPECT_EQ(ninetyEight.dramSize, 2048U * 1024);
  EXPECT_EQ(ninetyEight.characterPageMask, 0xFFU);
  BbgMachine machine(ninetyEight, syntheticBios(ninetyEight));
  std::size_t romSize = 0;
  for (const hw::MemorySpace& space : machine.debugger()->memorySpaces()) {
    if (space.id == "board.rom") romSize = space.size;
  }
  EXPECT_EQ(romSize, ninetyEight.romSize);
}

TEST(BbgMachine, AfterResetTheLastSixteenKilobytesOfRomAreAtTheTop) {
  BbgMachine machine(BbgVariant::floppyOne(), syntheticBios(BbgVariant::floppyOne()));
  EXPECT_EQ(peek(machine, 0x8000), 0);
  EXPECT_EQ(peek(machine, 0xA000), 0);
  EXPECT_EQ(peek(machine, 0xC000), 0x58) << "倒数第 2 页的第一个字节被程序占用（CLI）";
  EXPECT_EQ(peek(machine, 0xE000), 15) << "最后一页";
  EXPECT_EQ(peek(machine, 0xFFFC), 0x00);
  EXPECT_EQ(peek(machine, 0xFFFD), 0xC0);
}

TEST(BbgMachine, ProgramPageRegistersSelectRomOrDramPages) {
  BbgMachine machine(BbgVariant::floppyOne(), syntheticBios(BbgVariant::floppyOne()));
  poke(machine, 0xFF04, 0x02);
  EXPECT_EQ(peek(machine, 0x8000), 4) << "16K 页 2 是 8K 页 4 与 5";
  EXPECT_EQ(peek(machine, 0xA000), 5);
  poke(machine, 0xFF14, 0x07);
  poke(machine, 0x8000, 0x5A);
  EXPECT_EQ(machine.board().dram()[7 * PAGE], 0x5A) << "写 $FF14 同时选 DRAM";
  EXPECT_EQ(peek(machine, 0x8000), 0x5A);
  poke(machine, 0xFF04, 0x00);
  EXPECT_EQ(peek(machine, 0x8000), 0) << "写 $FF04 位 7 为 0 回到 ROM";
  poke(machine, 0xFF04, 0x81);
  poke(machine, 0xA000, 0x66);
  EXPECT_EQ(machine.board().dram()[3 * PAGE], 0x66) << "位 7 为 1 选 DRAM，16K 页 1 的后半是 8K 页 3";
}

TEST(BbgMachine, DramPageNumbersWrapAtTheDramSize) {
  BbgMachine machine(BbgVariant::floppyOne(), syntheticBios(BbgVariant::floppyOne()));
  poke(machine, 0xFF14, 0x41);
  poke(machine, 0x8000, 0x33);
  EXPECT_EQ(machine.board().dram()[1 * PAGE], 0x33) << "512K 的 DRAM 只有 64 个 8K 页，页号 $41 与 1 是同一页";
}

TEST(BbgMachine, SystemWorkAreaIsTheTopSixteenKilobytesOfDramAtFourThousand) {
  BbgMachine machine(BbgVariant::floppyOne(), syntheticBios(BbgVariant::floppyOne()));
  poke(machine, 0x5800, 0x11);
  poke(machine, 0x7FFF, 0x22);
  EXPECT_EQ(machine.board().dram()[machine.board().systemBase() + 0x1800], 0x11);
  EXPECT_EQ(machine.board().dram()[machine.board().systemBase() + 0x3FFF], 0x22);
  EXPECT_EQ(machine.board().systemBase(), 512U * 1024 - 32 * 1024);
  poke(machine, 0x4300, 0x99);
  EXPECT_EQ(peek(machine, 0x4300), 0) << "$4020-$43FF 没有器件";
  BbgMachine machine98(BbgVariant::model98(), syntheticBios(BbgVariant::model98()));
  EXPECT_EQ(machine98.board().systemBase(), 2048U * 1024 - 32 * 1024);
}

TEST(BbgMachine, UpperHalfBecomesDramWhenMapRamOrForceRamIsSetButRegistersStayWritable) {
  BbgMachine machine(BbgVariant::floppyOne(), syntheticBios(BbgVariant::floppyOne()));
  poke(machine, 0xFF24, 0x02);
  poke(machine, 0xFF2C, 0x03);
  poke(machine, 0xFF01, 0x08);
  poke(machine, 0xC000, 0x31);
  poke(machine, 0xE123, 0x32);
  EXPECT_EQ(machine.board().dram()[2 * PAGE], 0x31);
  EXPECT_EQ(machine.board().dram()[3 * PAGE + 0x123], 0x32);
  poke(machine, 0xFF24, 0x05);
  EXPECT_EQ(machine.board().registers().cdBank, 0x05) << "$FF00-$FFFF 的写入总是进寄存器";
  EXPECT_EQ(machine.board().dram()[3 * PAGE + 0x1F24], 0) << "没有同时写进 DRAM";
  poke(machine, 0xFF01, 0x00);
  EXPECT_EQ(peek(machine, 0xC000), 0x58);
  poke(machine, 0xFF11, 0x80);
  EXPECT_NE(peek(machine, 0xC000), 0x58) << "$FF11 位 7 强制使用 DRAM";
  poke(machine, 0xFF19, 0x00);
  EXPECT_EQ(peek(machine, 0xC000), 0x58) << "写 $FF19 取消强制";
}

TEST(BbgMachine, WritingPpuRegistersAlsoShadowsThemIntoTheSystemWorkAreaWhenDramIsMapped) {
  BbgMachine machine(BbgVariant::floppyOne(), syntheticBios(BbgVariant::floppyOne()));
  poke(machine, 0x2000, 0x90);
  EXPECT_EQ(machine.board().dram()[machine.board().systemBase() + 0x2000], 0) << "DRAM 没有映射到 $C000 以上时不做影子写";
  poke(machine, 0xFF01, 0x08);
  poke(machine, 0x2000, 0x91);
  poke(machine, 0x2001, 0x1E);
  poke(machine, 0x2005, 0x07);
  poke(machine, 0x2006, 0x3F);
  EXPECT_EQ(machine.board().dram()[machine.board().systemBase() + 0x2000], 0x91);
  EXPECT_EQ(machine.board().dram()[machine.board().systemBase() + 0x2001], 0x1E);
  EXPECT_EQ(machine.board().dram()[machine.board().systemBase() + 0x2005], 0x07);
  EXPECT_EQ(machine.board().dram()[machine.board().systemBase() + 0x2006], 0) << "只有 $2000、$2001、$2005";
  EXPECT_EQ(peek(machine, 0x6000), 0x91) << "固件从 $6000 读回";
}

TEST(BbgMachine, CharacterPageRegistersSelectOneKilobytePagesOfCharacterRam) {
  BbgMachine machine(BbgVariant::floppyOne(), syntheticBios(BbgVariant::floppyOne()));
  poke(machine, 0xFF03, 0x03);
  pokeVideo(machine, 0x0000, 0xA1);
  pokeVideo(machine, 0x0400, 0xA2);
  EXPECT_EQ(machine.board().characterRam()[6 * 1024], 0xA1) << "2K 页 3 是 1K 页 6 与 7";
  EXPECT_EQ(machine.board().characterRam()[7 * 1024], 0xA2);
  poke(machine, 0xFF2B, 0x09);
  pokeVideo(machine, 0x0400, 0xB3);
  EXPECT_EQ(machine.board().characterRam()[9 * 1024], 0xB3) << "$FF23 起每 8 个地址是一个 1K 页寄存器";
  poke(machine, 0xFF33, 0x21);
  pokeVideo(machine, 0x0800, 0xC4);
  EXPECT_EQ(machine.board().characterRam()[1 * 1024], 0xC4) << "软驱一号的字符页号只取低 5 位";
  EXPECT_EQ(peekVideo(machine, 0x0800), 0xC4);
}

TEST(BbgMachine, The98CharacterPagesUseAllEightBits) {
  BbgMachine machine(BbgVariant::model98(), syntheticBios(BbgVariant::model98()));
  poke(machine, 0xFF33, 0xF1);
  pokeVideo(machine, 0x0800, 0xD5);
  EXPECT_EQ(machine.board().characterRam()[0xF1 * 1024], 0xD5);
}

TEST(BbgMachine, VideoControlSelectsTheNametableConnection) {
  BbgMachine machine(BbgVariant::floppyOne(), syntheticBios(BbgVariant::floppyOne()));
  poke(machine, 0xFF01, 0x00);
  pokeVideo(machine, 0x2000, 0x01);
  pokeVideo(machine, 0x2400, 0x02);
  EXPECT_EQ(peekVideo(machine, 0x2800), 0x01) << "垂直连接：$2000 与 $2800 相同";
  EXPECT_EQ(peekVideo(machine, 0x2C00), 0x02);
  poke(machine, 0xFF01, 0x01);
  pokeVideo(machine, 0x2000, 0x03);
  pokeVideo(machine, 0x2800, 0x04);
  EXPECT_EQ(peekVideo(machine, 0x2400), 0x03) << "水平连接：$2000 与 $2400 相同";
  EXPECT_EQ(peekVideo(machine, 0x2C00), 0x04);
  poke(machine, 0xFF01, 0x02);
  pokeVideo(machine, 0x2000, 0x05);
  EXPECT_EQ(peekVideo(machine, 0x2C00), 0x05) << "全部连到第 1 页";
  poke(machine, 0xFF01, 0x03);
  pokeVideo(machine, 0x2000, 0x06);
  EXPECT_EQ(peekVideo(machine, 0x2400), 0x06) << "全部连到第 2 页";
  EXPECT_EQ(machine.board().nametable()[1024], 0x06);
  EXPECT_EQ(peekVideo(machine, 0x3000), 0x06) << "$3000 起镜像 $2000 起";
}

TEST(BbgMachine, VblankStartsAtLine291LikeADendyConsole) {
  BbgMachine machine(BbgVariant::floppyOne(), syntheticBios(BbgVariant::floppyOne()));
  machine.powerOn();
  stepToLine(machine, 250);
  ASSERT_EQ(machine.ppu().scanline(), 250);
  EXPECT_FALSE(machine.ppu().vblankFlag()) << "第 241 行之后还没有进入垂直消隐";
  stepToLine(machine, 292);
  ASSERT_EQ(machine.ppu().scanline(), 292);
  EXPECT_TRUE(machine.ppu().vblankFlag());
  stepToLine(machine, 311);
  stepToLine(machine, 5);
  EXPECT_FALSE(machine.ppu().vblankFlag());
}

TEST(BbgMachine, TheFrameRateIsFiftyHertz) {
  BbgMachine machine(BbgVariant::floppyOne(), syntheticBios(BbgVariant::floppyOne()));
  EXPECT_NEAR(machine.framesPerSecond(), 50.007, 0.001);
}

TEST(BbgMachine, TheScanlineCounterRaisesAnIrqWhenItWrapsAroundOnAVisibleLine) {
  BbgMachine machine(BbgVariant::floppyOne(), syntheticBios(BbgVariant::floppyOne()));
  machine.powerOn();
  poke(machine, 0x4017, 0x40);
  poke(machine, 0xFF02, 0x00);
  poke(machine, 0xFF06, 0x0F);
  poke(machine, 0x2001, 0x08);
  poke(machine, 0xFF01, 0x04);
  for (int guard = 0; guard < 400000 && peek(machine, 0x10) == 0; ++guard) machine.debugger()->step(hw::StepUnit::Instruction, 1);
  ASSERT_EQ(peek(machine, 0x10), 1) << "中断处理程序执行了";
  EXPECT_GE(machine.ppu().scanline(), 15);
  EXPECT_LE(machine.ppu().scanline(), 16) << "计数器从 $F0 开始，第 16 行回到 0";
  EXPECT_EQ(machine.board().registers().scanlineCounter, 0);
}

TEST(BbgMachine, NoIrqIsRaisedWhileTheDisplayIsOff) {
  BbgMachine machine(BbgVariant::floppyOne(), syntheticBios(BbgVariant::floppyOne()));
  machine.powerOn();
  poke(machine, 0x4017, 0x40);
  poke(machine, 0xFF06, 0x0F);
  poke(machine, 0xFF01, 0x04);
  for (int frame = 0; frame < 3; ++frame) machine.runFrame();
  EXPECT_EQ(peek(machine, 0x10), 0);
}

TEST(BbgMachine, SplitModeSwitchesTheCharacterPagesAtTheQueuedLines) {
  BbgMachine machine(BbgVariant::floppyOne(), syntheticBios(BbgVariant::floppyOne()));
  machine.powerOn();
  poke(machine, 0xFF01, 0x40);
  poke(machine, 0xFF12, 0x00);
  poke(machine, 0xFF0A, 10);
  poke(machine, 0xFF1A, 0x21);
  poke(machine, 0xFF22, 0x00);
  stepToLine(machine, 260);
  bool pattern = false;
  EXPECT_EQ(machine.board().locateVideo(0x0000, pattern), 2U * 1024) << "低半字节选 2K 页 1 放在 $0000，即 1K 页 2 与 3";
  EXPECT_TRUE(pattern);
  EXPECT_EQ(machine.board().locateVideo(0x0400, pattern), 3U * 1024);
  EXPECT_EQ(machine.board().locateVideo(0x0800, pattern), 4U * 1024) << "高半字节选 2K 页 2 放在 $0800";
  EXPECT_EQ(machine.board().registers().scanlineCounter, 10) << "取到第一项后计数器装入这一项的行数";
}

TEST(BbgMachine, Mmc3ModeMapsTheChipsBanksOntoTheDramAndCharacterRam) {
  BbgMachine machine(BbgVariant::floppyOne(), syntheticBios(BbgVariant::floppyOne()));
  poke(machine, 0xFF3C, 0x00);
  poke(machine, 0xFF01, 0x20);
  ASSERT_TRUE(machine.board().registers().mmc3Mode);
  poke(machine, 0x8000, 0x06);
  poke(machine, 0x8001, 0x05);
  poke(machine, 0x8000, 0x07);
  poke(machine, 0x8001, 0x09);
  EXPECT_EQ(machine.board().locate(0x8000).offset, 5 * PAGE);
  EXPECT_EQ(machine.board().locate(0xA000).offset, 9 * PAGE);
  EXPECT_EQ(machine.board().locate(0xC000).offset, 0x3E * PAGE) << "固定的倒数第 2 页落在 DRAM 的倒数第 2 页";
  EXPECT_EQ(machine.board().locate(0xE000).offset, 0x3F * PAGE);
  poke(machine, 0x8000, 0x06);
  poke(machine, 0x8001, 0x3C);
  EXPECT_EQ(machine.board().locate(0x8000).offset, 0U) << "游戏的倒数第 4 页与系统工作区重合，改放到 DRAM 的最前面";
  poke(machine, 0x8001, 0x3D);
  EXPECT_EQ(machine.board().locate(0x8000).offset, PAGE);
  poke(machine, 0x8000, 0x02);
  poke(machine, 0x8001, 0x07);
  bool pattern = false;
  EXPECT_EQ(machine.board().locateVideo(0x1000, pattern), 7U * 1024);
  EXPECT_TRUE(pattern);
  poke(machine, 0x4100, 0x77);
  EXPECT_EQ(peek(machine, 0x4100), 0x77) << "MMC3 模式下 $4020 起也是 DRAM";
}

TEST(BbgMachine, Mmc3ModeGameSizeRegisterPlacesTheGameAtTheTopOfDram) {
  BbgMachine machine(BbgVariant::floppyOne(), syntheticBios(BbgVariant::floppyOne()));
  poke(machine, 0xFF3C, 0x30);
  poke(machine, 0xFF01, 0x20);
  poke(machine, 0x8000, 0x06);
  poke(machine, 0x8001, 0x03);
  EXPECT_EQ(machine.board().locate(0x8000).offset, (3U + 0x30U) * PAGE) << "128K 的游戏装在 DRAM 的最后 128K";
  EXPECT_EQ(machine.board().locate(0xE000).offset, 0x3FU * PAGE);
}

TEST(BbgMachine, SpeechChipAcceptsTheBbgCommandByteAndReportsStatusLikeTheFirmwareExpects) {
  BbgMachine machine(BbgVariant::floppyOne(), syntheticBios(BbgVariant::floppyOne()));
  machine.powerOn();
  EXPECT_EQ(peek(machine, 0xFF18), 0x8F);
  poke(machine, 0xFF10, 0x01);
  poke(machine, 0xFF18, 0x0A);
  for (unsigned index = 0; index < 4; ++index) poke(machine, 0xFF18, 0x00);
  EXPECT_FALSE(machine.speech().playing()) << "$0A 不是步步高的命令字节";
  poke(machine, 0xFF10, 0x00);
  poke(machine, 0xFF10, 0x01);
  poke(machine, 0xFF18, 0xD6);
  for (unsigned index = 0; index < 8; ++index) poke(machine, 0xFF18, 0x00);
  EXPECT_TRUE(machine.speech().playing());
}

TEST(BbgMachine, ParallelPortStatusReadsLikeAnIdleNoPrinterPort) {
  BbgMachine machine(BbgVariant::floppyOne(), syntheticBios(BbgVariant::floppyOne()));
  EXPECT_EQ(peek(machine, 0xFF48), 0x38) << "没有接打印机：无错、已选中、缺纸";
  EXPECT_EQ(peek(machine, 0xFF40), 0);
}

TEST(BbgMachine, KeyboardAndMouseBelongToTheMachine) {
  BbgMachine machine(BbgVariant::floppyOne(), syntheticBios(BbgVariant::floppyOne()));
  EXPECT_TRUE(machine.keyboardIsMachineInput());
  EXPECT_TRUE(machine.hasMouse());
  EXPECT_EQ(machine.padPortCount(), 1U);
  machine.input(hw::KeyInput{"Digit4", true});
  machine.keyboard().write(0x05);
  machine.keyboard().write(0x04);
  EXPECT_EQ(machine.keyboard().read(), 0x1C);
  machine.input(hw::KeyInput{"Digit4", false});
  EXPECT_EQ(machine.keyboard().read(), 0x1E);
  EXPECT_THROW(machine.setButtons(1, 0), ember::shared::CoreError);
}

TEST(BbgMachine, FloppyDriveAndPrinterPresence) {
  BbgMachine machine(BbgVariant::floppyOne(), syntheticBios(BbgVariant::floppyOne()));
  EXPECT_EQ(machine.floppyDriveCount(), 1U);
  EXPECT_NE(machine.floppyDrive(0), nullptr);
  EXPECT_EQ(machine.floppyDrive(1), nullptr);
  EXPECT_EQ(machine.printer(), nullptr) << "没有输出方时并口不接打印机";
}

TEST(BbgMachine, PowerOnRunsTheProgramFromTheResetVector) {
  BbgMachine machine(BbgVariant::floppyOne(), syntheticBios(BbgVariant::floppyOne()));
  machine.powerOn();
  machine.runFrame();
  machine.runFrame();
  EXPECT_NE(peek(machine, 0x00), 0) << "程序在运行";
  const std::uint8_t before = peek(machine, 0x00);
  machine.softReset();
  machine.runFrame();
  EXPECT_NE(peek(machine, 0x00), before);
}

// ---------------------------------------------------------------- 真实 BIOS

void bootRealBios(const char* file, const BbgVariant& variant, const char* label, int frames, const char8_t* disk) {
  std::vector<std::uint8_t> bios = realBios(file, variant);
  if (bios.empty()) GTEST_SKIP() << std::string("没有找到 ember/roms/") + file;
  BbgMachine machine(variant, std::move(bios));
  if (disk != nullptr) {
    std::vector<std::uint8_t> image = realFloppy(std::filesystem::path(u8"reference/floppy-images/bbg") / std::u8string(disk));
    if (image.empty()) GTEST_SKIP() << "没有找到盘片镜像";
    machine.floppyDrive(0)->insertImage(std::move(image), false, "dos.img");
  }
  machine.powerOn();
  for (int frame = 0; frame < frames; ++frame) machine.runFrame();
  if (disk != nullptr) {
    // 回车跳过日期与时间，再输入 dir：键盘经 $4016、$4017 的矩阵扫描被固件读到。
    const auto tap = [&](const char* code) {
      machine.input(hw::KeyInput{code, true});
      for (int frame = 0; frame < 6; ++frame) machine.runFrame();
      machine.input(hw::KeyInput{code, false});
      for (int frame = 0; frame < 6; ++frame) machine.runFrame();
    };
    tap("Enter");
    for (int frame = 0; frame < 60; ++frame) machine.runFrame();
    tap("Enter");
    for (int frame = 0; frame < 120; ++frame) machine.runFrame();
    for (const char* code : {"KeyD", "KeyI", "KeyR", "Enter"}) tap(code);
    for (int frame = 0; frame < 300; ++frame) machine.runFrame();
  }
  const hw::FrameBuffer picture = machine.frame();
  ASSERT_EQ(picture.width, 256U);
  ASSERT_EQ(picture.height, 240U);
  std::set<std::uint32_t> colors(picture.pixels.begin(), picture.pixels.end());
  EXPECT_GT(colors.size(), 1U) << "BIOS 开机后画面不应该是纯色";
  if (const std::string dump = dumpPath(); !dump.empty()) dumpFrame(picture, dump + "." + label + ".ppm");
}

TEST(BbgMachine, RealBios10BootsToAPicture) { bootRealBios("bbg1_bios.bin", BbgVariant::floppyOne(), "bios10", 300, nullptr); }

TEST(BbgMachine, RealBios20BootsToAPicture) { bootRealBios("bbg98_bios.bin", BbgVariant::model98(), "bios20", 300, nullptr); }

TEST(BbgMachine, RealBios10BootsTheDosDisk) { bootRealBios("bbg1_bios.bin", BbgVariant::floppyOne(), "bios10-dos", 2500, u8"dos启动盘.IMG"); }

TEST(BbgMachine, RealBios20BootsTheDosDisk) { bootRealBios("bbg98_bios.bin", BbgVariant::model98(), "bios20-dos", 2500, u8"dos启动盘.IMG"); }

// ---------------------------------------------------------------- 格式化软盘

/** 在步步高 DOS 命令行里操作的会话：用 DOS 启动盘开机并跳过日期与时间，之后可以输入命令、等待。 */
class DosSession {
 public:
  DosSession(const BbgVariant& variant, std::vector<std::uint8_t> bios, std::vector<std::uint8_t> image) : machine_(variant, std::move(bios)) {
    machine_.floppyDrive(0)->insertImage(std::move(image), false, "dos.img");
    machine_.powerOn();
    wait(2500);
    tap("Enter", false);
    wait(60);
    tap("Enter", false);
    wait(120);
  }

  void wait(int frames) {
    for (int frame = 0; frame < frames; ++frame) machine_.runFrame();
  }

  /** 按下并松开一个键；shift 为真时先按住左 Shift。 */
  void tap(const std::string& code, bool shift) {
    if (shift) {
      machine_.input(hw::KeyInput{"ShiftLeft", true});
      wait(6);
    }
    machine_.input(hw::KeyInput{code, true});
    wait(6);
    machine_.input(hw::KeyInput{code, false});
    wait(6);
    if (shift) {
      machine_.input(hw::KeyInput{"ShiftLeft", false});
      wait(6);
    }
  }

  /** 输入一行命令（小写字母、数字、空格、点与冒号），不含回车。 */
  void type(std::string_view text) {
    for (const char letter : text) {
      if (letter >= 'a' && letter <= 'z') tap(std::string("Key") + static_cast<char>(letter - 'a' + 'A'), false);
      else if (letter >= '0' && letter <= '9') tap(std::string("Digit") + letter, false);
      else if (letter == ' ') tap("Space", false);
      else if (letter == '.') tap("Period", false);
      else if (letter == ':') tap("Semicolon", true);
    }
  }

  void enter() { tap("Enter", false); }

  [[nodiscard]] BbgMachine& machine() { return machine_; }

 private:
  BbgMachine machine_;
};

/** 让 DOS 格式化软驱里的盘片，返回格式化后的镜像；BIOS 或盘片缺失时返回空。 */
std::vector<std::uint8_t> formatTheFloppy(const char* biosFile, const BbgVariant& variant, std::vector<std::uint8_t> image, bool formatIsOnDriveC, const char* label) {
  std::vector<std::uint8_t> bios = realBios(biosFile, variant);
  if (bios.empty() || image.empty()) return {};
  DosSession session(variant, std::move(bios), std::move(image));
  if (formatIsOnDriveC) {
    session.type("c:");
    session.enter();
    session.wait(120);
  }
  session.type("format a:");
  session.enter();
  session.wait(300);
  session.enter();
  session.wait(3000);
  if (const std::string dump = dumpPath(); !dump.empty()) dumpFrame(session.machine().frame(), dump + "." + label + ".ppm");
  return session.machine().floppyDrive(0)->image();
}

/** 软驱一号没有 C 盘：先在步步高 98 的 C 盘里把 FORMAT.COM 复制到 DOS 启动盘上，返回复制后的镜像。 */
std::vector<std::uint8_t> diskWithTheFormatProgram() {
  std::vector<std::uint8_t> bios = realBios("bbg98_bios.bin", BbgVariant::model98());
  std::vector<std::uint8_t> image = realFloppy(std::filesystem::path(u8"reference/floppy-images/bbg") / std::u8string(u8"dos启动盘.IMG"));
  if (bios.empty() || image.empty()) return {};
  DosSession session(BbgVariant::model98(), std::move(bios), std::move(image));
  session.type("c:");
  session.enter();
  session.wait(120);
  session.type("copy format.cmd a:");
  session.enter();
  session.wait(900);
  if (const std::string dump = dumpPath(); !dump.empty()) dumpFrame(session.machine().frame(), dump + ".bios20-copy-format.ppm");
  return session.machine().floppyDrive(0)->image();
}

/** 格式化后：没有被 FAT 与根目录占用的磁道都被写成了填充字节 $F6。 */
void expectFormatted(const std::vector<std::uint8_t>& formatted) {
  ASSERT_EQ(formatted.size(), 1440U * 1024);
  constexpr std::size_t TRACK_BYTES = 18 * 512;
  constexpr std::size_t FIRST_DATA_TRACK = 3;
  std::size_t filledTracks = 0;
  for (std::size_t track = FIRST_DATA_TRACK; track < formatted.size() / TRACK_BYTES; ++track) {
    const auto begin = formatted.begin() + static_cast<std::ptrdiff_t>(track * TRACK_BYTES);
    if (std::all_of(begin, begin + TRACK_BYTES, [](std::uint8_t value) { return value == 0xF6; })) ++filledTracks;
  }
  EXPECT_GT(filledTracks, 150U) << "80 道 2 面共 160 个磁道，除了 FAT 与根目录所在的开头几个磁道外都应该被格式化";
}

TEST(BbgMachine, RealBios10FormatsAFloppyDisk) {
  std::vector<std::uint8_t> image = diskWithTheFormatProgram();
  if (image.empty()) GTEST_SKIP() << "没有找到 BIOS 或盘片镜像";
  const std::vector<std::uint8_t> formatted = formatTheFloppy("bbg1_bios.bin", BbgVariant::floppyOne(), std::move(image), false, "bios10-format");
  if (formatted.empty()) GTEST_SKIP() << "没有找到 BIOS";
  expectFormatted(formatted);
}

TEST(BbgMachine, RealBios20FormatsAFloppyDisk) {
  std::vector<std::uint8_t> image = realFloppy(std::filesystem::path(u8"reference/floppy-images/bbg") / std::u8string(u8"dos启动盘.IMG"));
  const std::vector<std::uint8_t> formatted = formatTheFloppy("bbg98_bios.bin", BbgVariant::model98(), std::move(image), true, "bios20-format");
  if (formatted.empty()) GTEST_SKIP() << "没有找到 BIOS 或盘片镜像";
  expectFormatted(formatted);
}

}  // namespace
