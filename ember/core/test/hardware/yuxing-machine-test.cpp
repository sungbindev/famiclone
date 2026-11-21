/*
 * ------------------------------------------------------------------------
 * 名称：yuxing-machine-test.cpp
 * 说明：主机型号 yuxing（裕兴学习机）的测试：ROM 大小、ROM 模式与程序 RAM 模式的窗口、页寄存器、32K 页、$6000 窗口、名称表镜像、字符页、MMC3 模式、汉字单色模式与 4 分屏的取指、键盘与语音数据口、软驱寄存器、手柄 1 与 2、磁带口、输入与调试接口。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：存储译码用合成的 ROM 检查：每个 32K 页的第一个字节写页号（第 0 页写 $A5），复位向量指向第 0 页里的原地跳转；读手柄的用例用一小段机器码，
 *       把 $4016、$4017 读到的位存进内部 RAM；真实 ROM 的开机、键盘、鼠标、软驱与磁带在 core/test/runtime/yuxing-test.cpp。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <vector>

#include "bootstrap/machines/yuxing/yuxing.hpp"
#include "contracts/hardware/input-event.hpp"
#include "shared/errors/core-error.hpp"

namespace {

namespace hw = ember::contracts::hardware;
using ember::bootstrap::YuxingBoard;
using ember::bootstrap::YuxingMachine;

constexpr std::uint8_t PAGE_ZERO_MARK = 0xA5;
constexpr std::size_t BANK_SIZE = 32 * 1024;
/** 复位后运行的小程序放在第 0 页的 $FF00（ROM 偏移 $7F00）。 */
constexpr std::size_t PROGRAM_OFFSET = 0x7F00;

/** 合成的 ROM：其余字节是 NOP，每个 32K 页的第一个字节标页号，第 0 页里 $FF00 是程序，三个向量都指向它。 */
std::vector<std::uint8_t> syntheticRom(const std::vector<std::uint8_t>& program = {0x4C, 0x00, 0xFF}) {
  std::vector<std::uint8_t> rom(YuxingBoard::ROM_SIZE, 0xEA);
  for (std::size_t bank = 0; bank < YuxingBoard::ROM_SIZE / BANK_SIZE; ++bank) rom[bank * BANK_SIZE] = bank == 0 ? PAGE_ZERO_MARK : static_cast<std::uint8_t>(bank);
  for (std::size_t index = 0; index < program.size(); ++index) rom[PROGRAM_OFFSET + index] = program[index];
  for (std::size_t vector = 0; vector < 3; ++vector) {
    rom[BANK_SIZE - 6 + vector * 2] = 0x00;
    rom[BANK_SIZE - 6 + vector * 2 + 1] = 0xFF;
  }
  return rom;
}

/** 经真实总线写一个字节（与 CPU 写一样有副作用）。 */
void poke(YuxingMachine& machine, std::uint16_t address, std::uint8_t value) {
  const std::array<std::uint8_t, 1> bytes{value};
  machine.debugger()->writeMemory("cpu", address, bytes);
}

std::uint8_t peek(YuxingMachine& machine, std::uint16_t address) { return machine.peekCpuBus(address); }

std::uint8_t readPpu(YuxingMachine& machine, std::uint32_t address) {
  std::array<std::uint8_t, 1> bytes{};
  machine.debugger()->readMemory("ppu", address, bytes);
  return bytes[0];
}

void pokePpu(YuxingMachine& machine, std::uint32_t address, std::uint8_t value) {
  const std::array<std::uint8_t, 1> bytes{value};
  machine.debugger()->writeMemory("ppu", address, bytes);
}

/** 上电后的主机。 */
struct PoweredMachine {
  explicit PoweredMachine(const std::vector<std::uint8_t>& program = {0x4C, 0x00, 0xFF}) : machine(syntheticRom(program)) { machine.powerOn(); }
  YuxingMachine machine;
};

