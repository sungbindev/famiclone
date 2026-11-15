/*
 * ------------------------------------------------------------------------
 * 名称：sb2000-fc-mode-test.cpp
 * 说明：sb2000 的 FC 兼容模式的测试：MMC3 的主板接线（窗口选择、页号到扩展 RAM 与扩展显存的偏移、名称表偏移、选窗口时撤销 IRQ、上电复位），以及整机的模式切换（$4301、$5006、CPU 在指令结束后复位、PPU 与 APU 的切换、PAL 帧节拍、软复位与上电）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：整机测试用合成的 BIOS：6576 模式的程序写 $5006、$4301 进入 FC 模式，FC 模式的程序与复位向量由测试直接写进扩展 RAM 的窗口 0；
 *       真实 BIOS 加游戏集锦软盘的运行见 sb2000-machine-test.cpp 的 RealGameCollectionDiskStartsAGameInFcMode；AnNromGameRunsFromTheMmc3Windows 按 DOS 下 play 程序的放法直接装入超级马里奥兄弟。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <set>
#include <span>
#include <vector>

#include "bootstrap/machines/sb2000-fc-board.hpp"
#include "bootstrap/machines/sb2000.hpp"
#include "logic/hardware/lines/wired-or-line.hpp"
#include "logic/hardware/mapper-chips/mmc3/mmc3.hpp"

namespace {

namespace hw = ember::contracts::hardware;
using ember::bootstrap::Sb2000FcMapper;
using ember::bootstrap::Sb2000Machine;
using ember::logic::hardware::WiredOrIrqLine;
using ember::logic::hardware::mapper_chips::Mmc3;

// ---------------------------------------------------------------- MMC3 映射器

/** 映射器测试台：MMC3 电路装在主板接线上。 */
struct MapperRig {
  MapperRig() : chip(irq), mapper(chip) {}

  /** 写 MMC3 的页寄存器 R0-R7。 */
  void setRegister(unsigned index, std::uint8_t value) {
    mapper.writeRegister(0x8000, static_cast<std::uint8_t>((chip.selector() & 0xC0U) | index));
    mapper.writeRegister(0x8001, value);
  }

  /** 设置 $8000 的选择位（位 7 反转图案、位 6 程序模式），保持当前选中的寄存器号。 */
  void setSelectorBits(std::uint8_t bits) { mapper.writeRegister(0x8000, bits); }

  /** 一次 A12 上升沿：先有足够多的低电平访问，再变高。 */
  void clockEdge() {
    for (unsigned i = 0; i < Mmc3::A12_LOW_ACCESSES; ++i) mapper.onVideoAddress(0x0000);
    mapper.onVideoAddress(0x1000);
  }

  WiredOrIrqLine irq;
  Mmc3 chip;
  Sb2000FcMapper mapper;
};

constexpr std::size_t PAGE = 8 * 1024;
constexpr std::size_t KILOBYTE = 1024;

TEST(Sb2000FcMapper, ProgramWindowFollowsTheRegistersAndTheModeBit) {
  MapperRig rig;
  // 上电值：R6、R7 为 0，$C000 与 $E000 固定为窗口的最后两页。
  EXPECT_EQ(rig.mapper.programOffset(0x8000), 0U);
  EXPECT_EQ(rig.mapper.programOffset(0xA000), 0U);
  EXPECT_EQ(rig.mapper.programOffset(0xC000), 14 * PAGE);
  EXPECT_EQ(rig.mapper.programOffset(0xE000), 15 * PAGE);
  EXPECT_EQ(rig.mapper.programOffset(0xFFFC), 15 * PAGE + 0x1FFC);

  rig.setRegister(6, 5);
  rig.setRegister(7, 9);
  EXPECT_EQ(rig.mapper.programOffset(0x8000), 5 * PAGE);
  EXPECT_EQ(rig.mapper.programOffset(0x8123), 5 * PAGE + 0x123);
  EXPECT_EQ(rig.mapper.programOffset(0xA000), 9 * PAGE);
  EXPECT_EQ(rig.mapper.programOffset(0xC000), 14 * PAGE);

  // 选择位 6：$8000 固定为倒数第 2 页，$C000 由 R6 选。
  rig.setSelectorBits(0x40);
  EXPECT_EQ(rig.mapper.programOffset(0x8000), 14 * PAGE);
  EXPECT_EQ(rig.mapper.programOffset(0xC000), 5 * PAGE);
  EXPECT_EQ(rig.mapper.programOffset(0xE000), 15 * PAGE);

  // R6、R7 只取低 4 位。
  rig.setSelectorBits(0x00);
  rig.setRegister(6, 0xF3);
  EXPECT_EQ(rig.mapper.programOffset(0x8000), 3 * PAGE);
}

