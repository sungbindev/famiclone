/*
 * ------------------------------------------------------------------------
 * 名称：pyramid-test.cpp
 * 说明：验证金字塔学习机（pec9588、pec586）：真实 ROM 开机、程序在复制到 RAM 里的代码上运行、主机自带的键盘经 $4016-$4017 送出按键、汉字模式下菜单画面能读；PEC-586 与 PEC-686F 两个 ROM 都能开机。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：真实 ROM 在仓库的 ember/roms（没有文件头的纯 ROM），缺它时对应用例跳过；设置环境变量 EMBER_REAL_ROM_DUMP 为目录时，把关键时刻的画面存成 PPM 供人工查看。
 *       画面只做结构判断（某几行是否有字模的亮点、两个画面是否不同），不比较整幅像素。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "../support/real-rom-support.hpp"

namespace {

namespace hw = ember::contracts::hardware;
namespace rt = ember::contracts::runtime;
using ember::test::readMachineRom;
using ember::test::RealRomBench;

/** 9588 复位后先把块 29 里的 4K 程序复制到 $6000-$7FFF 的 RAM 里，再跳到 $7000 运行。 */
constexpr std::uint16_t RAM_PROGRAM = 0x7000;
/** 9588 的程序把写给 $4100-$5FFF 的 reg0 记在 $03C8：位 7 为 1 时是汉字模式。 */
constexpr std::uint16_t SHADOW_REGISTER = 0x03C8;
constexpr std::uint8_t TEXT_MODE = 0x80;

/** 画面第 first 行起 count 行里亮点（红绿蓝之和很大的像素）的个数。 */
unsigned brightPixels(const rt::FrameImage& image, unsigned first, unsigned count) {
  unsigned total = 0;
  for (unsigned y = first; y < first + count && y < image.height; ++y) {
    for (unsigned x = 0; x < image.width; ++x) {
      const std::size_t index = (static_cast<std::size_t>(y) * image.width + x) * 4;
      if (static_cast<unsigned>(image.rgba[index]) + image.rgba[index + 1] + image.rgba[index + 2] > 600) ++total;
    }
  }
  return total;
}

/** 与左上角像素颜色不同的像素个数（画面上有没有内容）。 */
unsigned contentPixels(const rt::FrameImage& image) {
  unsigned total = 0;
  for (std::size_t index = 4; index + 3 < image.rgba.size(); index += 4) {
    if (!std::equal(image.rgba.begin(), image.rgba.begin() + 3, image.rgba.begin() + static_cast<std::ptrdiff_t>(index))) ++total;
  }
  return total;
}

/** 9588 的程序写给 $5000 的值的影子。 */
std::uint8_t shadowRegister(RealRomBench& bench) {
  std::uint8_t value = 0;
  bench.runtime->useMachine([&](hw::Machine& machine) { machine.debugger()->readMemory("cpu", SHADOW_REGISTER, std::span<std::uint8_t>(&value, 1)); });
  return value;
}

TEST(Pyramid, Pec9588BootsAndRunsFromTheCopiedRamProgram) {
  const auto rom = readMachineRom("pec9588.bin");
  if (rom.empty()) GTEST_SKIP() << "没有找到 ember/roms/pec9588.bin";
  RealRomBench bench;
  bench.bootMachine("pec9588", rom);
  ASSERT_TRUE(bench.run(120));
  bench.dump("pec9588-0-boot");
  EXPECT_TRUE(bench.errors->entries.empty());
  EXPECT_EQ(bench.runtime->status().machine, "pec9588");
  std::vector<std::uint8_t> head(3);
  std::uint16_t pc = 0;
  bench.runtime->useMachine([&](hw::Machine& machine) {
    machine.debugger()->readMemory("cpu", RAM_PROGRAM, std::span<std::uint8_t>(head));
    pc = machine.cpuRegisters().pc;
  });
  EXPECT_EQ(head, (std::vector<std::uint8_t>{0x4C, 0x14, 0x70})) << "$7000 是复制过来的程序：JMP $7014";
  EXPECT_GE(pc, 0x7000U);
  EXPECT_LT(pc, 0x8000U) << "开机画面里程序在 RAM 里等按键";
  EXPECT_EQ(shadowRegister(bench) & TEXT_MODE, 0) << "开机画面是图片，不是汉字模式";
}

