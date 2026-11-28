/*
 * ------------------------------------------------------------------------
 * 名称：yuxing-test.cpp
 * 说明：验证裕兴学习机（yuxing）：四个学习卡的真实 ROM 开机；键盘移动菜单选择；鼠标移动光标；软驱读真实的软盘镜像；声像学习放真实的声像磁带出现课文画面；V9.0-98 的系统检测菜单里没有多余的字模；V8.3-D 按小容量型号运行。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：真实 ROM 在仓库的 ember/roms（没有文件头的纯 ROM），软盘镜像与声像磁带在 reference，缺它们时对应用例跳过；设置环境变量 EMBER_REAL_ROM_DUMP 为目录时，把关键时刻的画面存成 PPM 供人工查看；
 *       声像磁带要播约 22 秒的磁带时间（模拟时间，不是真实时间），所以分段运行，避开单次等待 10 秒的限制；
 *       画面只做结构判断（两个画面是否不同、有内容的像素个数），不比较整幅像素；菜单的按键间隔取 8 帧按下、30 帧等待，比这短时菜单偶尔漏键。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "../support/real-rom-support.hpp"
#include "contracts/hardware/tape-deck.hpp"
#include "infra/media/disk-tape-store.hpp"


namespace {

namespace hw = ember::contracts::hardware;
namespace rt = ember::contracts::runtime;
using ember::test::readMachineRom;
using ember::test::RealRomBench;

/** 分段运行的帧数。 */
constexpr std::uint64_t CHUNK_FRAMES = 450;

/** 与左上角像素颜色不同的像素个数（画面上有没有内容）。 */
unsigned contentPixels(const rt::FrameImage& image) {
  unsigned total = 0;
  for (std::size_t index = 4; index + 3 < image.rgba.size(); index += 4) {
    if (!std::equal(image.rgba.begin(), image.rgba.begin() + 3, image.rgba.begin() + static_cast<std::ptrdiff_t>(index))) ++total;
  }
  return total;
}

/** 两个画面里颜色不同的像素个数。 */
std::size_t differingPixels(const rt::FrameImage& first, const rt::FrameImage& second) {
  std::size_t total = 0;
  for (std::size_t index = 0; index + 3 < first.rgba.size() && index + 3 < second.rgba.size(); index += 4) {
    if (!std::equal(first.rgba.begin() + static_cast<std::ptrdiff_t>(index), first.rgba.begin() + static_cast<std::ptrdiff_t>(index) + 3, second.rgba.begin() + static_cast<std::ptrdiff_t>(index))) ++total;
  }
  return total;
}

/** 分段运行 frames 帧；任何一段没跑完就返回 false。 */
bool runLong(RealRomBench& bench, std::uint64_t frames) {
  for (std::uint64_t done = 0; done < frames; done += CHUNK_FRAMES) {
    if (!bench.run(std::min(CHUNK_FRAMES, frames - done))) return false;
  }
  return true;
}

/** 菜单里的一次按键。 */
void menuKey(RealRomBench& bench, const char* code) { bench.tap(code, 8, 30); }

/** 四个学习卡的 ROM 文件。 */
class YuxingRoms : public ::testing::TestWithParam<const char*> {};

TEST_P(YuxingRoms, BootsToAScreenWithContentAndNoErrors) {
  const std::string file = GetParam();
  const auto rom = readMachineRom(file);
  if (rom.empty()) GTEST_SKIP() << "没有找到 ember/roms/" << file;
  RealRomBench bench;
  bench.bootMachine("yuxing", rom);
  ASSERT_TRUE(bench.run(600));
  bench.dump(file + "-0-boot");
  EXPECT_TRUE(bench.errors->entries.empty());
  EXPECT_EQ(bench.runtime->status().machine, "yuxing");
  EXPECT_GT(contentPixels(bench.runtime->captureFrame()), 1000U);
}

INSTANTIATE_TEST_SUITE_P(Roms, YuxingRoms, ::testing::Values("yuxing_v8.0-c.rom", "yuxing_v8.2-d.rom", "yuxing_v8.3-d.rom", "yuxing_v9.0-98.rom"));

TEST(Yuxing, TheKeyboardBelongsToTheMachineAndTheMachineHasAMouse) {
  const auto rom = readMachineRom("yuxing_v9.0-98.rom");
  if (rom.empty()) GTEST_SKIP() << "没有找到 ember/roms/yuxing_v9.0-98.rom";
  RealRomBench bench;
  bench.bootMachine("yuxing", rom);
  ASSERT_TRUE(bench.run(2));
  bool machineInput = false;
  bool mouse = false;
  unsigned pads = 0;
  bench.runtime->useMachine([&](hw::Machine& machine) {
    machineInput = machine.keyboardIsMachineInput();
    mouse = machine.hasMouse();
    pads = machine.padPortCount();
  });
  EXPECT_TRUE(machineInput);
  EXPECT_TRUE(mouse);
  EXPECT_EQ(pads, 2U);
}