TEST(Sb2000FcMapper, WindowSelectionUsesTheBitsOfTheWrittenValueAndMovesBothMaps) {
  MapperRig rig;
  const std::uint8_t values[] = {0x55, 0x77, 0xDD, 0xFF};
  for (unsigned window = 0; window < 4; ++window) {
    rig.mapper.selectWindow(values[window]);
    EXPECT_EQ(rig.mapper.window(), window) << "写入 $" << std::hex << static_cast<int>(values[window]);
    EXPECT_EQ(rig.mapper.programOffset(0x8000), window * Sb2000FcMapper::WINDOW_SIZE);
    EXPECT_EQ(rig.mapper.characterOffset(0x0000), window * Sb2000FcMapper::WINDOW_SIZE);
  }
}

TEST(Sb2000FcMapper, CharacterPagesFollowTheRegistersAndTheInversionBit) {
  MapperRig rig;
  rig.setRegister(0, 4);
  rig.setRegister(1, 10);
  rig.setRegister(2, 21);
  rig.setRegister(3, 22);
  rig.setRegister(4, 23);
  rig.setRegister(5, 24);
  // 不反转：$0000-$07FF 是 R0 的 2K，$0800-$0FFF 是 R1 的 2K，$1000-$1FFF 是 R2-R5 的 1K。
  EXPECT_EQ(rig.mapper.characterOffset(0x0000), 4 * KILOBYTE);
  EXPECT_EQ(rig.mapper.characterOffset(0x0400), 5 * KILOBYTE);
  EXPECT_EQ(rig.mapper.characterOffset(0x0800), 10 * KILOBYTE);
  EXPECT_EQ(rig.mapper.characterOffset(0x0C10), 11 * KILOBYTE + 0x10);
  EXPECT_EQ(rig.mapper.characterOffset(0x1000), 21 * KILOBYTE);
  EXPECT_EQ(rig.mapper.characterOffset(0x1400), 22 * KILOBYTE);
  EXPECT_EQ(rig.mapper.characterOffset(0x1800), 23 * KILOBYTE);
  EXPECT_EQ(rig.mapper.characterOffset(0x1FFF), 24 * KILOBYTE + 0x3FF);
  // 反转：两个半区对调。
  rig.setSelectorBits(0x80);
  EXPECT_EQ(rig.mapper.characterOffset(0x0000), 21 * KILOBYTE);
  EXPECT_EQ(rig.mapper.characterOffset(0x0C00), 24 * KILOBYTE);
  EXPECT_EQ(rig.mapper.characterOffset(0x1000), 4 * KILOBYTE);
  EXPECT_EQ(rig.mapper.characterOffset(0x1400), 5 * KILOBYTE);
  EXPECT_EQ(rig.mapper.characterOffset(0x1800), 10 * KILOBYTE);
  // R0、R1 取偶数页，所有寄存器只取低 7 位。
  rig.setSelectorBits(0x00);
  rig.setRegister(0, 0xFF);
  EXPECT_EQ(rig.mapper.characterOffset(0x0000), 0x7E * KILOBYTE);
  EXPECT_EQ(rig.mapper.characterOffset(0x0400), 0x7F * KILOBYTE);
  rig.setRegister(2, 0xFF);
  EXPECT_EQ(rig.mapper.characterOffset(0x1000), 0x7F * KILOBYTE);
}

