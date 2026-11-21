/*
 * ------------------------------------------------------------------------
 * 名称：pyramid-machine-test.cpp
 * 说明：主机型号 pec586 的精灵取色：汉字模式下精灵颜色整体后移 2 项；关闭汉字模式时照常取色。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：ROM 全部是 NOP，复位后 CPU 在里面空转；颜色不写死，用同一台机器关闭渲染时输出的背景色取得。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

#include "bootstrap/machines/pyramid/pyramid.hpp"

namespace {

namespace hw = ember::contracts::hardware;
using ember::bootstrap::PyramidBoard;
using ember::bootstrap::PyramidMachine;

constexpr std::uint8_t NOP = 0xEA;
/** 精灵 0 的位置：屏幕第 10 行的下一行开始显示，左边第 40 列。 */
constexpr std::uint8_t SPRITE_Y = 10;
constexpr std::uint8_t SPRITE_X = 40;
constexpr std::uint32_t SHOWN_ROW = SPRITE_Y + 1U;
constexpr std::uint32_t SHOWN_COLUMN = SPRITE_X;
constexpr std::uint8_t SHOW_SPRITES = 0x14;
constexpr std::uint8_t TEXT_MODE = 0x80;
constexpr std::uint16_t REGISTER_ZERO = 0x5000;

void pokeByte(PyramidMachine& machine, std::string_view space, std::uint32_t address, std::uint8_t value) {
  const std::array<std::uint8_t, 1> bytes{value};
  machine.debugger()->writeMemory(space, address, bytes);
}

/** 画面 (x, y) 的像素。 */
std::uint32_t pixelAt(const hw::FrameBuffer& frame, std::uint32_t x, std::uint32_t y) { return frame.pixels[static_cast<std::size_t>(y) * frame.width + x]; }

/** 运行两帧，让最新的设置出现在已完成的画面里。 */
void runTwoFrames(PyramidMachine& machine) {
  machine.runFrame();
  machine.runFrame();
}

/** 关闭渲染、把背景色设成 entry 后的画面左上角颜色，作为这个颜色号的参照。 */
std::uint32_t backdropColor(PyramidMachine& machine, std::uint8_t entry) {
  pokeByte(machine, "ppu", 0x3F00, entry);
  pokeByte(machine, "cpu", 0x2001, 0x00);
  runTwoFrames(machine);
  return pixelAt(machine.frame(), 0, 0);
}

/** 上电后的 pec586：字符 RAM 的 1 号图块是实心的颜色 1，精灵 0 用它，调色板 0 的颜色 1 是 $3F11，颜色 3 是 $3F13。 */
struct SpriteRig {
  SpriteRig() : machine(PyramidBoard::Model::Pec586, std::vector<std::uint8_t>(PyramidBoard::ROM_SIZE, NOP)) {
    machine.powerOn();
    constexpr std::uint32_t TILE_ONE = 16;
    for (std::uint32_t plane0 = 0; plane0 < 8; ++plane0) pokeByte(machine, "board.characterRam", TILE_ONE + plane0, 0xFF);
    const std::array<std::uint8_t, 4> sprite{SPRITE_Y, 1, 0, SPRITE_X};
    machine.debugger()->writeMemory("chip.oam", 0, sprite);
    pokeByte(machine, "ppu", 0x3F00, 0x0F);
    pokeByte(machine, "ppu", 0x3F11, 0x16);
    pokeByte(machine, "ppu", 0x3F13, 0x12);
  }

  PyramidMachine machine;
};

TEST(PyramidMachine, SpriteColorOneIsTheFirstEntryOfSpritePaletteZeroWhenTextModeIsOff) {
  SpriteRig rig;
  const std::uint32_t red = backdropColor(rig.machine, 0x16);
  const std::uint32_t blue = backdropColor(rig.machine, 0x12);
  ASSERT_NE(red, blue);
  pokeByte(rig.machine, "ppu", 0x3F00, 0x0F);
  pokeByte(rig.machine, "cpu", 0x2001, SHOW_SPRITES);
  runTwoFrames(rig.machine);
  EXPECT_FALSE(rig.machine.board().textMode());
  EXPECT_EQ(pixelAt(rig.machine.frame(), SHOWN_COLUMN, SHOWN_ROW), red);
}

TEST(PyramidMachine, TextModeMovesTheSpriteColorsByTwoEntries) {
  SpriteRig rig;
  const std::uint32_t red = backdropColor(rig.machine, 0x16);
  const std::uint32_t blue = backdropColor(rig.machine, 0x12);
  pokeByte(rig.machine, "ppu", 0x3F00, 0x0F);
  pokeByte(rig.machine, "cpu", 0x2001, SHOW_SPRITES);
  pokeByte(rig.machine, "cpu", REGISTER_ZERO, TEXT_MODE);
  runTwoFrames(rig.machine);
  ASSERT_TRUE(rig.machine.board().textMode());
  EXPECT_EQ(pixelAt(rig.machine.frame(), SHOWN_COLUMN, SHOWN_ROW), blue) << "颜色 1 取调色板第 2 + 1 项，即 $3F13";
  EXPECT_NE(pixelAt(rig.machine.frame(), SHOWN_COLUMN, SHOWN_ROW), red);
}

}  // namespace