TEST(Yuxing, TheArrowKeysMoveTheSelectionOfTheMainMenu) {
  const auto rom = readMachineRom("yuxing_v9.0-98.rom");
  if (rom.empty()) GTEST_SKIP() << "没有找到 ember/roms/yuxing_v9.0-98.rom";
  RealRomBench bench;
  bench.bootMachine("yuxing", rom);
  ASSERT_TRUE(bench.run(600));
  const auto idle = bench.runtime->captureFrame();
  menuKey(bench, "ArrowRight");
  const auto first = bench.runtime->captureFrame();
  bench.dump("keyboard-1-first");
  menuKey(bench, "ArrowDown");
  const auto second = bench.runtime->captureFrame();
  bench.dump("keyboard-2-second");
  EXPECT_NE(differingPixels(idle, first), 0U) << "第一次按键唤出光标并选中第一项";
  EXPECT_GT(differingPixels(first, second), 100U) << "向下键把选中项移到下一项";
  EXPECT_TRUE(bench.errors->entries.empty());
}

TEST(Yuxing, MouseMovementMovesTheCursorOfTheMainMenu) {
  const auto rom = readMachineRom("yuxing_v9.0-98.rom");
  if (rom.empty()) GTEST_SKIP() << "没有找到 ember/roms/yuxing_v9.0-98.rom";
  RealRomBench bench;
  bench.bootMachine("yuxing", rom);
  ASSERT_TRUE(bench.run(600));
  menuKey(bench, "ArrowRight");
  const auto before = bench.runtime->captureFrame();
  for (int step = 0; step < 6; ++step) {
    hw::MouseInput move;
    move.dx = 12;
    move.dy = -10;
    bench.runtime->sendInput(move);
    ASSERT_TRUE(bench.run(3));
  }
  bench.dump("mouse-moved");
  const auto after = bench.runtime->captureFrame();
  EXPECT_NE(differingPixels(before, after), 0U) << "鼠标移动之后光标的位置变了";
  EXPECT_TRUE(bench.errors->entries.empty());
}

TEST(Yuxing, TheFloppyDriveReadsARealDiskInTheDiskManager) {
  const auto rom = readMachineRom("yuxing_v9.0-98.rom");
  const auto disk = ember::test::readReferenceFile("floppy-images/裕兴/工具软件/磁盘处理(98-F).IMG");
  if (rom.empty() || disk.empty()) GTEST_SKIP() << "没有找到 ember/roms/yuxing_v9.0-98.rom 或软盘镜像";
  // 同样的操作：进入“磁盘管理”，按回车列目录；有盘时列出文件，没有盘时报“磁盘格式错”。
  const auto listing = [&](bool withDisk) {
    RealRomBench bench;
    bench.bootMachine("yuxing", rom);
    EXPECT_TRUE(bench.run(600));
    if (withDisk) bench.runtime->useMachine([&](hw::Machine& machine) { machine.floppyDrive(0)->insertImage(disk, false, "disk.img"); });
    menuKey(bench, "ArrowRight");
    menuKey(bench, "ArrowRight");
    bench.tap("Enter", 8, 200);
    bench.tap("Enter", 8, 400);
    bench.dump(withDisk ? "floppy-with-disk" : "floppy-without-disk");
    EXPECT_TRUE(bench.errors->entries.empty());
    return bench.runtime->captureFrame();
  };
  const auto withDisk = listing(true);
  const auto withoutDisk = listing(false);
  EXPECT_GT(differingPixels(withDisk, withoutDisk), 500U) << "有盘时列出文件目录，没有盘时报错";
}

TEST(Yuxing, TheDiskOperationScreenOfV83HasNoWhiteBlockInTheTopLeftCorner) {
  const auto rom = readMachineRom("yuxing_v8.3-d.rom");
  if (rom.empty()) GTEST_SKIP() << "没有找到 ember/roms/yuxing_v8.3-d.rom";
  RealRomBench bench;
  bench.bootMachine("yuxing", rom);
  ASSERT_TRUE(bench.run(600));
  menuKey(bench, "ArrowRight");
  bench.tap("Enter", 8, 200);
  bench.dump("disk-operation-v83");
  // 汉字单色模式下程序在左上角放了 16 个不可见的精灵（图块 $F0、在背景之后）：精灵取色后移 2 项，颜色 2 落到背景色上。
  const auto image = bench.runtime->captureFrame();
  ASSERT_GT(image.width, 64U);
  const auto pixelAt = [&](std::uint32_t x, std::uint32_t y) { return std::vector<std::uint8_t>(image.rgba.begin() + (y * image.width + x) * 4, image.rgba.begin() + (y * image.width + x) * 4 + 4); };
  const auto background = pixelAt(image.width - 2, 2);
  for (std::uint32_t y = 0; y < 16; ++y) {
    for (std::uint32_t x = 0; x < 64; ++x) ASSERT_EQ(pixelAt(x, y), background) << "(" << x << ", " << y << ") 不是背景色";
  }
  EXPECT_TRUE(bench.errors->entries.empty());
}

