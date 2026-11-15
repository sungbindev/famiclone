/*
 * ------------------------------------------------------------------------
 * 名称：sb2000-machine-test.cpp
 * 说明：主机型号 sb2000 的测试：存储布局与译码（用合成的 BIOS 检查）、电源与复位向量、通用 DMA、显存页、输入；以及用真实 BIOS 开机的冒烟测试。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：真实 BIOS 在仓库的 reference/rom/SB2K_BIOS.NES（iNES 文件，16 字节文件头之后是 512KB 的 PRG）；文件不存在时跳过冒烟测试；
 *       设置环境变量 EMBER_SB2000_DUMP 为一个文件路径时，冒烟测试把开机后的画面写成 PPM 文件，供人工查看。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "bootstrap/machines/sb2000/sb2000.hpp"
#include "contracts/hardware/hardware-error.hpp"
#include "shared/errors/core-error.hpp"

namespace {

namespace hw = ember::contracts::hardware;
using ember::bootstrap::Sb2000Machine;

/** 合成的 BIOS：每个 4K 页的第一个字节是页号，复位向量指向 $C000，$C000 处是一小段测试程序。 */
std::vector<std::uint8_t> syntheticBios() {
  std::vector<std::uint8_t> bios(Sb2000Machine::BIOS_SIZE, 0xEA);
  for (std::size_t page = 0; page < 128; ++page) bios[page * 0x1000] = static_cast<std::uint8_t>(page);
  // 向量表：NMI、RESET、IRQ 都指向 $C000 开始的程序。
  bios[0x7FFA] = 0x00;
  bios[0x7FFB] = 0xC0;
  bios[0x7FFC] = 0x00;
  bios[0x7FFD] = 0xC0;
  bios[0x7FFE] = 0x00;
  bios[0x7FFF] = 0xC0;
  // $C000：进入 6576 模式，打开背景显示，然后在 $0000 上无限计数。
  const std::uint8_t program[] = {
      0x78,              // SEI
      0xA9, 0x65,        // LDA #$65
      0x8D, 0x31, 0x40,  // STA $4031
      0xA9, 0x76,        // LDA #$76
      0x8D, 0x31, 0x40,  // STA $4031
      0xA9, 0x08,        // LDA #$08
      0x8D, 0x01, 0x20,  // STA $2001
      0xE6, 0x00,        // loop: INC $00
      0x4C, 0x10, 0xC0   // JMP loop（$C010）
  };
  std::copy(std::begin(program), std::end(program), bios.begin() + 0x4000);
  return bios;
}