TEST(Pyramid, AKeyPressLeavesTheSplashAndTheMainMenuIsReadableInTextMode) {
  const auto rom = readMachineRom("pec9588.bin");
  if (rom.empty()) GTEST_SKIP() << "没有找到 ember/roms/pec9588.bin";
  RealRomBench bench;
  bench.bootMachine("pec9588", rom);
  ASSERT_TRUE(bench.run(300));
  const auto splash = bench.runtime->captureFrame();
  bench.tap("Enter", 8, 240);
  bench.dump("pec9588-1-menu");
  const auto menu = bench.runtime->captureFrame();
  EXPECT_FALSE(ember::test::sameFrame(splash, menu)) << "按键之后画面应该离开开机画面";
  EXPECT_NE(shadowRegister(bench) & TEXT_MODE, 0) << "主菜单用汉字模式显示";
  EXPECT_GT(brightPixels(menu, 32, 16), 100U) << "标题“系统主菜单”所在的行应该有字模的亮点";
  EXPECT_GT(brightPixels(menu, 64, 64), 300U) << "八个菜单项所在的行应该有字模的亮点";
  EXPECT_TRUE(bench.errors->entries.empty());
}

TEST(Pyramid, TheMenuKeysStartDifferentPrograms) {
  const auto rom = readMachineRom("pec9588.bin");
  if (rom.empty()) GTEST_SKIP() << "没有找到 ember/roms/pec9588.bin";
  std::vector<rt::FrameImage> screens;
  for (const char* key : {"Digit1", "Digit4", "Digit5", "Digit7", "Digit8"}) {
    RealRomBench bench;
    bench.bootMachine("pec9588", rom);
    ASSERT_TRUE(bench.run(300));
    bench.tap("Enter", 8, 200);
    bench.tap(key, 8, 300);
    bench.dump(std::string("pec9588-menu-") + key);
    EXPECT_TRUE(bench.errors->entries.empty()) << key;
    screens.push_back(bench.runtime->captureFrame());
    EXPECT_GT(contentPixels(screens.back()), 10U) << key << " 之后画面上应该有内容";
  }
  for (std::size_t first = 0; first < screens.size(); ++first) {
    for (std::size_t second = first + 1; second < screens.size(); ++second) EXPECT_FALSE(ember::test::sameFrame(screens[first], screens[second])) << first << " 与 " << second;
  }
}

TEST(Pyramid, TheKeyboardBelongsToTheMachineAndTheMachineHasNoMouse) {
  const auto rom = readMachineRom("pec9588.bin");
  if (rom.empty()) GTEST_SKIP() << "没有找到 ember/roms/pec9588.bin";
  RealRomBench bench;
  bench.bootMachine("pec9588", rom);
  ASSERT_TRUE(bench.run(2));
  bool machineInput = false;
  bool mouse = true;
  bench.runtime->useMachine([&](hw::Machine& machine) {
    machine.connectFamilyKeyboard(false);
    machineInput = machine.keyboardIsMachineInput();
    mouse = machine.hasMouse();
  });
  EXPECT_TRUE(machineInput);
  EXPECT_FALSE(mouse);
}

/** PEC-586 与 PEC-686F 是同一个机型 pec586 的两个 ROM。 */
class Pec586Roms : public ::testing::TestWithParam<const char*> {};

TEST_P(Pec586Roms, BootToAPictureWithoutErrors) {
  const std::string file = GetParam();
  const auto rom = readMachineRom(file);
  if (rom.empty()) GTEST_SKIP() << "没有找到 ember/roms/" << file;
  RealRomBench bench;
  bench.bootMachine("pec586", rom);
  ASSERT_TRUE(bench.run(300));
  bench.dump(std::string("pec586-") + file + "-0-boot");
  EXPECT_TRUE(bench.errors->entries.empty());
  EXPECT_EQ(bench.runtime->status().machine, "pec586");
  EXPECT_GT(contentPixels(bench.runtime->captureFrame()), 500U) << "开机之后画面上应该有内容";
}

TEST_P(Pec586Roms, AKeyPressLeavesTheSplashForAMenuInTextMode) {
  const std::string file = GetParam();
  const auto rom = readMachineRom(file);
  if (rom.empty()) GTEST_SKIP() << "没有找到 ember/roms/" << file;
  RealRomBench bench;
  bench.bootMachine("pec586", rom);
  ASSERT_TRUE(bench.run(300));
  const auto splash = bench.runtime->captureFrame();
  bench.tap("Space", 8, 240);
  bench.dump(std::string("pec586-") + file + "-1-menu");
  const auto menu = bench.runtime->captureFrame();
  EXPECT_FALSE(ember::test::sameFrame(splash, menu)) << "按键之后画面应该离开开机画面";
  EXPECT_GT(brightPixels(menu, 0, menu.height), 300U) << "菜单上应该有字模的亮点";
  EXPECT_TRUE(bench.errors->entries.empty());
}

INSTANTIATE_TEST_SUITE_P(Pyramid, Pec586Roms, ::testing::Values("pec586.bin", "pec686f.bin"));

}  // namespace
