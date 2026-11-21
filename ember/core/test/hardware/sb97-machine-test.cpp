/*
 * ------------------------------------------------------------------------
 * 名称：sb97-machine-test.cpp
 * 说明：主机型号 sb97（小霸王 97 学习机）的测试：BIOS 大小、程序窗口的页寄存器（16K 模式、32K 模式、程序 RAM 模式）、工作 RAM、整机寄存器的读写、软驱寄存器的地址、名称表镜像、字符 RAM、背景图案表的选择与键盘；以及用真实 BIOS 开机的测试（小霸王画面、按键后进菜单、菜单下半屏、DOS 与读盘、F-BASIC）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-17
 * 备注：存储译码用合成的 BIOS 检查：每页的第一个字节写页号（第 0 页写 $A5），复位向量指向第 0 页里的原地跳转；
 *       真实 BIOS 在仓库的 ember/roms/sb97.bin（1MB 的纯 BIOS，没有文件头），文件不存在时跳过这些测试。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>

#include "bootstrap/machines/sb97/sb97.hpp"
#include "contracts/hardware/floppy-activity.hpp"
#include "contracts/hardware/input-event.hpp"
#include "shared/errors/core-error.hpp"

namespace {

namespace hw = ember::contracts::hardware;
using ember::bootstrap::Sb97Board;
using ember::bootstrap::Sb97Machine;

constexpr std::uint8_t PAGE_ZERO_MARK = 0xA5;

/** 合成的 BIOS：其余字节是 NOP，每页第一个字节标页号，复位向量指向 $FF00 的原地跳转。 */
std::vector<std::uint8_t> syntheticBios() {
  std::vector<std::uint8_t> bios(Sb97Board::ROM_SIZE, 0xEA);
  for (std::size_t page = 0; page < Sb97Board::PAGE_COUNT; ++page) bios[page * Sb97Board::PAGE_SIZE] = page == 0 ? PAGE_ZERO_MARK : static_cast<std::uint8_t>(page);
  const std::size_t vectors = Sb97Board::PAGE_SIZE - 6;
  for (std::size_t vector = 0; vector < 3; ++vector) {
    bios[vectors + vector * 2] = 0x00;
    bios[vectors + vector * 2 + 1] = 0xFF;
  }
  const std::size_t loop = Sb97Board::PAGE_SIZE - 0x100;
  bios[loop] = 0x4C;
  bios[loop + 1] = 0x00;
  bios[loop + 2] = 0xFF;
  return bios;
}

/** 经真实总线写一个字节（与 CPU 写一样有副作用）。 */
void poke(Sb97Machine& machine, std::uint16_t address, std::uint8_t value) {
  const std::array<std::uint8_t, 1> bytes{value};
  machine.debugger()->writeMemory("cpu", address, bytes);
}

std::uint8_t peek(Sb97Machine& machine, std::uint16_t address) { return machine.peekCpuBus(address); }

std::uint8_t readPpu(Sb97Machine& machine, std::uint32_t address) {
  std::array<std::uint8_t, 1> bytes{};
  machine.debugger()->readMemory("ppu", address, bytes);
  return bytes[0];
}

void pokePpu(Sb97Machine& machine, std::uint32_t address, std::uint8_t value) {
  const std::array<std::uint8_t, 1> bytes{value};
  machine.debugger()->writeMemory("ppu", address, bytes);
}

/** 上电后的主机。 */
struct PoweredMachine {
  PoweredMachine() : machine(syntheticBios()) { machine.powerOn(); }
  Sb97Machine machine;
};

TEST(Sb97Machine, RejectsABiosOfTheWrongSize) {
  EXPECT_THROW(Sb97Machine(std::vector<std::uint8_t>(512 * 1024)), ember::shared::CoreError);
  EXPECT_THROW(Sb97Machine(std::vector<std::uint8_t>(Sb97Board::ROM_SIZE + 1)), ember::shared::CoreError);
}

TEST(Sb97Machine, AfterPowerOnPageZeroIsAtBothHalvesOfTheWindow) {
  PoweredMachine rig;
  EXPECT_EQ(peek(rig.machine, 0x8000), PAGE_ZERO_MARK);
  EXPECT_EQ(peek(rig.machine, 0xC000), PAGE_ZERO_MARK);
}