/** 读真实的 BIOS：跳过 iNES 文件头；文件不存在时返回空。 */
std::vector<std::uint8_t> realBios() {
  const std::filesystem::path path = std::filesystem::path(EMBER_TEST_DATA_DIR) / ".." / ".." / ".." / ".." / "reference" / "rom" / "SB2K_BIOS.NES";
  std::ifstream stream(path, std::ios::binary);
  if (!stream) return {};
  std::vector<std::uint8_t> file((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
  if (file.size() != 16 + Sb2000Machine::BIOS_SIZE) return {};
  return std::vector<std::uint8_t>(file.begin() + 16, file.end());
}

/** 读仓库 reference/floppy-images/sb2000 里的盘片镜像；文件不存在时返回空。 */
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

std::uint8_t peek(Sb2000Machine& machine, std::uint16_t address) { return machine.peekCpuBus(address); }

/** 记录软驱马达开关通知的接收方。 */
struct MotorRecorder final : hw::FloppyActivity {
  std::vector<std::pair<unsigned, bool>> changes;
  void motorChanged(unsigned drive, bool on) override { changes.emplace_back(drive, on); }
};

/** 环境变量 EMBER_SB2000_DUMP 的值；没有设置时为空。 */
std::string dumpPath() {
#ifdef _MSC_VER
  char* value = nullptr;
  std::size_t length = 0;
  if (_dupenv_s(&value, &length, "EMBER_SB2000_DUMP") != 0 || value == nullptr) return {};
  std::string path(value);
  std::free(value);
  return path;
#else
  const char* value = std::getenv("EMBER_SB2000_DUMP");
  return value == nullptr ? std::string() : std::string(value);
#endif
}

void poke(Sb2000Machine& machine, std::uint16_t address, std::uint8_t value) {
  const std::uint8_t bytes[] = {value};
  machine.debugger()->writeMemory("cpu", address, bytes);
}

TEST(Sb2000Machine, RejectsABiosOfTheWrongSize) {
  try {
    Sb2000Machine machine(std::vector<std::uint8_t>(1024, 0));
    FAIL() << "应该抛出异常";
  } catch (const ember::shared::CoreError& error) {
    EXPECT_EQ(error.code(), hw::ERROR_INVALID_ARGUMENT);
  }
}

TEST(Sb2000Machine, ProgramWindowsStartAsRomPagesZeroToSeven) {
  Sb2000Machine machine(syntheticBios());
  EXPECT_EQ(peek(machine, 0x8000), 0);
  EXPECT_EQ(peek(machine, 0x9000), 1);
  EXPECT_EQ(peek(machine, 0xF000), 7);
}

TEST(Sb2000Machine, PagingRegistersSelectRomOrExtendedRamPages) {
  Sb2000Machine machine(syntheticBios());
  poke(machine, 0x4040, 0x05);
  EXPECT_EQ(peek(machine, 0x8000), 5) << "页号小于 $80 取 BIOS ROM";
  poke(machine, 0x4041, 0x83);
  poke(machine, 0x9000, 0x5A);
  EXPECT_EQ(peek(machine, 0x9000), 0x5A) << "页号不小于 $80 取扩展 RAM，可写";
  EXPECT_EQ(machine.extendedRam()[3 * 0x1000], 0x5A);
  poke(machine, 0x8000, 0x77);
  EXPECT_EQ(peek(machine, 0x8000), 5) << "ROM 页的写入被丢弃";
}

TEST(Sb2000Machine, InternalRamIsTwoKilobytesMirroredAndExternalRamIsEight) {
  Sb2000Machine machine(syntheticBios());
  poke(machine, 0x0000, 0x11);
  EXPECT_EQ(peek(machine, 0x0800), 0x11);
  EXPECT_EQ(peek(machine, 0x1800), 0x11);
  poke(machine, 0x6000, 0x22);
  poke(machine, 0x6400, 0x33);
  EXPECT_EQ(peek(machine, 0x6000), 0x22) << "外部 RAM 各 1K 互相独立";
  EXPECT_EQ(peek(machine, 0x6400), 0x33);
  poke(machine, 0x7FFF, 0x44);
  EXPECT_EQ(machine.externalRam().read(0x1FFF), 0x44);
}

TEST(Sb2000Machine, WorkWindowReadsFixedValueOnceInitialSetupIsDone) {
  Sb2000Machine machine(syntheticBios());
  poke(machine, 0x5000, 0x99);
  EXPECT_EQ(peek(machine, 0x5000), 0x99) << "进入 6576 模式之前 $5000-$5FFF 是 RAM";
  poke(machine, 0x4031, 0x65);
  poke(machine, 0x4031, 0x76);
  EXPECT_EQ(peek(machine, 0x4400), 0x68);
  EXPECT_EQ(peek(machine, 0x5000), 0x68);
  poke(machine, 0x5000, 0x55);
  EXPECT_EQ(peek(machine, 0x5000), 0x68) << "写入无效";
  EXPECT_EQ(machine.internalRam().read(0), 0x99 & 0xFF) << "没有写进内部 RAM";
}

TEST(Sb2000Machine, VideoBankRegisterSelectsTheExtendedVramPage) {
  Sb2000Machine machine(syntheticBios());
  const std::uint8_t first[] = {0xA1};
  const std::uint8_t second[] = {0xB2};
  poke(machine, 0x4300, 0x00);
  machine.debugger()->writeMemory("ppu", 0x8000, first);
  poke(machine, 0x4300, 0x03);
  machine.debugger()->writeMemory("ppu", 0x8000, second);
  EXPECT_EQ(machine.video().extendedVram()[0], 0xA1);
  EXPECT_EQ(machine.video().extendedVram()[3 * 0x8000], 0xB2);
  std::uint8_t out[1] = {0};
  machine.debugger()->readMemory("ppu", 0x8000, out);
  EXPECT_EQ(out[0], 0xB2);
  const std::uint8_t character[] = {0xC3};
  machine.debugger()->writeMemory("ppu", 0x0123, character);
  EXPECT_EQ(machine.video().characterRam()[0x0123], 0xC3) << "$0000-$7FFF 是字符 RAM，不受页寄存器影响";
}

TEST(Sb2000Machine, GeneralDmaCopiesRomIntoVideoMemory) {
  std::vector<std::uint8_t> bios = syntheticBios();
  bios[0x100] = 1;
  bios[0x101] = 2;
  bios[0x102] = 3;
  bios[0x103] = 4;
  Sb2000Machine machine(std::move(bios));
  poke(machine, 0x4049, 0x00);  // 源：BIOS ROM，页 0
  poke(machine, 0x404A, 0x00);  // 源地址低
  poke(machine, 0x404B, 0x81);  // 源地址高：位 7 必须为 1，A14-A8 = 1，所以源在 $0100
  poke(machine, 0x404C, 0x20);  // 目的低
  poke(machine, 0x404D, 0x01);  // 目的高：显存 $0120
  poke(machine, 0x404E, 0x03);  // 长度减 1
  poke(machine, 0x404F, 0x00);
  poke(machine, 0x4048, 0xC0);  // 启动，目的是显存
  EXPECT_EQ(machine.video().characterRam()[0x120], 1);
  EXPECT_EQ(machine.video().characterRam()[0x121], 2);
  EXPECT_EQ(machine.video().characterRam()[0x122], 3);
  EXPECT_EQ(machine.video().characterRam()[0x123], 4);
  EXPECT_EQ(machine.video().characterRam()[0x124], 0) << "长度寄存器是字节数减 1，所以只搬 4 个字节";
}

TEST(Sb2000Machine, PowerOnRunsTheResetVectorAndEachFrameTakesFiftyHertzOfLines) {
  Sb2000Machine machine(syntheticBios());
  EXPECT_FALSE(machine.isPoweredOn());
  machine.powerOn();
  EXPECT_TRUE(machine.isPoweredOn());
  EXPECT_EQ(machine.frame().width, 0U) << "还没有完成过画面";
  machine.runFrame();
  EXPECT_EQ(machine.frame().width, 256U);
  EXPECT_EQ(machine.frame().height, 240U);
  EXPECT_EQ(peek(machine, 0x4400), 0x68) << "程序向 $4031 写过 $65、$76";
  EXPECT_EQ(machine.ppu().mask(), 0x08);
  const std::uint8_t counter = machine.internalRam().read(0);
  machine.runFrame();
  EXPECT_NE(machine.internalRam().read(0), counter) << "$C000 的程序在继续运行";
  EXPECT_EQ(machine.ppu().frameCount(), 2U);
  // 画面在第 240 行开始时发布：第一帧经过 240 行，第二帧再经过 312 行；每行 341 点、每点 5 个主时钟周期，CPU 每周期 15 个主时钟周期。
  const double expectedCycles = (240.0 + 312.0) * 341.0 * 5.0 / 15.0;
  EXPECT_NEAR(static_cast<double>(machine.cpuRegisters().cycles), expectedCycles, 100.0);
}

TEST(Sb2000Machine, SoftResetKeepsRamAndHardResetClearsIt) {
  Sb2000Machine machine(syntheticBios());
  machine.powerOn();
  machine.runFrame();
  poke(machine, 0x0010, 0xAB);
  machine.softReset();
  EXPECT_EQ(peek(machine, 0x0010), 0xAB);
  machine.powerOff();
  EXPECT_FALSE(machine.isPoweredOn());
  machine.powerOn();
  EXPECT_EQ(peek(machine, 0x0010), 0x00);
}

TEST(Sb2000Machine, DebuggerExposesTheBoardMemorySpaces) {
  Sb2000Machine machine(syntheticBios());
  hw::MachineDebugger* debugger = machine.debugger();
  ASSERT_NE(debugger, nullptr);
  const auto spaces = debugger->memorySpaces();
  std::set<std::string> ids;
  for (const auto& space : spaces) ids.insert(space.id);
  for (const char* id : {"cpu", "ppu", "board.ram", "board.externalRam", "board.bios", "board.extendedRam", "board.characterRam", "board.extendedVram", "chip.oam", "chip.palette"}) EXPECT_TRUE(ids.count(id) == 1) << id;
  const std::uint8_t byte[] = {1};
  EXPECT_THROW(debugger->writeMemory("board.bios", 0, byte), ember::shared::CoreError);
  const auto target = debugger->resolve("cpu", 0xC000);
  EXPECT_EQ(target.kind, "rom");
  EXPECT_EQ(target.region, "board.bios");
  EXPECT_EQ(target.offset, 0x4000U);
}

TEST(Sb2000Machine, TheKeyboardBelongsToTheMachineAndThePadPortsAreSetDirectly) {
  Sb2000Machine machine(syntheticBios());
  EXPECT_EQ(machine.padPortCount(), 2U);
  EXPECT_TRUE(machine.keyboardIsMachineInput()) << "6576 模式下键盘属于机器";
  EXPECT_NO_THROW(machine.setButtons(1, hw::pad_button::START));
  EXPECT_THROW(machine.setButtons(2, 0), ember::shared::CoreError);
}

/** 一段 D6 码表的语音码流：命令字节、两个相同的浊音帧、结束帧（字段按位反序打包）。 */
std::vector<std::uint8_t> voicedSpeechStream() {
  const std::vector<std::pair<unsigned, unsigned>> frame = {{8, 4}, {0, 1}, {40, 7}, {30, 6}, {23, 6}, {20, 5}, {17, 5}, {8, 4}, {8, 4}, {8, 4}, {4, 3}, {4, 3}, {4, 3}};
  std::vector<std::pair<unsigned, unsigned>> fields = frame;
  fields.insert(fields.end(), frame.begin(), frame.end());
  fields.insert(fields.end(), frame.begin(), frame.end());
  fields.emplace_back(15U, 4U);
  std::vector<unsigned> bits;
  for (const auto& [value, count] : fields) {
    for (unsigned index = 0; index < count; ++index) bits.push_back((value >> (count - 1 - index)) & 1U);
  }
  std::vector<std::uint8_t> bytes = {0x0A};
  bytes.resize(1 + (bits.size() + 7) / 8, 0);
  for (std::size_t index = 0; index < bits.size(); ++index) bytes[1 + index / 8] = static_cast<std::uint8_t>(bytes[1 + index / 8] | (bits[index] << (index % 8)));
  return bytes;
}

TEST(Sb2000Machine, TheSpeechPortSynthesizesAStreamIntoTheMachineAudio) {
  Sb2000Machine machine(syntheticBios());
  machine.powerOn();
  machine.runFrame();
  std::vector<float> samples;
  machine.takeAudio(samples);
  EXPECT_EQ(peek(machine, 0x4302), 0x80) << "缓冲未满，没有播放结束";
  for (const std::uint8_t byte : voicedSpeechStream()) poke(machine, 0x4302, byte);
  for (unsigned frame = 0; frame < 4; ++frame) machine.runFrame();
  machine.takeAudio(samples);
  float peakLevel = 0;
  for (const float sample : samples) peakLevel = std::max(peakLevel, std::fabs(sample));
  EXPECT_GT(peakLevel, 0.01F) << "语音经 PSG 混进了机器的声音输出";
  EXPECT_EQ(peek(machine, 0x4302) & 0x0F, 0x0F) << "两帧合成完，读到结束帧";
  poke(machine, 0x4302, 0x0F);
  EXPECT_EQ(peek(machine, 0x4302), 0x80);
}

TEST(Sb2000Machine, RealBiosBootsToAPicture) {
  std::vector<std::uint8_t> bios = realBios();
  if (bios.empty()) GTEST_SKIP() << "没有找到 reference/rom/SB2K_BIOS.NES";
  Sb2000Machine machine(std::move(bios));
  machine.powerOn();
  for (int frame = 0; frame < 300; ++frame) machine.runFrame();
  const hw::FrameBuffer picture = machine.frame();
  ASSERT_EQ(picture.width, 256U);
  ASSERT_EQ(picture.height, 240U);
  std::set<std::uint32_t> colors(picture.pixels.begin(), picture.pixels.end());
  EXPECT_GT(colors.size(), 1U) << "BIOS 开机后画面不应该是纯色";
  EXPECT_EQ(peek(machine, 0x4400), 0x68) << "BIOS 完成了 6576 的初始设置";
  if (const std::string dump = dumpPath(); !dump.empty()) dumpFrame(picture, dump);
}

TEST(Sb2000Machine, RealBiosBootsTheSystemDemoDiskFromTheFloppyDrive) {
  std::vector<std::uint8_t> bios = realBios();
  std::vector<std::uint8_t> disk = realFloppy(std::filesystem::path(u8"reference/floppy-images/sb2000/FLPPY0系统演示.img"));
  if (bios.empty() || disk.empty()) GTEST_SKIP() << "没有找到 BIOS 或系统演示盘";
  MotorRecorder recorder;
  Sb2000Machine machine(std::move(bios), nullptr, &recorder);
  ASSERT_EQ(machine.floppyDriveCount(), 1U);
  machine.floppyDrive(0)->insertImage(std::move(disk), false, "demo.img");
  machine.powerOn();
  // BIOS 从软盘读入 SB-Win98 需要约 1500 帧；再跑到第 3000 帧时已经在显示演示画面。
  std::size_t highestCylinder = 0;
  for (int frame = 0; frame < 3000; ++frame) {
    machine.runFrame();
    highestCylinder = std::max<std::size_t>(highestCylinder, machine.floppyDrive(0)->cylinder());
  }
  const hw::FrameBuffer picture = machine.frame();
  if (const std::string dump = dumpPath(); !dump.empty()) dumpFrame(picture, dump + ".demo.ppm");
  EXPECT_GT(highestCylinder, 40U) << "BIOS 把磁头移到了盘的后部读系统";
  ASSERT_FALSE(recorder.changes.empty()) << "读盘时马达开关要通知接收方";
  EXPECT_EQ(recorder.changes.front(), (std::pair<unsigned, bool>{0, true})) << "第一次通知是驱动器 0 的马达开始转";
  const std::set<std::uint32_t> colors(picture.pixels.begin(), picture.pixels.end());
  EXPECT_GT(colors.size(), 4U) << "演示画面有多种颜色";
}

TEST(Sb2000Machine, RealGameCollectionDiskStartsAGameInFcMode) {
  std::vector<std::uint8_t> bios = realBios();
  std::vector<std::uint8_t> disk = realFloppy(std::filesystem::path(u8"reference/floppy-images/sb2000/FLPPY18游戏集锦001热血格斗热血排球.img"));
  const std::string dump = dumpPath();
  if (bios.empty() || disk.empty()) GTEST_SKIP() << "没有找到 BIOS 或游戏集锦盘";
  Sb2000Machine machine(std::move(bios));
  machine.floppyDrive(0)->insertImage(std::move(disk), false, "collection.img");
  machine.powerOn();
  // 软盘里的启动程序（GAME.EXE 只是加载器，游戏代码在没有文件名的扇区里）把游戏集锦的菜单装进扩展 RAM 后写 $4301，约在第 1650 帧进入 FC 模式。
  for (int frame = 0; frame < 3000 && !machine.fcMode(); ++frame) machine.runFrame();
  ASSERT_TRUE(machine.fcMode()) << "启动后应该进入 FC 兼容模式";
  for (int frame = 0; frame < 120; ++frame) machine.runFrame();
  const hw::FrameBuffer menu = machine.frame();
  if (!dump.empty()) dumpFrame(menu, dump + ".collection-menu.ppm");
  const std::set<std::uint32_t> menuColors(menu.pixels.begin(), menu.pixels.end());
  EXPECT_GT(menuColors.size(), 3U) << "游戏集锦的菜单是多色画面";
  // 在菜单里按开始，运行选中的游戏（热血格斗：先出厂商标志，再进入剧情画面）。
  machine.setButtons(0, hw::pad_button::START);
  for (int frame = 0; frame < 10; ++frame) machine.runFrame();
  machine.setButtons(0, 0);
  for (int frame = 0; frame < 760; ++frame) machine.runFrame();
  const hw::FrameBuffer game = machine.frame();
  if (!dump.empty()) dumpFrame(game, dump + ".collection-game.ppm");
  const std::set<std::uint32_t> gameColors(game.pixels.begin(), game.pixels.end());
  EXPECT_GT(gameColors.size(), 8U) << "游戏画面是多色的（图案页经 MMC3 映射取自扩展显存）";
  EXPECT_NE(menuColors, gameColors);
  EXPECT_TRUE(machine.fcMode());
}

/** 按一下键：按下、跑几帧、松开、再跑几帧。 */
void tapKey(Sb2000Machine& machine, const char* code) {
  machine.input(hw::KeyInput{code, true});
  for (int frame = 0; frame < 4; ++frame) machine.runFrame();
  machine.input(hw::KeyInput{code, false});
  for (int frame = 0; frame < 4; ++frame) machine.runFrame();
}

/** 在 SB-DOS 的命令行里输入一个由字母组成的命令并回车。 */
void typeCommand(Sb2000Machine& machine, const std::string& command) {
  for (const char letter : command) {
    const std::string code = std::string("Key") + static_cast<char>(std::toupper(static_cast<unsigned char>(letter)));
    tapKey(machine, code.c_str());
  }
  tapKey(machine, "Enter");
}

TEST(Sb2000Machine, RealMemtestDiskRunsUnderSbDos) {
  std::vector<std::uint8_t> bios = realBios();
  std::vector<std::uint8_t> disk = realFloppy(std::filesystem::path(u8"apps/sb2000/memtest/build/sb2000/MEMTEST.img"));
  const std::string dump = dumpPath();
  if (bios.empty() || disk.empty() || dump.empty()) GTEST_SKIP() << "只在设置了 EMBER_SB2000_DUMP 且有 BIOS 与 MEMTEST.img 时运行（人工验收用）";
  Sb2000Machine machine(std::move(bios));
  machine.floppyDrive(0)->insertImage(std::move(disk), false, "MEMTEST.img");
  machine.powerOn();
  for (int frame = 0; frame < 2400; ++frame) machine.runFrame();
  dumpFrame(machine.frame(), dump + ".memtest0.ppm");
  // SB-DOS 启动后先问日期与时间，回车跳过。
  tapKey(machine, "Enter");
  tapKey(machine, "Enter");
  for (int frame = 0; frame < 120; ++frame) machine.runFrame();
  typeCommand(machine, "memtest");
  for (int frame = 0; frame < 1800; ++frame) machine.runFrame();
  dumpFrame(machine.frame(), dump + ".memtest1.ppm");
  tapKey(machine, "Space");
  for (int frame = 0; frame < 300; ++frame) machine.runFrame();
  dumpFrame(machine.frame(), dump + ".memtest2.ppm");
}

/** 在 FAT12 软盘镜像的根目录里按 8.3 名称找文件，沿簇链读出内容；找不到时返回空。 */
std::vector<std::uint8_t> readRootFile(const std::vector<std::uint8_t>& image, const char* name83) {
  constexpr std::size_t kSector = 512;
  const std::size_t fatOffset = 1 * kSector;
  const std::size_t rootOffset = 19 * kSector;
  const std::size_t dataOffset = 33 * kSector;
  for (std::size_t entry = 0; entry < 224; ++entry) {
    const std::size_t base = rootOffset + entry * 32;
    if (image.size() < base + 32 || image[base] == 0) break;
    if (std::string(reinterpret_cast<const char*>(&image[base]), 11) != name83) continue;
    std::size_t cluster = image[base + 26] | (static_cast<std::size_t>(image[base + 27]) << 8);
    const std::size_t size = image[base + 28] | (static_cast<std::size_t>(image[base + 29]) << 8) | (static_cast<std::size_t>(image[base + 30]) << 16);
    std::vector<std::uint8_t> content;
    while (cluster >= 2 && cluster < 0xFF8 && content.size() < size) {
      const std::size_t start = dataOffset + (cluster - 2) * kSector;
      content.insert(content.end(), image.begin() + static_cast<std::ptrdiff_t>(start), image.begin() + static_cast<std::ptrdiff_t>(start + kSector));
      const std::size_t pair = fatOffset + cluster * 3 / 2;
      const std::size_t raw = image[pair] | (static_cast<std::size_t>(image[pair + 1]) << 8);
      cluster = (cluster & 1) ? (raw >> 4) : (raw & 0xFFF);
    }
    content.resize(std::min(content.size(), size));
    return content;
  }
  return {};
}

TEST(Sb2000Machine, RealHwtestDiskRunsUnderSbDos) {
  std::vector<std::uint8_t> bios = realBios();
  std::vector<std::uint8_t> disk = realFloppy(std::filesystem::path(u8"apps/sb2000/hwtest/build/sb2000/HWTEST.img"));
  const std::string dump = dumpPath();
  if (bios.empty() || disk.empty() || dump.empty()) GTEST_SKIP() << "只在设置了 EMBER_SB2000_DUMP 且有 BIOS 与 HWTEST.img 时运行（人工验收用）";
  Sb2000Machine machine(std::move(bios));
  machine.floppyDrive(0)->insertImage(std::move(disk), false, "HWTEST.img");
  machine.powerOn();
  for (int frame = 0; frame < 2400; ++frame) machine.runFrame();
  tapKey(machine, "Enter");
  tapKey(machine, "Enter");
  for (int frame = 0; frame < 120; ++frame) machine.runFrame();
  typeCommand(machine, "hwtest");
  for (int frame = 0; frame < 300; ++frame) machine.runFrame();
  dumpFrame(machine.frame(), dump + ".hwtest0.ppm");
  // 每一项先显示名字并等键：回车运行这一项；共 18 项，每项最多等 20 秒（1000 帧）；多按一次回车用来确认最后一项没有被漏掉。
  for (int item = 0; item < 19; ++item) {
    tapKey(machine, "Enter");
    for (int frame = 0; frame < 1000; ++frame) machine.runFrame();
  }
  dumpFrame(machine.frame(), dump + ".hwtest1.ppm");
  const auto report = readRootFile(machine.floppyDrive(0)->image(), "HWTEST  TXT");
  std::ofstream out(dump + ".hwtest.txt", std::ios::binary);
  out.write(reinterpret_cast<const char*>(report.data()), static_cast<std::streamsize>(report.size()));
  // 看图场景 V1-V8：回车进入，每个场景存一张画面，任意键翻页。
  for (int scene = 1; scene <= 8; ++scene) {
    for (int frame = 0; frame < 200; ++frame) machine.runFrame();
    dumpFrame(machine.frame(), dump + ".hwtestv" + std::to_string(scene) + ".ppm");
    tapKey(machine, "Space");
  }
}

/** 把一个文件放进 FAT12 软盘镜像的根目录（1.44MB：每簇 1 扇区、根目录从第 19 扇区起、数据从第 33 扇区起、两份 FAT 各 9 扇区）；空间不够时返回 false。 */
bool addRootFile(std::vector<std::uint8_t>& image, const char* name83, const std::vector<std::uint8_t>& content) {
  constexpr std::size_t kSector = 512;
  constexpr std::size_t kFirstFat = 1 * kSector;
  constexpr std::size_t kSecondFat = 10 * kSector;
  constexpr std::size_t kRoot = 19 * kSector;
  constexpr std::size_t kData = 33 * kSector;
  constexpr std::size_t kClusters = 2847;
  const auto fatGet = [&](std::size_t cluster) {
    const std::size_t pair = kFirstFat + cluster * 3 / 2;
    const std::size_t raw = image[pair] | (static_cast<std::size_t>(image[pair + 1]) << 8);
    return (cluster & 1) ? (raw >> 4) : (raw & 0xFFF);
  };
  const auto fatSet = [&](std::size_t cluster, std::size_t value) {
    for (const std::size_t base : {kFirstFat, kSecondFat}) {
      const std::size_t pair = base + cluster * 3 / 2;
      if (cluster & 1) {
        image[pair] = static_cast<std::uint8_t>((image[pair] & 0x0F) | ((value & 0x0F) << 4));
        image[pair + 1] = static_cast<std::uint8_t>(value >> 4);
      } else {
        image[pair] = static_cast<std::uint8_t>(value & 0xFF);
        image[pair + 1] = static_cast<std::uint8_t>((image[pair + 1] & 0xF0) | (value >> 8));
      }
    }
  };
  std::size_t entry = 0;
  while (entry < 224 && image[kRoot + entry * 32] != 0 && image[kRoot + entry * 32] != 0xE5) ++entry;
  if (entry == 224) return false;
  const std::size_t needed = (content.size() + kSector - 1) / kSector;
  std::vector<std::size_t> chain;
  for (std::size_t cluster = 2; cluster < kClusters + 2 && chain.size() < needed; ++cluster) {
    if (fatGet(cluster) == 0) chain.push_back(cluster);
  }
  if (chain.size() < needed) return false;
  for (std::size_t index = 0; index < chain.size(); ++index) {
    fatSet(chain[index], index + 1 < chain.size() ? chain[index + 1] : 0xFFF);
    const std::size_t from = index * kSector;
    const std::size_t count = std::min(kSector, content.size() - from);
    std::copy_n(content.begin() + static_cast<std::ptrdiff_t>(from), count, image.begin() + static_cast<std::ptrdiff_t>(kData + (chain[index] - 2) * kSector));
  }
  const std::size_t base = kRoot + entry * 32;
  std::fill_n(image.begin() + static_cast<std::ptrdiff_t>(base), 32, std::uint8_t{0});
  std::copy_n(name83, 11, image.begin() + static_cast<std::ptrdiff_t>(base));
  image[base + 11] = 0x20;
  image[base + 26] = static_cast<std::uint8_t>(chain.front() & 0xFF);
  image[base + 27] = static_cast<std::uint8_t>(chain.front() >> 8);
  for (unsigned byte = 0; byte < 4; ++byte) image[base + 28 + byte] = static_cast<std::uint8_t>((content.size() >> (8 * byte)) & 0xFF);
  return true;
}

/** 在 SB-DOS 的命令行里输入一行文字（字母、数字、空格与小数点）并回车。 */
void typeLine(Sb2000Machine& machine, const std::string& text) {
  for (const char letter : text) {
    std::string code;
    if (letter == ' ') code = "Space";
    else if (letter == '.') code = "Period";
    else if (letter >= '0' && letter <= '9') code = std::string("Digit") + letter;
    else code = std::string("Key") + static_cast<char>(std::toupper(static_cast<unsigned char>(letter)));
    tapKey(machine, code.c_str());
  }
  tapKey(machine, "Enter");
}

/** 把超级马里奥兄弟放进 imagePath 的软盘，在 SB-DOS 下输入 command 运行，标题画面按开始后应进入第一关。 */
void expectMarioRunsUnderSbDos(const std::filesystem::path& imagePath, const std::string& command, const char* dumpName) {
  std::vector<std::uint8_t> bios = realBios();
  std::vector<std::uint8_t> disk = realFloppy(imagePath);
  const std::vector<std::uint8_t> game = realFloppy(std::filesystem::path(u8"reference/nes-games/超级马里奥兄弟(日美).nes"));
  const std::string dump = dumpPath();
  if (bios.empty() || disk.empty() || game.empty()) GTEST_SKIP() << "没有找到 BIOS、" << imagePath.string() << "（用 fcdev disk 生成）或 reference/nes-games 里的超级马里奥兄弟";
  ASSERT_TRUE(addRootFile(disk, "MARIO   NES", game));
  Sb2000Machine machine(std::move(bios));
  machine.floppyDrive(0)->insertImage(std::move(disk), false, "game.img");
  machine.powerOn();
  for (int frame = 0; frame < 2400; ++frame) machine.runFrame();
  tapKey(machine, "Enter");
  tapKey(machine, "Enter");
  for (int frame = 0; frame < 120; ++frame) machine.runFrame();
  EXPECT_FALSE(machine.fcMode());
  typeLine(machine, command);
  for (int frame = 0; frame < 1500 && !machine.fcMode(); ++frame) machine.runFrame();
  if (!dump.empty()) dumpFrame(machine.frame(), dump + "." + dumpName + "0.ppm");
  ASSERT_TRUE(machine.fcMode()) << "程序装入游戏后写 $4301 进入 FC 模式";
  for (int frame = 0; frame < 300; ++frame) machine.runFrame();
  EXPECT_EQ(peek(machine, 0x0770), 0) << "$0770 是马里奥的运行模式，标题画面为 0";
  machine.setButtons(0, hw::pad_button::START);
  for (int frame = 0; frame < 10; ++frame) machine.runFrame();
  machine.setButtons(0, 0);
  for (int frame = 0; frame < 400; ++frame) machine.runFrame();
  const hw::FrameBuffer picture = machine.frame();
  if (!dump.empty()) dumpFrame(picture, dump + "." + dumpName + "1.ppm");
  EXPECT_EQ(peek(machine, 0x0770), 1) << "按开始后进入游戏";
  const std::set<std::uint32_t> colors(picture.pixels.begin(), picture.pixels.end());
  EXPECT_GT(colors.size(), 4U) << "第一关画面是多色的";
}

TEST(Sb2000Machine, RealPlayRunsAnNesGameUnderSbDos) {
  expectMarioRunsUnderSbDos(std::filesystem::path(u8"apps/sb2000/examples/play/build/sb2000/PLAY.img"), "play mario.nes", "play");
}

TEST(Sb2000Machine, RealRunnesRunsAnNesGameUnderSbDos) {
  expectMarioRunsUnderSbDos(std::filesystem::path(u8"apps/sb2000/runnes/build/sb2000/RUNNES.img"), "runnes mario.nes", "runnes");
}

}  // namespace