TEST(YuxingMachine, RejectsARomOfTheWrongSize) {
  EXPECT_THROW(YuxingMachine(std::vector<std::uint8_t>(512 * 1024)), ember::shared::CoreError);
  EXPECT_THROW(YuxingMachine(std::vector<std::uint8_t>(YuxingBoard::ROM_SIZE + 1)), ember::shared::CoreError);
}

TEST(YuxingMachine, TheMachineHasTheDevicesOfAYuxing) {
  PoweredMachine rig;
  EXPECT_TRUE(rig.machine.keyboardIsMachineInput());
  EXPECT_TRUE(rig.machine.hasMouse());
  EXPECT_EQ(rig.machine.padPortCount(), 2U);
  EXPECT_EQ(rig.machine.floppyDriveCount(), 1U);
  EXPECT_NE(rig.machine.floppyDrive(0), nullptr);
  EXPECT_EQ(rig.machine.floppyDrive(1), nullptr);
  EXPECT_NE(rig.machine.tape(), nullptr);
  EXPECT_EQ(rig.machine.printer(), nullptr);
}

// ---------------------------------------------------------------- 程序窗口

TEST(YuxingMachine, AfterPowerOnTheFirstThirtyTwoKilobytesOfTheRomAreInTheWindow) {
  PoweredMachine rig;
  EXPECT_EQ(peek(rig.machine, 0x8000), PAGE_ZERO_MARK);
  EXPECT_EQ(peek(rig.machine, 0xFF00), 0x4C);
}

TEST(YuxingMachine, Reg4800SelectsThe32KilobyteRomPageInRomMode) {
  PoweredMachine rig;
  poke(rig.machine, 0x4800, 5);
  EXPECT_EQ(peek(rig.machine, 0x8000), 5);
  poke(rig.machine, 0x4800, 0x25);
  EXPECT_EQ(peek(rig.machine, 0x8000), 5) << "页号只取低 5 位";
  poke(rig.machine, 0x4800, 31);
  EXPECT_EQ(peek(rig.machine, 0x8000), 31);
  // ROM 不可写。
  poke(rig.machine, 0x8000, 0x11);
  EXPECT_EQ(peek(rig.machine, 0x8000), 31);
}

TEST(YuxingMachine, ProgramRamModeMapsWritableRamAndTheLastSixteenKilobytesStayOnTop) {
  PoweredMachine rig;
  poke(rig.machine, 0x5500, 0x04);
  // 页寄存器是 0：$8000 起是程序 RAM 的第 0 个 16K，$C000 起是最后一个 16K（第 63 个）。
  poke(rig.machine, 0x8000, 0x5A);
  poke(rig.machine, 0xBFFF, 0x6B);
  poke(rig.machine, 0xC000, 0x77);
  EXPECT_EQ(peek(rig.machine, 0x8000), 0x5A);
  EXPECT_EQ(peek(rig.machine, 0xBFFF), 0x6B);
  EXPECT_EQ(peek(rig.machine, 0xC000), 0x77);
  EXPECT_EQ(rig.machine.board().programRam()[0], 0x5A);
  EXPECT_EQ(rig.machine.board().programRam()[63 * 16 * 1024], 0x77);
}

TEST(YuxingMachine, WithBitFiveOfReg4800WritesToTheWindowSetThePageRegister) {
  PoweredMachine rig;
  poke(rig.machine, 0x5500, 0x04);
  poke(rig.machine, 0xC000, 0x77);
  poke(rig.machine, 0x4800, 0x20);
  poke(rig.machine, 0x8000, 63);
  EXPECT_EQ(peek(rig.machine, 0x8000), 0x77) << "$8000 起换成第 63 个 16K";
  EXPECT_EQ(rig.machine.board().registers().pageRegister, 63);
  poke(rig.machine, 0x8000, 0xFF);
  EXPECT_EQ(rig.machine.board().registers().pageRegister, 63) << "页寄存器只取低 6 位";
  poke(rig.machine, 0x8000, 2);
  poke(rig.machine, 0x4800, 0x00);
  poke(rig.machine, 0x8000, 0x33);
  EXPECT_EQ(rig.machine.board().programRam()[2 * 16 * 1024], 0x33) << "没有页寄存器模式时写入进程序 RAM";
}