TEST(Sb97Machine, SixteenKilobyteModeSelectsTheWindowPageAndKeepsPageZeroOnTop) {
  PoweredMachine rig;
  poke(rig.machine, 0x5000, 5);
  EXPECT_EQ(peek(rig.machine, 0x8000), 5);
  EXPECT_EQ(peek(rig.machine, 0xC000), PAGE_ZERO_MARK);
  poke(rig.machine, 0x5000, 0x45);
  EXPECT_EQ(peek(rig.machine, 0x8000), 5) << "页号按 64 页取模";
  poke(rig.machine, 0x5000, 63);
  EXPECT_EQ(peek(rig.machine, 0x8000), 63);
  poke(rig.machine, 0x5000, 0x40);
  EXPECT_EQ(peek(rig.machine, 0x8000), PAGE_ZERO_MARK) << "第 64 页就是第 0 页";
}

TEST(Sb97Machine, ThirtyTwoKilobyteModeUsesPairsOfPages) {
  PoweredMachine rig;
  poke(rig.machine, 0x5200, 0x04);
  poke(rig.machine, 0x5000, 3);
  EXPECT_EQ(peek(rig.machine, 0x8000), 6);
  EXPECT_EQ(peek(rig.machine, 0xC000), 7);
  poke(rig.machine, 0x5000, 0x23);
  EXPECT_EQ(peek(rig.machine, 0x8000), 6) << "32K 页号按 32 个取模";
  poke(rig.machine, 0x5000, 0);
  EXPECT_EQ(peek(rig.machine, 0x8000), PAGE_ZERO_MARK);
  EXPECT_EQ(peek(rig.machine, 0xC000), 1);
}

TEST(Sb97Machine, ProgramRamModeReplacesTheWindowWithWritableRamInSixteenKilobyteMode) {
  PoweredMachine rig;
  poke(rig.machine, 0x5000, 0x82);
  poke(rig.machine, 0x8000, 0x5A);
  poke(rig.machine, 0xBFFF, 0x6B);
  EXPECT_EQ(peek(rig.machine, 0x8000), 0x5A);
  EXPECT_EQ(peek(rig.machine, 0xBFFF), 0x6B);
  poke(rig.machine, 0x5000, 0x8A);
  EXPECT_EQ(peek(rig.machine, 0x8000), 0x5A) << "RAM 页号只取低 3 位";
  poke(rig.machine, 0x5000, 0x83);
  EXPECT_EQ(peek(rig.machine, 0x8000), 0) << "另一个 16K 是空的";
  // $C000 起仍是 ROM，写入被忽略。
  poke(rig.machine, 0xC000, 0x11);
  EXPECT_EQ(peek(rig.machine, 0xC000), PAGE_ZERO_MARK);
  // 回到 ROM 模式后看到的是 ROM。
  poke(rig.machine, 0x5000, 2);
  EXPECT_EQ(peek(rig.machine, 0x8000), 2);
}

TEST(Sb97Machine, ProgramRamModeInThirtyTwoKilobyteModeMakesTheWholeWindowRam) {
  PoweredMachine rig;
  poke(rig.machine, 0x5200, 0x04);
  poke(rig.machine, 0x5000, 0x81);
  poke(rig.machine, 0x8000, 0x11);
  poke(rig.machine, 0xFFFF, 0x77);
  EXPECT_EQ(peek(rig.machine, 0x8000), 0x11);
  EXPECT_EQ(peek(rig.machine, 0xFFFF), 0x77);
  poke(rig.machine, 0x5000, 0x85);
  EXPECT_EQ(peek(rig.machine, 0xFFFF), 0x77) << "32K RAM 页号只取低 2 位";
  poke(rig.machine, 0x5000, 0x82);
  EXPECT_EQ(peek(rig.machine, 0xFFFF), 0);
}

TEST(Sb97Machine, WritingToRomIsIgnored) {
  PoweredMachine rig;
  poke(rig.machine, 0x8000, 0x33);
  poke(rig.machine, 0xC000, 0x44);
  EXPECT_EQ(peek(rig.machine, 0x8000), PAGE_ZERO_MARK);
  EXPECT_EQ(peek(rig.machine, 0xC000), PAGE_ZERO_MARK);
}