TEST(Sb2000FcMapper, NametableMirroringFollowsTheMirrorRegister) {
  MapperRig rig;
  // 上电后垂直镜像：$2000 与 $2800 同一页，$2400 与 $2C00 同一页。
  EXPECT_EQ(rig.mapper.nametableOffset(0x2000), 0x2000U);
  EXPECT_EQ(rig.mapper.nametableOffset(0x2400), 0x2400U);
  EXPECT_EQ(rig.mapper.nametableOffset(0x2800), 0x2000U);
  EXPECT_EQ(rig.mapper.nametableOffset(0x2C05), 0x2405U);
  EXPECT_EQ(rig.mapper.nametableOffset(0x3000), 0x2000U) << "$3000 起镜像 $2000 起";
  rig.mapper.writeRegister(0xA000, 1);  // 水平
  EXPECT_EQ(rig.mapper.nametableOffset(0x2000), 0x2000U);
  EXPECT_EQ(rig.mapper.nametableOffset(0x2400), 0x2000U);
  EXPECT_EQ(rig.mapper.nametableOffset(0x2800), 0x2400U);
  EXPECT_EQ(rig.mapper.nametableOffset(0x2C00), 0x2400U);
  rig.mapper.writeRegister(0xA001, 0);  // PRG RAM 保护，忽略
  EXPECT_TRUE(rig.chip.horizontalMirroring());
  rig.mapper.writeRegister(0xBFFE, 0);  // 偶地址都是镜像寄存器
  EXPECT_FALSE(rig.chip.horizontalMirroring());
}

TEST(Sb2000FcMapper, SelectingAWindowDisablesAndClearsTheIrq) {
  MapperRig rig;
  rig.mapper.writeRegister(0xC000, 0);
  rig.mapper.writeRegister(0xE001, 0);
  rig.clockEdge();
  ASSERT_TRUE(rig.irq.active());
  rig.mapper.selectWindow(0x77);
  EXPECT_FALSE(rig.irq.active());
  EXPECT_FALSE(rig.chip.irqEnabled());
}

TEST(Sb2000FcMapper, PowerResetClearsEverythingButAWriteDoesNot) {
  MapperRig rig;
  rig.setRegister(6, 7);
  rig.mapper.selectWindow(0xFF);
  rig.mapper.writeRegister(0xA000, 1);
  rig.mapper.reset();
  EXPECT_EQ(rig.chip.registerValue(6), 0);
  EXPECT_EQ(rig.mapper.window(), 0U);
  EXPECT_FALSE(rig.chip.horizontalMirroring());
}

// ---------------------------------------------------------------- 整机的模式切换

/** 合成的 BIOS：$C000 的程序进入 6576 模式，选窗口 0，向 $4301 写 $55，随后的 INC $02 不应执行。 */
std::vector<std::uint8_t> fcBios() {
  std::vector<std::uint8_t> bios(Sb2000Machine::BIOS_SIZE, 0xEA);
  const std::uint8_t vectors[] = {0x00, 0xC0, 0x00, 0xC0, 0x00, 0xC0};
  std::copy(std::begin(vectors), std::end(vectors), bios.begin() + 0x7FFA);
  const std::uint8_t program[] = {
      0x78,              // SEI
      0xA9, 0x65,        // LDA #$65
      0x8D, 0x31, 0x40,  // STA $4031
      0xA9, 0x76,        // LDA #$76
      0x8D, 0x31, 0x40,  // STA $4031
      0xA9, 0x55,        // LDA #$55
      0x8D, 0x06, 0x50,  // STA $5006（窗口 0）
      0x8D, 0x01, 0x43,  // STA $4301（进入 FC 兼容模式）
      0xE6, 0x02,        // INC $02：切换发生在 STA 结束后，这条不会执行
      0x4C, 0x15, 0xC0   // JMP $C015
  };
  std::copy(std::begin(program), std::end(program), bios.begin() + 0x4000);
  return bios;
}

/** FC 模式下窗口 0 里的程序：开背景、在 $00 上计数；复位向量指向 $E000。 */
void loadFcProgram(Sb2000Machine& machine) {
  const std::uint8_t program[] = {
      0x78,              // SEI
      0xA9, 0x08,        // LDA #$08
      0x8D, 0x01, 0x20,  // STA $2001
      0xE6, 0x00,        // loop: INC $00（$E006）
      0x4C, 0x06, 0xE0   // JMP $E006
  };
  machine.debugger()->writeMemory("board.extendedRam", 0x1E000, program);
  const std::uint8_t vector[] = {0x00, 0xE0};
  machine.debugger()->writeMemory("board.extendedRam", 0x1FFFC, vector);
}