TEST(YuxingMachine, ThirtyTwoKilobytePagesUseHalfThePageRegister) {
  PoweredMachine rig;
  rig.machine.board().programRam()[3 * 32 * 1024] = 0xC3;
  rig.machine.board().programRam()[3 * 32 * 1024 + 0x7FFF] = 0xD4;
  poke(rig.machine, 0x5500, 0x44);
  poke(rig.machine, 0x4800, 0x20);
  poke(rig.machine, 0x8000, 6);
  EXPECT_EQ(peek(rig.machine, 0x8000), 0xC3) << "页寄存器 6 是第 3 个 32K";
  EXPECT_EQ(peek(rig.machine, 0xFFFF), 0xD4);
}

TEST(YuxingMachine, TheWorkWindowAtSixThousandSelectsStaticRamPagesOrTheDefaultProgramRamPage) {
  PoweredMachine rig;
  poke(rig.machine, 0x6000, 0x34);
  EXPECT_EQ(rig.machine.board().programRam()[0x3C * 0x2000], 0x34) << "页选择为 0 时接程序 RAM 的 8K 页 $3C";
  poke(rig.machine, 0x5500, 0x01);
  poke(rig.machine, 0x6000, 0x12);
  EXPECT_EQ(peek(rig.machine, 0x6000), 0x12);
  EXPECT_EQ(rig.machine.board().staticRam()[0x2000], 0x12);
  poke(rig.machine, 0x5500, 0x02);
  EXPECT_EQ(peek(rig.machine, 0x6000), 0) << "另一个 8K 页是空的";
  poke(rig.machine, 0x5500, 0x03);
  poke(rig.machine, 0x7FFF, 0x56);
  EXPECT_EQ(rig.machine.board().staticRam()[0x7FFF], 0x56);
  poke(rig.machine, 0x5500, 0x01);
  EXPECT_EQ(peek(rig.machine, 0x6000), 0x12);
  poke(rig.machine, 0x5500, 0x00);
  EXPECT_EQ(peek(rig.machine, 0x6000), 0x34);
}

// ---------------------------------------------------------------- 视频

TEST(YuxingMachine, Reg5500BitThreeSelectsTheNametableMirroring) {
  PoweredMachine rig;
  // 垂直镜像：$2000 与 $2800 是同一页，$2400 与 $2C00 是另一页。
  pokePpu(rig.machine, 0x2000, 0x11);
  pokePpu(rig.machine, 0x2400, 0x22);
  EXPECT_EQ(readPpu(rig.machine, 0x2800), 0x11);
  EXPECT_EQ(readPpu(rig.machine, 0x2C00), 0x22);
  poke(rig.machine, 0x5500, 0x08);
  // 水平镜像：$2000 与 $2400 是同一页。
  pokePpu(rig.machine, 0x2000, 0x33);
  EXPECT_EQ(readPpu(rig.machine, 0x2400), 0x33);
  pokePpu(rig.machine, 0x2800, 0x44);
  EXPECT_EQ(readPpu(rig.machine, 0x2C00), 0x44);
  EXPECT_NE(readPpu(rig.machine, 0x2000), readPpu(rig.machine, 0x2800));
}

TEST(YuxingMachine, Reg5501SelectsTheEightKilobyteCharacterPage) {
  PoweredMachine rig;
  rig.machine.board().characterRam()[0] = 0x10;
  rig.machine.board().characterRam()[3 * 0x2000 + 5] = 0x35;
  EXPECT_EQ(readPpu(rig.machine, 0x0000), 0x10);
  poke(rig.machine, 0x5501, 3);
  EXPECT_EQ(readPpu(rig.machine, 0x0005), 0x35);
  poke(rig.machine, 0x5501, 0x13);
  EXPECT_EQ(readPpu(rig.machine, 0x0005), 0x35) << "页号只取低 4 位";
  pokePpu(rig.machine, 0x1FFF, 0x99);
  EXPECT_EQ(rig.machine.board().characterRam()[3 * 0x2000 + 0x1FFF], 0x99) << "PPU 写入落进选中的页";
}

