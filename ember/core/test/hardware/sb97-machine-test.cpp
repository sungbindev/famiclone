/*
 * ------------------------------------------------------------------------
 * 名称：sb97-machine-test.cpp
 * 说明：主机型号 sb97（小霸王 97 学习机）的测试：BIOS 大小、存储译码（用合成的 BIOS 检查）、$5000 页寄存器、主板 RAM、软驱控制器的寄存器、四屏名称表与字符 RAM；以及用真实 BIOS 开机的冒烟测试。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-17
 * 备注：真实 BIOS 在仓库的 reference/rom/小霸王/SB97.nes（iNES 文件，16 字节文件头之后是 1MB 的 PRG），文件不存在时跳过冒烟测试；
 *       设置环境变量 EMBER_SB97_DUMP 为一个文件路径时，冒烟测试把开机后的画面写成以它为前缀的 PPM 文件，供人工查看。
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
#include <utility>
#include <vector>

#include "bootstrap/machines/sb97/sb97.hpp"
#include "contracts/hardware/hardware-error.hpp"
#include "shared/errors/core-error.hpp"

namespace {

namespace hw = ember::contracts::hardware;
using ember::bootstrap::Sb97Board;
using ember::bootstrap::Sb97Machine;

/** 合成的 BIOS：每个 16K 页的第一个字节是页号；第 0 页的向量表指向 $C100，那里是 CLI 后在 $00 上无限计数的程序。 */
std::vector<std::uint8_t> syntheticBios() {
  std::vector<std::uint8_t> bios(Sb97Board::ROM_SIZE, 0xEA);
  for (std::size_t page = 0; page < Sb97Board::PAGE_COUNT; ++page) bios[page * Sb97Board::PAGE_SIZE] = static_cast<std::uint8_t>(page);
  const std::size_t vectors = Sb97Board::PAGE_SIZE - 6;
  const std::uint8_t table[] = {0x00, 0xC1, 0x00, 0xC1, 0x00, 0xC1};
  std::copy(std::begin(table), std::end(table), bios.begin() + static_cast<std::ptrdiff_t>(vectors));
  const std::uint8_t program[] = {
      0x58,             // CLI
      0xE6, 0x00,       // loop: INC $00
      0x4C, 0x01, 0xC1  // JMP loop（$C101）
  };
  std::copy(std::begin(program), std::end(program), bios.begin() + 0x100);
  return bios;
}