TEST(Sb97Machine, WorkRamIsEightKilobytesAtSixThousandAndIndependentOfTheWindow) {
  PoweredMachine rig;
  poke(rig.machine, 0x6000, 0x12);
  poke(rig.machine, 0x7FFF, 0x34);
  EXPECT_EQ(peek(rig.machine, 0x6000), 0x12);
  EXPECT_EQ(peek(rig.machine, 0x7FFF), 0x34);
  poke(rig.machine, 0x5000, 0x80);
  poke(rig.machine, 0x8000, 0x99);
  EXPECT_EQ(peek(rig.machine, 0x6000), 0x12);
  EXPECT_EQ(rig.machine.board().workRam()[0], 0x12);
}

TEST(Sb97Machine, TheRegistersAreWriteOnlyAndUnusedAddressesDoNotDriveTheBus) {
  PoweredMachine rig;
  Sb97Board& board = rig.machine.board();
  EXPECT_EQ(board.cpuSide().peek(0x5000).drivenMask, 0);
  EXPECT_EQ(board.cpuSide().peek(0x5200).drivenMask, 0);
  EXPECT_EQ(board.cpuSide().peek(0x5500).drivenMask, 0) << "软驱的写端口读不出东西";
  EXPECT_EQ(board.cpuSide().peek(0x4800).drivenMask, 0);
  EXPECT_EQ(board.cpuSide().peek(0x5FFF).drivenMask, 0);
  EXPECT_EQ(board.cpuSide().peek(0x4020).drivenMask, 0xFF) << "$4020-$40FF 读出 0";
  EXPECT_EQ(board.cpuSide().peek(0x4020).value, 0);
  poke(rig.machine, 0x480D, 0x02);  // 键盘灯等没有器件的地址：写入忽略
  EXPECT_EQ(board.registers().programPage, 0);
}

TEST(Sb97Machine, TheRegistersKeepWhatWasWrittenAndResetClearsThem) {
  PoweredMachine rig;
  poke(rig.machine, 0x5000, 0x2F);
  poke(rig.machine, 0x5200, 0x05);
  Sb97Board::Registers registers = rig.machine.board().registers();
  EXPECT_EQ(registers.programPage, 0x2F);
  EXPECT_EQ(registers.control, 0x05);
  EXPECT_TRUE(registers.thirtyTwoKilobyteMode);
  EXPECT_TRUE(registers.horizontalMirroring);
  EXPECT_FALSE(registers.programRamMode);
  rig.machine.softReset();
  registers = rig.machine.board().registers();
  EXPECT_EQ(registers.programPage, 0);
  EXPECT_EQ(registers.control, 0);
}

TEST(Sb97Machine, FloppyControllerRegistersAreAtFiveFiveZeroZeroAndFiveSixZeroZero) {
  PoweredMachine rig;
  poke(rig.machine, 0x5501, 0x1C);
  EXPECT_EQ(rig.machine.fdc().dor(), 0x1C) << "$5501 对应寄存器 2（DOR）";
  poke(rig.machine, 0x5502, 0x0C);
  EXPECT_EQ(rig.machine.fdc().dor(), 0x0C) << "$5502 也对应寄存器 2";
  poke(rig.machine, 0x5501, 0x1C);
  EXPECT_EQ(rig.machine.board().cpuSide().peek(0x5604).value, rig.machine.fdc().mainStatus()) << "$5604 是主状态寄存器";
  EXPECT_EQ(rig.machine.board().cpuSide().peek(0x5604).drivenMask, 0xFF);
  EXPECT_EQ(rig.machine.board().cpuSide().peek(0x5602).value, 0x1C) << "$5602 读出 DOR";
}