TEST(YuxingMachine, MmcModeTakesTheWindowAndTheCharacterPagesFromTheMmc3Circuit) {
  PoweredMachine rig;
  poke(rig.machine, 0x5500, 0x04);
  rig.machine.board().programRam()[5 * 0x2000] = 0x99;
  rig.machine.board().programRam()[0x3E * 0x2000] = 0x3E;
  rig.machine.board().programRam()[0x3F * 0x2000] = 0x3F;
  poke(rig.machine, 0x5501, 0x80);
  EXPECT_TRUE(rig.machine.board().registers().mmc3Mode);
  // 寄存器 6 选 $8000 起的 8K 页。
  poke(rig.machine, 0x8000, 6);
  poke(rig.machine, 0x8001, 5);
  EXPECT_EQ(peek(rig.machine, 0x8000), 0x99);
  EXPECT_EQ(peek(rig.machine, 0xC000), 0x3E) << "倒数第 2 页固定在 $C000";
  EXPECT_EQ(peek(rig.machine, 0xE000), 0x3F) << "最后一页固定在 $E000";
  // 字符：寄存器 2 选 $1000 起的 1K 页，页号换算到字符 RAM：8 的位 3 起是 1，重排后是 4，所以落在第 32 个 1K。
  rig.machine.board().characterRam()[32 * 1024] = 0xAB;
  poke(rig.machine, 0x8000, 2);
  poke(rig.machine, 0x8001, 8);
  EXPECT_EQ(readPpu(rig.machine, 0x1000), 0xAB);
  // 退出 MMC3 模式后回到 reg5500 与 reg4800 决定的窗口。
  poke(rig.machine, 0x5501, 0x00);
  EXPECT_FALSE(rig.machine.board().registers().mmc3Mode);
  EXPECT_NE(peek(rig.machine, 0x8000), 0x99);
}

TEST(YuxingMachine, MmcModeWritesToA000SetTheMirroring) {
  PoweredMachine rig;
  poke(rig.machine, 0x5500, 0x04);
  poke(rig.machine, 0x5501, 0x80);
  poke(rig.machine, 0xA000, 1);
  EXPECT_TRUE(rig.machine.board().registers().horizontalMirroring);
  poke(rig.machine, 0xA000, 0);
  EXPECT_FALSE(rig.machine.board().registers().horizontalMirroring);
}

/** 用机器码走一遍 PPU 的取指：先设好分屏相关的寄存器，读名称表区 region 里的一个字节（地址位 9、8 是区号），再读图案表 $1000 的第一个字节存进 $0300。 */
std::vector<std::uint8_t> readPatternAfterNametable(unsigned region) {
  std::vector<std::uint8_t> program = {0xA9, 0x80, 0x8D, 0x00, 0x48, 0xA9, 0x80, 0x8D, 0x00, 0x55, 0xA9, 0x01, 0x8D, 0x01, 0x55,
                                       0xA9, static_cast<std::uint8_t>(0x20 | region), 0x8D, 0x06, 0x20, 0xA9, 0x00, 0x8D, 0x06, 0x20, 0xAD, 0x07, 0x20,
                                       0xA9, 0x10, 0x8D, 0x06, 0x20, 0xA9, 0x00, 0x8D, 0x06, 0x20, 0xAD, 0x07, 0x20, 0xAD, 0x07, 0x20, 0x8D, 0x00, 0x03};
  const std::uint16_t self = static_cast<std::uint16_t>(0xFF00 + program.size());
  program.insert(program.end(), {0x4C, static_cast<std::uint8_t>(self & 0xFF), static_cast<std::uint8_t>(self >> 8)});
  return program;
}