/** 读真实的 BIOS：跳过 iNES 文件头；文件不存在或大小不对时返回空。 */
std::vector<std::uint8_t> realBios() {
  const std::filesystem::path path = std::filesystem::path(EMBER_TEST_DATA_DIR) / ".." / ".." / ".." / ".." / "reference" / "rom" / u8"小霸王" / "SB97.nes";
  std::ifstream stream(path, std::ios::binary);
  if (!stream) return {};
  std::vector<std::uint8_t> file((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
  if (file.size() != 16 + Sb97Board::ROM_SIZE) return {};
  return std::vector<std::uint8_t>(file.begin() + 16, file.end());
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

/** 环境变量 EMBER_SB97_DUMP 的值；没有设置时为空。 */
std::string dumpPath() {
#ifdef _MSC_VER
  char* value = nullptr;
  std::size_t length = 0;
  if (_dupenv_s(&value, &length, "EMBER_SB97_DUMP") != 0 || value == nullptr) return {};
  std::string path(value);
  std::free(value);
  return path;
#else
  const char* value = std::getenv("EMBER_SB97_DUMP");
  return value == nullptr ? std::string() : std::string(value);
#endif
}

std::uint8_t peek(Sb97Machine& machine, std::uint16_t address) { return machine.peekCpuBus(address); }

void poke(Sb97Machine& machine, std::uint16_t address, std::uint8_t value) {
  const std::uint8_t bytes[] = {value};
  machine.debugger()->writeMemory("cpu", address, bytes);
}

void pokeVideo(Sb97Machine& machine, std::uint16_t address, std::uint8_t value) {
  const std::uint8_t bytes[] = {value};
  machine.debugger()->writeMemory("ppu", address, bytes);
}

std::uint8_t peekVideo(Sb97Machine& machine, std::uint16_t address) {
  std::uint8_t value = 0;
  machine.debugger()->readMemory("ppu", address, std::span<std::uint8_t>(&value, 1));
  return value;
}

TEST(Sb97Machine, RejectsABiosOfTheWrongSize) {
  try {
    Sb97Machine machine(std::vector<std::uint8_t>(1024, 0));
    FAIL() << "BIOS 大小不对应该抛出错误";
  } catch (const ember::shared::CoreError& error) {
    EXPECT_EQ(error.code(), hw::ERROR_INVALID_ARGUMENT);
  }
}

TEST(Sb97Machine, AfterPowerOnThePageZeroIsFixedAtTheTopAndAlsoInTheWindow) {
  Sb97Machine machine(syntheticBios());
  machine.powerOn();
  EXPECT_EQ(peek(machine, 0xC000), 0x00);
  EXPECT_EQ(peek(machine, 0x8000), 0x00);
  EXPECT_EQ(peek(machine, 0xFFFC), 0x00);
  EXPECT_EQ(peek(machine, 0xFFFD), 0xC1);
}

TEST(Sb97Machine, WritingFiveThousandSelectsThePageAtEightThousandAndTheTopStaysFixed) {
  Sb97Machine machine(syntheticBios());
  machine.powerOn();
  poke(machine, 0x5000, 5);
  EXPECT_EQ(peek(machine, 0x8000), 5);
  EXPECT_EQ(peek(machine, 0xBFFF), 0xEA);
  EXPECT_EQ(peek(machine, 0xC000), 0x00);
  poke(machine, 0x5000, 63);
  EXPECT_EQ(peek(machine, 0x8000), 63);
  poke(machine, 0x5000, 0x45);
  EXPECT_EQ(peek(machine, 0x8000), 5) << "页号只取低 6 位";
  EXPECT_EQ(machine.board().registers().programPage, 5);
}

TEST(Sb97Machine, WorkRamIsEightKilobytesAtSixThousand) {
  Sb97Machine machine(syntheticBios());
  machine.powerOn();
  poke(machine, 0x6000, 0x12);
  poke(machine, 0x7FFF, 0x34);
  EXPECT_EQ(peek(machine, 0x6000), 0x12);
  EXPECT_EQ(peek(machine, 0x7FFF), 0x34);
  EXPECT_EQ(machine.board().ram()[0], 0x12);
  EXPECT_EQ(machine.board().ram()[0x1FFF], 0x34);
}

TEST(Sb97Machine, UnmappedAddressesBelowSixThousandDoNotDriveTheBus) {
  Sb97Machine machine(syntheticBios());
  machine.powerOn();
  const hw::AddressTarget target = machine.debugger()->resolve("cpu", 0x4800);
  EXPECT_EQ(target.kind, "none");
  EXPECT_EQ(machine.debugger()->resolve("cpu", 0x5000).kind, "register");
  EXPECT_EQ(machine.debugger()->resolve("cpu", 0x6000).kind, "ram");
}

TEST(Sb97Machine, SoundDataPortAlwaysReportsReady) {
  Sb97Machine machine(syntheticBios());
  machine.powerOn();
  EXPECT_EQ(peek(machine, 0x5300), 0x80);
}

TEST(Sb97Machine, FloppyControllerRegistersAreAtFiveFiveZeroOneAndFiveSixZeroFour) {
  Sb97Machine machine(syntheticBios());
  machine.powerOn();
  poke(machine, 0x5501, 0x18);
  poke(machine, 0x5501, 0x1C);
  EXPECT_EQ(peek(machine, 0x5604) & 0xC0, 0x80) << "主状态：可以写命令";
  EXPECT_EQ(machine.debugger()->resolve("cpu", 0x5605).kind, "register");
}

TEST(Sb97Machine, CharacterRamIsAtZeroAndTheFourNametablesAreSeparate) {
  Sb97Machine machine(syntheticBios());
  machine.powerOn();
  pokeVideo(machine, 0x0010, 0x77);
  EXPECT_EQ(peekVideo(machine, 0x0010), 0x77);
  pokeVideo(machine, 0x2005, 0x11);
  pokeVideo(machine, 0x2405, 0x22);
  pokeVideo(machine, 0x2805, 0x33);
  pokeVideo(machine, 0x2C05, 0x44);
  EXPECT_EQ(peekVideo(machine, 0x2005), 0x11);
  EXPECT_EQ(peekVideo(machine, 0x2405), 0x22);
  EXPECT_EQ(peekVideo(machine, 0x2805), 0x33);
  EXPECT_EQ(peekVideo(machine, 0x2C05), 0x44);
  EXPECT_EQ(peekVideo(machine, 0x3005), 0x11) << "$3000-$3EFF 是 $2000-$2EFF 的镜像";
  EXPECT_EQ(machine.board().nametable()[5], 0x11);
  EXPECT_EQ(machine.board().nametable()[3 * 1024 + 5], 0x44);
}

TEST(Sb97Machine, ProgramRunsFromTheResetVectorAndTheFrameIsDendy) {
  Sb97Machine machine(syntheticBios());
  machine.powerOn();
  machine.runFrame();
  EXPECT_NE(machine.peekCpuBus(0x00), 0) << "CPU 从复位向量开始执行并计数";
  EXPECT_NEAR(machine.framesPerSecond(), 50.0, 0.1);
  EXPECT_FALSE(machine.hasMouse());
  EXPECT_TRUE(machine.keyboardIsMachineInput());
  EXPECT_EQ(machine.padPortCount(), 1U);
}

// ---------------------------------------------------------------- 真实 BIOS

/** 记录马达开关的次数。 */
struct MotorLog final : hw::FloppyActivity {
  int turnedOn{0};
  void motorChanged(unsigned /*drive*/, bool on) override {
    if (on) ++turnedOn;
  }
};

/** 画面里白色像素的个数：DOS 文字多起来，这个数就大。 */
std::size_t whitePixels(const hw::FrameBuffer& picture) {
  return static_cast<std::size_t>(std::count_if(picture.pixels.begin(), picture.pixels.end(), [](std::uint32_t pixel) { return (pixel & 0xFFFFFFU) == 0xFFFFFFU; }));
}

/** 读仓库 reference/floppy-images/sb97 里的盘片镜像；文件不存在时返回空。 */
std::vector<std::uint8_t> realFloppy(const char* name) {
  const std::filesystem::path path = std::filesystem::path(EMBER_TEST_DATA_DIR) / ".." / ".." / ".." / ".." / "reference" / "floppy-images" / "sb97" / name;
  std::ifstream stream(path, std::ios::binary);
  if (!stream) return {};
  return std::vector<std::uint8_t>((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
}

/** 按一下键：按下与松开各保持 8 帧。 */
void tap(Sb97Machine& machine, const char* code) {
  machine.input(hw::KeyInput{code, true});
  for (int frame = 0; frame < 8; ++frame) machine.runFrame();
  machine.input(hw::KeyInput{code, false});
  for (int frame = 0; frame < 8; ++frame) machine.runFrame();
}

void run(Sb97Machine& machine, int frames) {
  for (int frame = 0; frame < frames; ++frame) machine.runFrame();
}

TEST(Sb97Machine, RealBiosBootsToTheMenu) {
  std::vector<std::uint8_t> bios = realBios();
  if (bios.empty()) GTEST_SKIP() << "没有找到 reference/rom/小霸王/SB97.nes";
  Sb97Machine machine(std::move(bios));
  machine.powerOn();
  const std::string dump = dumpPath();
  int frames = 0;
  // 先是小霸王的开机画面，再检测硬件，最后是菜单。
  for (const int checkpoint : {600, 2400, 4200}) {
    run(machine, checkpoint - frames);
    frames = checkpoint;
    const hw::FrameBuffer picture = machine.frame();
    ASSERT_EQ(picture.width, 256U);
    ASSERT_EQ(picture.height, 240U);
    std::set<std::uint32_t> colors(picture.pixels.begin(), picture.pixels.end());
    EXPECT_GT(colors.size(), 1U) << "BIOS 开机后画面不应该是纯色，帧数 " << checkpoint;
    if (!dump.empty()) dumpFrame(picture, dump + ".boot" + std::to_string(checkpoint) + ".ppm");
  }
  // 菜单有 6 个八边形的按钮，白色像素很多。
  EXPECT_GT(whitePixels(machine.frame()), 3000U) << "菜单没有显示出来";
}

TEST(Sb97Machine, RealBiosDosListsTheFilesOnTheFloppyDisk) {
  std::vector<std::uint8_t> bios = realBios();
  std::vector<std::uint8_t> image = realFloppy("1.IMA");
  if (bios.empty() || image.empty()) GTEST_SKIP() << "没有找到 reference/rom/小霸王/SB97.nes 或 reference/floppy-images/sb97/1.IMA";
  MotorLog motors;
  Sb97Machine machine(std::move(bios), &motors);
  machine.floppyDrive(0)->insertImage(std::move(image), false, "1.IMA");
  machine.powerOn();
  run(machine, 4200);
  // 菜单上初始选中“指法训练”，按一次左键移到 DOS，回车进入。
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
  if (const std::string dump = dumpPath(); !dump.empty()) dumpFrame(machine.frame(), dump + ".dos-dir.ppm");
}

}  // namespace