TEST(Sb97Machine, NametableMirroringFollowsBitZeroOfFiveTwoHundred) {
  PoweredMachine rig;
  // 垂直镜像：名称表 0 与 2 相同，1 与 3 相同。
  pokePpu(rig.machine, 0x2000, 0x11);
  pokePpu(rig.machine, 0x2400, 0x22);
  EXPECT_EQ(readPpu(rig.machine, 0x2800), 0x11);
  EXPECT_EQ(readPpu(rig.machine, 0x2C00), 0x22);
  EXPECT_EQ(readPpu(rig.machine, 0x3000), 0x11) << "$3000 起镜像 $2000";
  // 水平镜像：0 与 1 相同，2 与 3 相同。
  poke(rig.machine, 0x5200, 0x01);
  pokePpu(rig.machine, 0x2000, 0x33);
  pokePpu(rig.machine, 0x2800, 0x44);
  EXPECT_EQ(readPpu(rig.machine, 0x2400), 0x33);
  EXPECT_EQ(readPpu(rig.machine, 0x2C00), 0x44);
}

TEST(Sb97Machine, CharacterRamIsEightKilobytesAtTheBottomOfThePpuSpace) {
  PoweredMachine rig;
  pokePpu(rig.machine, 0x0000, 0xAA);
  pokePpu(rig.machine, 0x1FFF, 0xBB);
  EXPECT_EQ(readPpu(rig.machine, 0x0000), 0xAA);
  EXPECT_EQ(readPpu(rig.machine, 0x1FFF), 0xBB);
  EXPECT_EQ(rig.machine.board().characterRam()[0x1FFF], 0xBB);
  EXPECT_EQ(readPpu(rig.machine, 0x2000), 0) << "名称表是另一块存储";
}

TEST(Sb97Machine, TheBackgroundPatternTableFollowsTheNametableAndFiveTwoHundred) {
  PoweredMachine rig;
  const Sb97Board& board = rig.machine.board();
  const auto tables = [&] { return std::array<bool, 4>{board.backgroundUsesTableOne(0), board.backgroundUsesTableOne(1), board.backgroundUsesTableOne(2), board.backgroundUsesTableOne(3)}; };
  EXPECT_EQ(tables(), (std::array<bool, 4>{false, false, true, false})) << "名称表 2 总是用图案表 1";
  poke(rig.machine, 0x5200, 0x02);
  EXPECT_EQ(tables(), (std::array<bool, 4>{false, true, true, true})) << "$5200 的低 2 位等于 2 时名称表 1、3 也用图案表 1";
  poke(rig.machine, 0x5200, 0x03);
  EXPECT_EQ(tables(), (std::array<bool, 4>{false, false, true, false}));
  poke(rig.machine, 0x5200, 0x06);
  EXPECT_EQ(tables(), (std::array<bool, 4>{false, true, true, true}));
  EXPECT_TRUE(rig.machine.board().registers().tableSwitch);
}

TEST(Sb97Machine, ProgramRunsFromTheResetVectorAndTheFrameIsDendy) {
  PoweredMachine rig;
  for (int frame = 0; frame < 3; ++frame) rig.machine.runFrame();
  const std::uint16_t pc = rig.machine.cpuRegisters().pc;
  EXPECT_TRUE(pc >= 0xFF00 && pc <= 0xFF02) << std::hex << pc;
  const hw::FrameBuffer picture = rig.machine.frame();
  EXPECT_EQ(picture.width, 256U);
  EXPECT_EQ(picture.height, 240U);
  EXPECT_NEAR(rig.machine.framesPerSecond(), 50.0, 0.05);
  EXPECT_EQ(rig.machine.padPortCount(), 1U);
  EXPECT_TRUE(rig.machine.keyboardIsMachineInput());
  EXPECT_FALSE(rig.machine.hasMouse());
}

TEST(Sb97Machine, KeyboardEventsReachTheMatrixAndScanWritesGoToTheKeyboard) {
  PoweredMachine rig;
  rig.machine.input(hw::KeyInput{"KeyA", true});
  poke(rig.machine, 0x4016, 0x05);
  for (int position = 0; position < 6; ++position) {
    poke(rig.machine, 0x4016, 0x04);
    if (position < 5) poke(rig.machine, 0x4016, 0x06);
  }
  poke(rig.machine, 0x4016, 0x06);
  EXPECT_EQ(rig.machine.keyboard().position(), 6U);
  EXPECT_EQ(rig.machine.keyboard().read(), 0x1A) << "KeyA 在位置 6 的后半边";
  rig.machine.input(hw::KeyInput{"KeyA", false});
  EXPECT_EQ(rig.machine.keyboard().read(), 0x1E);
  rig.machine.input(hw::KeyInput{"F13", true});  // 键位表里没有的键丢弃
  EXPECT_EQ(rig.machine.keyboard().read(), 0x1E);
}