TEST(Yuxing, RealSoundAndPictureTapeDrivesTheLessonScreenOfTheSoundAndPictureCard) {
  const auto rom = readMachineRom("yuxing_v8.0-c.rom");
  const auto tape = ember::test::firstTape();
  if (rom.empty() || tape.empty()) GTEST_SKIP() << "没有找到 ember/roms/yuxing_v8.0-c.rom 或声像磁带";
  RealRomBench bench;
  bench.bootMachine("yuxing", rom);
  ASSERT_TRUE(bench.run(600));
  // 主菜单第 5 项“声像学习”：进入声像卡的画面，等磁带送数据。
  for (int index = 0; index < 4; ++index) menuKey(bench, "ArrowDown");
  bench.tap("Enter", 8, 300);
  bench.dump("sound-picture-0-logo");
  const auto logo = bench.runtime->captureFrame();

  ember::infra::media::DiskTapeStore store(nullptr);
  const auto tapePath = tape.u8string();
  auto source = store.open(std::string(tapePath.begin(), tapePath.end()));
  EXPECT_TRUE(source->stereo());
  bench.runtime->useMachine([&](hw::Machine& machine) {
    machine.tape()->load(std::move(source), "tape");
    machine.tape()->play();
  });
  ASSERT_TRUE(runLong(bench, 1350));
  bench.dump("sound-picture-1-lesson");
  EXPECT_GT(differingPixels(logo, bench.runtime->captureFrame()), 10000U) << "磁带的数据线驱动了程序，画面应该从声像卡的标志换成课文";
  EXPECT_TRUE(bench.errors->entries.empty());
}

}  // namespace

TEST(Yuxing, TheSystemCheckMenuLeavesNoStrayTileInTheMessageRow) {
  const auto rom = readMachineRom("yuxing_v9.0-98.rom");
  if (rom.empty()) GTEST_SKIP() << "没有找到 ember/roms/yuxing_v9.0-98.rom";
  RealRomBench bench;
  bench.bootMachine("yuxing", rom);
  ASSERT_TRUE(bench.run(600));
  menuKey(bench, "ArrowRight");
  menuKey(bench, "ArrowRight");
  menuKey(bench, "ArrowDown");
  menuKey(bench, "ArrowDown");
  menuKey(bench, "ArrowDown");
  bench.tap("Enter", 8, 200);
  bench.dump("system-check-menu");
  // 程序在画面渲染期间用 $2006、$2007 写调色板，此时 PPU 地址已被渲染推到 $3B02：这样的写入要丢弃，不能把 $16 写进名称表第 24 行第 2 列。
  std::array<std::uint8_t, 1> tile{};
  bench.runtime->useMachine([&](hw::Machine& machine) { machine.debugger()->readMemory("board.nametable", 0x302, tile); });
  EXPECT_EQ(tile[0], 0x81);
  EXPECT_TRUE(bench.errors->entries.empty());
}

TEST(Yuxing, TheV83CardRunsWithTheSmallMemoryOfItsRevision) {
  const auto rom = readMachineRom("yuxing_v8.3-d.rom");
  if (rom.empty()) GTEST_SKIP() << "没有找到 ember/roms/yuxing_v8.3-d.rom";
  RealRomBench bench;
  bench.bootMachine("yuxing", rom);
  ASSERT_TRUE(bench.run(2));
  // V8.3-D 的程序 RAM 只有 128K：写入页寄存器的页号只取低 3 位。
  std::int64_t pageRegister = -1;
  bench.runtime->useMachine([&](hw::Machine& machine) {
    const std::array<std::uint8_t, 1> programRamMode{0x04};
    const std::array<std::uint8_t, 1> pageRegisterMode{0x20};
    const std::array<std::uint8_t, 1> page{0x3F};
    machine.debugger()->writeMemory("cpu", 0x5500, programRamMode);
    machine.debugger()->writeMemory("cpu", 0x4800, pageRegisterMode);
    machine.debugger()->writeMemory("cpu", 0x8000, page);
    for (const auto& field : machine.debugger()->readState("board")) {
      if (field.name == "pageRegister") pageRegister = field.value;
    }
  });
  EXPECT_EQ(pageRegister, 0x07);
}