std::uint8_t peekBus(Sb2000Machine& machine, std::uint16_t address) { return machine.peekCpuBus(address); }

void pokeBus(Sb2000Machine& machine, std::uint16_t address, std::uint8_t value) {
  const std::uint8_t bytes[] = {value};
  machine.debugger()->writeMemory("cpu", address, bytes);
}

/** 开机并跑到进入 FC 模式后的第一帧结束。 */
void bootIntoFcMode(Sb2000Machine& machine) {
  machine.powerOn();
  loadFcProgram(machine);
  for (int frame = 0; frame < 4 && !machine.fcMode(); ++frame) machine.runFrame();
  ASSERT_TRUE(machine.fcMode());
}

TEST(Sb2000FcMode, WritingTheModeRegisterSwitchesAfterTheInstructionAndRestartsTheCpu) {
  Sb2000Machine machine(fcBios());
  machine.powerOn();
  loadFcProgram(machine);
  EXPECT_FALSE(machine.fcMode());
  EXPECT_TRUE(machine.keyboardIsMachineInput());
  EXPECT_TRUE(machine.hasMouse());
  machine.runFrame();
  ASSERT_TRUE(machine.fcMode());
  EXPECT_FALSE(machine.keyboardIsMachineInput()) << "进入 FC 兼容模式后键盘可以当手柄用";
  EXPECT_FALSE(machine.hasMouse()) << "进入 FC 兼容模式后没有串口鼠标";
  EXPECT_EQ(peekBus(machine, 0x0002), 0) << "写 $4301 的 STA 之后的 INC $02 没有执行：CPU 在指令结束后复位";
  machine.runFrame();
  EXPECT_NE(peekBus(machine, 0x0000), 0) << "窗口 0 里的 FC 程序在运行";
}

TEST(Sb2000FcMode, OnlyTheKeyValueSwitchesTheMode) {
  std::vector<std::uint8_t> bios = fcBios();
  bios[0x4000 + 0x0B + 1] = 0x54;  // LDA #$54：不是 $55
  Sb2000Machine machine(bios);
  machine.powerOn();
  machine.runFrame();
  EXPECT_FALSE(machine.fcMode());
}

TEST(Sb2000FcMode, ThePpuAndTheApuAreSwitchedAndTheFramesKeepComing) {
  Sb2000Machine machine(fcBios());
  bootIntoFcMode(machine);
  EXPECT_EQ(machine.audioSampleRate(), 48000U);
  EXPECT_EQ(machine.fcPpu().frameCount(), 1U) << "第一帧在进入 FC 模式后完成";
  const std::uint64_t graphicFrames = machine.ppu().frameCount();
  const std::uint64_t cyclesBefore = machine.cpuRegisters().cycles;
  machine.runFrame();
  machine.runFrame();
  EXPECT_EQ(machine.ppu().frameCount(), graphicFrames) << "6576 的 PPU 不再运行";
  EXPECT_EQ(machine.fcPpu().frameCount(), 3U);
  EXPECT_EQ(machine.frame().width, 256U);
  EXPECT_EQ(machine.frame().height, 240U);
  // PAL：每帧 312 行、每行 341 点、每点 5 个主时钟周期，CPU 每周期 15 个主时钟周期。
  const double cyclesPerFrame = 312.0 * 341.0 * 5.0 / 15.0;
  EXPECT_NEAR(static_cast<double>(machine.cpuRegisters().cycles - cyclesBefore), 2.0 * cyclesPerFrame, 50.0);
  EXPECT_EQ(machine.fcPpu().scanline() < 312, true);
  std::vector<float> samples;
  machine.takeAudio(samples);
  EXPECT_GT(samples.size(), 1000U) << "FC 模式的声音由 APU 产生";
  EXPECT_LT(samples.size(), 4000U) << "切换时刻不会补出之前的采样";
}