TEST(Sb97Machine, OnlyOneJoypadPortExists) {
  PoweredMachine rig;
  EXPECT_NO_THROW(rig.machine.setButtons(0, 0x81));
  EXPECT_THROW(rig.machine.setButtons(1, 0x01), ember::shared::CoreError);
}

// ---------------------------------------------------------------- 真实 BIOS

/** 读仓库里的文件；不存在时返回空。 */
std::vector<std::uint8_t> readReferenceFile(const std::filesystem::path& relative) {
  const std::filesystem::path path = std::filesystem::path(EMBER_TEST_DATA_DIR) / ".." / ".." / ".." / ".." / relative;
  std::ifstream stream(path, std::ios::binary);
  if (!stream) return {};
  return std::vector<std::uint8_t>((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
}

/** 真实 BIOS（ember/roms 里没有文件头的纯 BIOS）；文件不存在或大小不对时返回空。 */
std::vector<std::uint8_t> realBios() {
  std::vector<std::uint8_t> file = readReferenceFile(std::filesystem::path("ember") / "roms" / "sb97.bin");
  if (file.size() != Sb97Board::ROM_SIZE) return {};
  return file;
}

/** 画面里白色像素的个数。 */
std::size_t whitePixels(const hw::FrameBuffer& picture, int firstRow = 0, int lastRow = 239) {
  std::size_t count = 0;
  for (int row = firstRow; row <= lastRow; ++row) {
    for (int column = 0; column < 256; ++column) {
      if ((picture.pixels[static_cast<std::size_t>(row) * 256 + static_cast<std::size_t>(column)] & 0xFFFFFFU) == 0xFFFFFFU) ++count;
    }
  }
  return count;
}

/** 画面里不是黑色的像素的个数。 */
std::size_t litPixels(const hw::FrameBuffer& picture) {
  return static_cast<std::size_t>(std::count_if(picture.pixels.begin(), picture.pixels.end(), [](std::uint32_t pixel) { return (pixel & 0xFFFFFFU) != 0; }));
}

void run(Sb97Machine& machine, int frames) {
  for (int frame = 0; frame < frames; ++frame) machine.runFrame();
}

/** 按一下键：按下与松开各保持 8 帧。 */
void tap(Sb97Machine& machine, const char* code) {
  machine.input(hw::KeyInput{code, true});
  run(machine, 8);
  machine.input(hw::KeyInput{code, false});
  run(machine, 8);
}

/** 开机、在小霸王画面按一下回车，等硬件检测结束，停在主菜单（初始高亮的是“指法训练”）。 */
void bootToMenu(Sb97Machine& machine) {
  machine.powerOn();
  run(machine, 1200);
  tap(machine, "Enter");
  run(machine, 500);
}

TEST(Sb97Machine, RealBiosShowsTheLogoWithinTwoAndAHalfSeconds) {
  std::vector<std::uint8_t> bios = realBios();
  if (bios.empty()) GTEST_SKIP() << "没有找到 ember/roms/sb97.bin";
  Sb97Machine machine(std::move(bios));
  machine.powerOn();
  run(machine, 150);
  EXPECT_GT(litPixels(machine.frame()), 3000U) << "开机 150 帧（3 秒）内应该已经有画面";
}

TEST(Sb97Machine, RealBiosWaitsOnTheLogoScreenForAKeyThenChecksTheHardwareAndShowsTheMenu) {
  std::vector<std::uint8_t> bios = realBios();
  if (bios.empty()) GTEST_SKIP() << "没有找到 ember/roms/sb97.bin";
  Sb97Machine machine(std::move(bios));
  machine.powerOn();
  run(machine, 1200);
  const std::size_t logoWhite = whitePixels(machine.frame());
  EXPECT_GT(logoWhite, 200U) << "小霸王画面有白色的字";
  tap(machine, "Enter");
  EXPECT_LT(whitePixels(machine.frame()), 600U) << "按键后先是“正在检测”";
  run(machine, 400);
  EXPECT_GT(whitePixels(machine.frame()), 3000U) << "菜单的按钮是白色的";
}

TEST(Sb97Machine, RealBiosMenuShowsTheLowerHalfWithTheItemsBelowTheMiddle) {
  std::vector<std::uint8_t> bios = realBios();
  if (bios.empty()) GTEST_SKIP() << "没有找到 ember/roms/sb97.bin";
  Sb97Machine machine(std::move(bios));
  bootToMenu(machine);
  // 菜单下半屏有“音乐之窗”“WPS”“认知码”三个八边形按钮，在第 140-200 行。
  EXPECT_GT(whitePixels(machine.frame(), 140, 200), 800U);
}

TEST(Sb97Machine, RealBiosMenuStartsDosAtTheCenterAndShowsThePrompt) {
  std::vector<std::uint8_t> bios = realBios();
  if (bios.empty()) GTEST_SKIP() << "没有找到 ember/roms/sb97.bin";
  Sb97Machine machine(std::move(bios));
  bootToMenu(machine);
  const std::size_t menuWhite = whitePixels(machine.frame());
  tap(machine, "ArrowLeft");
  run(machine, 20);
  tap(machine, "Enter");
  run(machine, 600);
  const std::size_t dosWhite = whitePixels(machine.frame());
  EXPECT_GT(dosWhite, 100U);
  EXPECT_LT(dosWhite, menuWhite / 2) << "DOS 是黑底白字，白色比菜单少得多";
  EXPECT_EQ(machine.board().registers().programPage != 0, true);
}

TEST(Sb97Machine, RealBiosFBasicStartsTheBasicPromptNotAnotherProgram) {
  std::vector<std::uint8_t> bios = realBios();
  if (bios.empty()) GTEST_SKIP() << "没有找到 ember/roms/sb97.bin";
  Sb97Machine machine(std::move(bios));
  bootToMenu(machine);
  // 菜单顺时针转：指法训练、打字游戏、五笔字型、认知码、WPS、音乐之窗、计算器、F-BASIC。
  for (int step = 0; step < 7; ++step) tap(machine, "ArrowRight");
  tap(machine, "Enter");
  run(machine, 400);
  const std::size_t promptWhite = whitePixels(machine.frame());
  EXPECT_GT(promptWhite, 0U);
  EXPECT_LT(promptWhite, 300U) << "F-BASIC 的画面只有一个提示符，不是菜单或 WPS 的满屏文字";
  tap(machine, "KeyA");
  run(machine, 20);
  EXPECT_GT(whitePixels(machine.frame()), promptWhite) << "输入的字符显示在提示符后面";
}

TEST(Sb97Machine, RealBiosDosListsTheFilesOnTheFloppyDisk) {
  std::vector<std::uint8_t> bios = realBios();
  std::vector<std::uint8_t> image = readReferenceFile(std::filesystem::path("reference") / "floppy-images" / "sb97" / "1.IMA");
  if (bios.empty() || image.empty()) GTEST_SKIP() << "没有找到 ember/roms/sb97.bin 或 reference/floppy-images/sb97/1.IMA";
  struct MotorLog final : hw::FloppyActivity {
    int turnedOn{0};
    void motorChanged(unsigned /*drive*/, bool on) override {
      if (on) ++turnedOn;
    }
  } motors;
  Sb97Machine machine(std::move(bios), &motors);
  machine.floppyDrive(0)->insertImage(std::move(image), false, "1.IMA");
  bootToMenu(machine);
  tap(machine, "ArrowLeft");
  run(machine, 20);
  tap(machine, "Enter");
  run(machine, 600);
  const std::size_t beforeDir = whitePixels(machine.frame());
  const int motorsBeforeDir = motors.turnedOn;
  for (const char* code : {"KeyD", "KeyI", "KeyR", "Enter"}) tap(machine, code);
  run(machine, 400);
  EXPECT_GT(motors.turnedOn, motorsBeforeDir) << "dir 要读盘，软驱马达应该转起来";
  EXPECT_GT(whitePixels(machine.frame()), beforeDir) << "dir 之后屏幕上应该多出文件列表";
}

}  // namespace