TEST(YuxingMachine, FourWaySplitTakesTheUpperCharacterPageFromTheLatchedNametableRegion) {
  for (unsigned region = 0; region < 4; ++region) {
    PoweredMachine rig(readPatternAfterNametable(region));
    auto& ram = rig.machine.board().characterRam();
    // 8K 页 (区号 << 2 | reg5501 的低 2 位) 的上半（偏移 $1000）里标上区号；reg5501 的低 2 位是 1。
    for (unsigned mark = 0; mark < 4; ++mark) ram[(mark << 2 | 1) * 0x2000 + 0x1000] = static_cast<std::uint8_t>(0x40 + mark);
    rig.machine.runFrame();
    rig.machine.runFrame();
    EXPECT_TRUE(rig.machine.board().registers().fourWaySplit);
    EXPECT_EQ(peek(rig.machine, 0x0300), 0x40 + region) << "区 " << region;
  }
}

TEST(YuxingMachine, WithoutTheFourWaySplitTheUpperCharacterPageIsTheSelectedEightKilobytePage) {
  PoweredMachine rig;
  rig.machine.board().characterRam()[1 * 0x2000 + 0x1000] = 0x61;
  poke(rig.machine, 0x5501, 1);
  EXPECT_EQ(readPpu(rig.machine, 0x1000), 0x61);
  poke(rig.machine, 0x4800, 0x80);
  EXPECT_TRUE(rig.machine.board().registers().textMode) << "分屏且 reg5500 位 7 为 0 是汉字单色模式";
  EXPECT_FALSE(rig.machine.board().registers().fourWaySplit);
  poke(rig.machine, 0x5500, 0x80);
  EXPECT_FALSE(rig.machine.board().registers().textMode);
  EXPECT_TRUE(rig.machine.board().registers().fourWaySplit);
}
// ---------------------------------------------------------------- 寄存器与外设

TEST(YuxingMachine, Register5002ReadsTwoAndOnlyWriteRegistersReadOpenBus) {
  PoweredMachine rig;
  std::array<std::uint8_t, 1> value{};
  rig.machine.debugger()->readMemory("cpu", 0x5002, value);
  EXPECT_EQ(value[0], 0x02);
  EXPECT_EQ(peek(rig.machine, 0x4020), 0) << "$4020-$40FF 读出 0";
}

TEST(YuxingMachine, TheKeyboardIsScannedThroughTheRowRegistersAndReadAtFourThousandTwoHundredSeven) {
  PoweredMachine rig;
  rig.machine.input(hw::KeyInput{"Space", true});
  poke(rig.machine, 0x4202, 0x10);
  EXPECT_EQ(peek(rig.machine, 0x4207), 0x80);
  poke(rig.machine, 0x4202, 0x00);
  EXPECT_EQ(peek(rig.machine, 0x4207), 0);
  // 第 13 行（高位寄存器的位 5）：Tab 在第 6 列。
  rig.machine.input(hw::KeyInput{"Tab", true});
  poke(rig.machine, 0x4203, 0x20);
  EXPECT_EQ(peek(rig.machine, 0x4207), 0x40);
  // 另外两组地址也是行选择口。
  poke(rig.machine, 0x4203, 0x00);
  poke(rig.machine, 0x4302, 0x10);
  EXPECT_EQ(peek(rig.machine, 0x4207), 0x80);
  poke(rig.machine, 0x4302, 0x00);
  poke(rig.machine, 0x5005, 0x20);
  EXPECT_EQ(peek(rig.machine, 0x4207), 0x40);
  poke(rig.machine, 0x5005, 0x00);
  poke(rig.machine, 0x5004, 0x10);
  EXPECT_EQ(peek(rig.machine, 0x4207), 0x80);
  poke(rig.machine, 0x4303, 0x20);
  EXPECT_EQ(peek(rig.machine, 0x4207), 0xC0);
}