TEST(Sb2000FcMode, ExternalPeripheralsAreResetAndTheWorkWindowBecomesRamAgain) {
  Sb2000Machine machine(fcBios());
  bootIntoFcMode(machine);
  // 6576 模式下 $4400-$5FFF 读 $68；FC 模式下 $5000-$5FFF 是 2K 工作 RAM（与内部 RAM 同一块）。
  pokeBus(machine, 0x0123, 0x5A);
  EXPECT_EQ(peekBus(machine, 0x5123), 0x5A);
  EXPECT_EQ(peekBus(machine, 0x5923), 0x5A) << "镜像两次";
  pokeBus(machine, 0x5456, 0xA5);
  EXPECT_EQ(peekBus(machine, 0x0456), 0xA5);
}

TEST(Sb2000FcMode, TheProgramWindowIsMappedByMmc3AndWritesAreRegisterWrites) {
  Sb2000Machine machine(fcBios());
  bootIntoFcMode(machine);
  // 窗口 0 的第 5 页放一个标记，R6 选它之后出现在 $8000。
  const std::uint8_t marker[] = {0x5B};
  machine.debugger()->writeMemory("board.extendedRam", 5 * 8192, marker);
  EXPECT_NE(peekBus(machine, 0x8000), 0x5B);
  pokeBus(machine, 0x8000, 6);
  pokeBus(machine, 0x8001, 5);
  EXPECT_EQ(peekBus(machine, 0x8000), 0x5B);
  EXPECT_EQ(machine.mmc3().chipValue("r6"), 5U);
  // 写 $8000 以上不会写进扩展 RAM。
  std::uint8_t byte = 0;
  machine.debugger()->readMemory("board.extendedRam", 5 * 8192, std::span<std::uint8_t>(&byte, 1));
  EXPECT_EQ(byte, 0x5B);
  // 窗口 1 的同一页放另一个标记，$5006 选窗口后读到它。
  const std::uint8_t other[] = {0x6C};
  machine.debugger()->writeMemory("board.extendedRam", Sb2000FcMapper::WINDOW_SIZE + 5 * 8192, other);
  pokeBus(machine, 0x5006, 0x77);
  EXPECT_EQ(machine.mmc3().window(), 1U);
  EXPECT_EQ(peekBus(machine, 0x8000), 0x6C);
}

TEST(Sb2000FcMode, TheWindowSelectionRegisterWorksInTheNativeModeToo) {
  Sb2000Machine machine(fcBios());
  machine.powerOn();
  pokeBus(machine, 0x5006, 0xDD);
  EXPECT_EQ(machine.mmc3().window(), 2U);
  EXPECT_FALSE(machine.fcMode());
}

TEST(Sb2000FcMode, ThePatternAreaIsReadOnlyAndTheNametableFollowsTheMirroring) {
  Sb2000Machine machine(fcBios());
  bootIntoFcMode(machine);
  hw::MachineDebugger* debugger = machine.debugger();
  // 图案区：扩展显存窗口 0 的第 0 页放标记，经 PPU 数据口读得到、写不进去。
  const std::uint8_t pattern[] = {0x3C};
  debugger->writeMemory("board.extendedVram", 0, pattern);
  std::uint8_t byte = 0;
  debugger->readMemory("ppu", 0x0000, std::span<std::uint8_t>(&byte, 1));
  EXPECT_EQ(byte, 0x3C);
  const std::uint8_t other[] = {0x99};
  debugger->writeMemory("ppu", 0x0000, other);
  debugger->readMemory("ppu", 0x0000, std::span<std::uint8_t>(&byte, 1));
  EXPECT_EQ(byte, 0x3C) << "图案区只读";
  // 名称表：垂直镜像时 $2000 与 $2800 同一页。
  const std::uint8_t tile[] = {0x42};
  debugger->writeMemory("ppu", 0x2005, tile);
  debugger->readMemory("ppu", 0x2805, std::span<std::uint8_t>(&byte, 1));
  EXPECT_EQ(byte, 0x42);
  debugger->readMemory("ppu", 0x2405, std::span<std::uint8_t>(&byte, 1));
  EXPECT_NE(byte, 0x42);
  // 名称表落在字符 RAM 的 $2000 起。
  debugger->readMemory("board.characterRam", 0x2005, std::span<std::uint8_t>(&byte, 1));
  EXPECT_EQ(byte, 0x42);
  // 改成水平镜像：$2000 与 $2400 同一页。
  pokeBus(machine, 0xA000, 1);
  debugger->readMemory("ppu", 0x2405, std::span<std::uint8_t>(&byte, 1));
  EXPECT_EQ(byte, 0x42);
}