TEST(YuxingMachine, TheFloppyControllerRegistersAreAtTheirAddresses) {
  PoweredMachine rig;
  // $4201 写 DOR（寄存器 2）：不复位、选驱动器 0、马达 A 开 = $1C；$4304 读主状态，复位后控制器空闲、可以收命令字节（位 7 就绪）。
  poke(rig.machine, 0x4201, 0x1C);
  EXPECT_TRUE(rig.machine.floppyDrive(0)->motorRunning());
  poke(rig.machine, 0x4201, 0x00);
  EXPECT_FALSE(rig.machine.floppyDrive(0)->motorRunning());
}

TEST(YuxingMachine, TheSpeechDataPortSendsBytesAfterTheStartCommandAndTheScrambleXor) {
  PoweredMachine rig;
  // 没有先写 $FF 时半字节被忽略。
  poke(rig.machine, 0x4700, 0xC0);
  poke(rig.machine, 0x4700, 0xC4);
  EXPECT_EQ(rig.machine.speech().pendingBytes(), 0U);
  poke(rig.machine, 0x4700, 0xFF);
  // 低半字节 0、高半字节 4 合成 $40，异或 $41 得 $01；能量字段是 8，一帧没收齐，字节留在缓冲里。
  poke(rig.machine, 0x4700, 0xC0);
  poke(rig.machine, 0x4700, 0xC4);
  EXPECT_EQ(rig.machine.speech().pendingBytes(), 1U);
  // 高 4 位不是 $C 的写入不算半字节。
  poke(rig.machine, 0x4700, 0xD0);
  poke(rig.machine, 0x4700, 0xC0);
  poke(rig.machine, 0x4700, 0xC4);
  EXPECT_EQ(rig.machine.speech().pendingBytes(), 2U);
  EXPECT_EQ(peek(rig.machine, 0x4701), 0) << "缓冲不到 8 个字节时可以写";
  // 写 0 复位。
  poke(rig.machine, 0x4700, 0x00);
  EXPECT_EQ(rig.machine.speech().pendingBytes(), 0U);
}

// ---------------------------------------------------------------- 手柄与磁带口

/** 读手柄的小程序：先写 $4016 为 1 再为 0 锁存，然后连读 8 次 $4016 与 $4017，把位 0 存进 $0300 起与 $0310 起。 */
const std::vector<std::uint8_t> READ_PADS = {0xA9, 0x01, 0x8D, 0x16, 0x40, 0xA9, 0x00, 0x8D, 0x16, 0x40, 0xA2, 0x00, 0xAD, 0x16, 0x40, 0x29, 0x01, 0x9D, 0x00, 0x03,
                                             0xAD, 0x17, 0x40, 0x29, 0x01, 0x9D, 0x10, 0x03, 0xE8, 0xE0, 0x08, 0xD0, 0xEB, 0x4C, 0x21, 0xFF};

TEST(YuxingMachine, BothJoypadsAreReadThroughFourThousandSixteenAndFourThousandSeventeen) {
  PoweredMachine rig(READ_PADS);
  rig.machine.setButtons(0, 0x05);  // A 与 选择
  rig.machine.setButtons(1, 0x82);  // B 与 右
  rig.machine.runFrame();
  rig.machine.runFrame();
  const std::array<std::uint8_t, 8> first{1, 0, 1, 0, 0, 0, 0, 0};
  const std::array<std::uint8_t, 8> second{0, 1, 0, 0, 0, 0, 0, 1};
  for (std::uint16_t index = 0; index < 8; ++index) {
    EXPECT_EQ(peek(rig.machine, 0x0300 + index), first[index]) << "手柄 1 的第 " << index << " 位";
    EXPECT_EQ(peek(rig.machine, 0x0310 + index), second[index]) << "手柄 2 的第 " << index << " 位";
  }
  EXPECT_THROW(rig.machine.setButtons(2, 0), ember::shared::CoreError);
}

/** 读鼠标的小程序：先写 $4016 为 0，再写 1（上升沿），读 $4016 的位 0 存进 $0300。 */
const std::vector<std::uint8_t> READ_MOUSE_START_BIT = {0xA9, 0x00, 0x8D, 0x16, 0x40, 0xA9, 0x01, 0x8D, 0x16, 0x40, 0xAD, 0x16, 0x40, 0x29, 0x01, 0x8D, 0x00, 0x03, 0x4C, 0x12, 0xFF};

TEST(YuxingMachine, TheMouseSendsItsStartBitOnBitZeroAfterTheFirstRisingEdge) {
  PoweredMachine rig(READ_MOUSE_START_BIT);
  rig.machine.runFrame();
  rig.machine.runFrame();
  EXPECT_EQ(peek(rig.machine, 0x0300), 1) << "起始位取反后读 1";
  EXPECT_EQ(rig.machine.mouse().bitsLeft(), 27U);
}

TEST(YuxingMachine, MouseEventsFeedThePacketOfTheNextFrame) {
  PoweredMachine rig;
  hw::MouseInput move;
  move.dx = 7;
  move.dy = -2;
  move.buttons = 1;
  rig.machine.input(move);
  rig.machine.runFrame();
  // 位移与左键在这一帧开始时锁存：之后装的包里第 1 个字节有左键位。
  rig.machine.mouse().write(0x00);
  rig.machine.mouse().write(0x01);
  std::uint8_t first = 0;
  for (unsigned index = 0; index < 9; ++index) {
    if (index > 0) {
      rig.machine.mouse().write(0x00);
      rig.machine.mouse().write(0x01);
    }
    const unsigned bit = rig.machine.mouse().read();
    if (index >= 1 && index <= 7) first = static_cast<std::uint8_t>((first << 1) | (bit ^ 1U));
  }
  EXPECT_EQ(first, 0x63) << "$40 | 左键 $20 | y 的位 7、6 是 3";
}
// ---------------------------------------------------------------- 复位与调试

TEST(YuxingMachine, SoftResetKeepsTheMemoryButRestoresTheRegisters) {
  PoweredMachine rig;
  poke(rig.machine, 0x5500, 0x04);
  poke(rig.machine, 0x8000, 0x5A);
  poke(rig.machine, 0x4800, 0x05);
  rig.machine.softReset();
  EXPECT_EQ(rig.machine.board().registers().reg4800, 0);
  EXPECT_EQ(rig.machine.board().registers().reg5500, 0);
  EXPECT_EQ(rig.machine.board().programRam()[0], 0x5A);
  EXPECT_EQ(peek(rig.machine, 0x8000), PAGE_ZERO_MARK);
  rig.machine.powerOff();
  rig.machine.powerOn();
  EXPECT_EQ(rig.machine.board().programRam()[0], 0) << "上电清零";
}

TEST(YuxingMachine, TheDebuggerResolvesTheWindowsToTheirMemory) {
  PoweredMachine rig;
  auto target = rig.machine.debugger()->resolve("cpu", 0x8000);
  EXPECT_EQ(target.region, "board.rom");
  poke(rig.machine, 0x5500, 0x04);
  target = rig.machine.debugger()->resolve("cpu", 0x8000);
  EXPECT_EQ(target.region, "board.programRam");
  target = rig.machine.debugger()->resolve("cpu", 0x6000);
  EXPECT_EQ(target.region, "board.programRam");
  poke(rig.machine, 0x5500, 0x05);
  target = rig.machine.debugger()->resolve("cpu", 0x6000);
  EXPECT_EQ(target.region, "board.staticRam");
  target = rig.machine.debugger()->resolve("cpu", 0x4800);
  EXPECT_EQ(target.kind, "register");
  target = rig.machine.debugger()->resolve("ppu", 0x0100);
  EXPECT_EQ(target.region, "board.characterRam");
  const auto modules = rig.machine.debugger()->stateModules();
  EXPECT_EQ(modules, (std::vector<std::string>{"cpu", "ppu", "board"}));
  EXPECT_FALSE(rig.machine.debugger()->readState("board").empty());
}

}  // namespace