/** 读仓库 reference 目录里的文件；不存在时返回空。 */
std::vector<std::uint8_t> readReference(const std::filesystem::path& relative) {
  const std::filesystem::path path = std::filesystem::path(EMBER_TEST_DATA_DIR) / ".." / ".." / ".." / ".." / relative;
  std::ifstream stream(path, std::ios::binary);
  if (!stream) return {};
  return std::vector<std::uint8_t>((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
}

TEST(Sb2000FcMode, AnNromGameRunsFromTheMmc3Windows) {
  // 超级马里奥兄弟：mapper 0，32K PRG + 8K CHR，水平滚动、垂直镜像。按 DOS 下 play 程序的放法装入：
  // PRG 放在扩展 RAM 窗口 0 的末尾 32K，CHR 放在扩展显存窗口 0 的开头，复位向量改为 ERAM $6100 的启动桩，桩里设好 MMC3 寄存器后跳到游戏原来的入口。
  const std::vector<std::uint8_t> rom = readReference(std::filesystem::path(u8"reference/nes-games/超级马里奥兄弟(日美).nes"));
  if (rom.size() != 16 + 32768 + 8192) GTEST_SKIP() << "没有找到 reference/nes-games 里的超级马里奥兄弟";
  Sb2000Machine machine(fcBios());
  machine.powerOn();
  hw::MachineDebugger* debugger = machine.debugger();
  debugger->writeMemory("board.extendedRam", 0x18000, std::span<const std::uint8_t>(rom).subspan(16, 32768));
  debugger->writeMemory("board.extendedVram", 0, std::span<const std::uint8_t>(rom).subspan(16 + 32768, 8192));
  const std::uint8_t patchedReset[] = {0x00, 0x61};
  debugger->writeMemory("board.extendedRam", 0x1FFFC, patchedReset);
  const std::uint8_t stub[] = {
      0x78,                    // SEI
      0xA2, 0x00,              // LDX #$00
      0xBD, 0x1D, 0x61,        // loop: LDA table,X
      0x8D, 0x00, 0x80,        // STA $8000
      0xBD, 0x1E, 0x61,        // LDA table+1,X
      0x8D, 0x01, 0x80,        // STA $8001
      0xE8, 0xE8,              // INX、INX
      0xE0, 0x10,              // CPX #$10
      0xD0, 0xEE,              // BNE loop
      0xA9, static_cast<std::uint8_t>((rom[6] & 1U) ^ 1U),  // LDA #镜像（垂直镜像写 0）
      0x8D, 0x00, 0xA0,        // STA $A000
      0x4C, rom[16 + 0x7FFC], rom[16 + 0x7FFD],  // JMP 游戏原来的复位入口
      0x00, 0x00, 0x01, 0x02, 0x02, 0x04, 0x03, 0x05, 0x04, 0x06, 0x05, 0x07, 0x06, 0x0C, 0x07, 0x0D};  // 寄存器号与初值：R0-R5 为 CHR，R6、R7 为 PRG 的第 12、13 个 8K
  debugger->writeMemory("board.externalRam", 0x100, stub);
  for (int frame = 0; frame < 4 && !machine.fcMode(); ++frame) machine.runFrame();
  ASSERT_TRUE(machine.fcMode());
  for (int frame = 0; frame < 120; ++frame) machine.runFrame();
  EXPECT_EQ(peekBus(machine, 0x0770), 0) << "$0770 是游戏的运行模式，标题画面为 0";
  const hw::FrameBuffer title = machine.frame();
  const std::set<std::uint32_t> titleColors(title.pixels.begin(), title.pixels.end());
  EXPECT_GT(titleColors.size(), 4U) << "标题画面是多色的（图案取自扩展显存）";
  // 标题画面按开始：游戏要等 0 号精灵命中来分隔状态栏，命中不对游戏就停住。
  machine.setButtons(0, hw::pad_button::START);
  for (int frame = 0; frame < 10; ++frame) machine.runFrame();
  machine.setButtons(0, 0);
  for (int frame = 0; frame < 400; ++frame) machine.runFrame();
  EXPECT_EQ(peekBus(machine, 0x0770), 1) << "按开始后进入游戏";
  const hw::FrameBuffer level = machine.frame();
  const std::set<std::uint32_t> levelColors(level.pixels.begin(), level.pixels.end());
  EXPECT_GT(levelColors.size(), 4U);
  EXPECT_NE(peekBus(machine, 0x075A), 0) << "$075A 是剩余生命数，进入游戏后为 2";
  std::vector<float> samples;
  machine.takeAudio(samples);
  ASSERT_FALSE(samples.empty());
  EXPECT_GT(*std::max_element(samples.begin(), samples.end()), 0.01F) << "背景音乐在 APU 上发声";
}

TEST(Sb2000FcMode, TheSpriteBackdropEntriesMirrorTheBackgroundOnes) {
  Sb2000Machine machine(fcBios());
  bootIntoFcMode(machine);
  // 游戏（如马里奥）靠写 $3F10 改背景色：标准 PPU 的调色板方式。
  pokeBus(machine, 0x2006, 0x3F);
  pokeBus(machine, 0x2006, 0x10);
  pokeBus(machine, 0x2007, 0x15);
  std::uint8_t entry = 0;
  machine.debugger()->readMemory("chip.palette", 0, std::span<std::uint8_t>(&entry, 1));
  EXPECT_EQ(entry, 0x15);
}

TEST(Sb2000FcMode, DebuggerDescribesTheFcModeAddresses) {
  Sb2000Machine machine(fcBios());
  bootIntoFcMode(machine);
  hw::MachineDebugger* debugger = machine.debugger();
  pokeBus(machine, 0x8000, 6);
  pokeBus(machine, 0x8001, 3);
  const auto program = debugger->resolve("cpu", 0x8010);
  EXPECT_EQ(program.region, "board.extendedRam");
  EXPECT_EQ(program.offset, 3U * 8192 + 0x10);
  const auto pattern = debugger->resolve("ppu", 0x0000);
  EXPECT_EQ(pattern.region, "board.extendedVram");
  const auto nametable = debugger->resolve("ppu", 0x2000);
  EXPECT_EQ(nametable.region, "board.characterRam");
  EXPECT_EQ(nametable.offset, 0x2000U);
  const auto modules = debugger->stateModules();
  EXPECT_NE(std::find(modules.begin(), modules.end(), "mmc3"), modules.end());
  bool sawFcMode = false;
  for (const auto& field : debugger->readState("paging")) {
    if (field.name == "fcMode") {
      sawFcMode = true;
      EXPECT_EQ(field.value, 1);
    }
  }
  EXPECT_TRUE(sawFcMode);
}

TEST(Sb2000FcMode, SoftResetKeepsTheModeAndTheMappingAndHardResetGoesBackToTheNativeMode) {
  Sb2000Machine machine(fcBios());
  bootIntoFcMode(machine);
  pokeBus(machine, 0x8000, 6);
  pokeBus(machine, 0x8001, 4);
  pokeBus(machine, 0x0010, 0xAB);
  machine.softReset();
  EXPECT_TRUE(machine.fcMode());
  EXPECT_EQ(machine.mmc3().chipValue("r6"), 4U) << "映射器状态保持";
  EXPECT_EQ(peekBus(machine, 0x0010), 0xAB) << "内存保持";
  machine.runFrame();
  EXPECT_TRUE(machine.fcMode()) << "软复位不会回到 6576 模式";
  EXPECT_EQ(machine.cpuRegisters().pc >= 0x8000, true);

  const std::uint64_t fcFramesBefore = machine.fcPpu().frameCount();
  machine.powerOff();
  machine.powerOn();
  EXPECT_FALSE(machine.fcMode());
  EXPECT_EQ(machine.mmc3().chipValue("r6"), 0U);
  EXPECT_EQ(peekBus(machine, 0x0010), 0x00);
  // 合成的 BIOS 会再次进入 FC 模式，所以只看“回到 6576 模式”的那一刻：FC 的 PPU 停在复位前的帧数，没有被重新启动。
  EXPECT_EQ(machine.fcPpu().frameCount(), fcFramesBefore);
}

}  // namespace
