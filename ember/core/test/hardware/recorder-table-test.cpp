/*
 * ------------------------------------------------------------------------
 * 名称：recorder-table-test.cpp
 * 说明：记录表与反汇编的测试：用 nestest 运行时，指令跟踪的每一行与 nestest 日志逐字一致（去掉 PPU 列）；
 *       唯一地址、行数上限、调用记录、I/O 记录、中断记录与记录数量上限。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：nestest 日志共 8991 行，覆盖全部官方指令与 nestest 用到的非官方指令，以及各种寻址方式的有效地址与取值注释。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <fstream>
#include <iterator>
#include <regex>
#include <string>
#include <utility>
#include <vector>

#include "contracts/hardware/bus-device.hpp"
#include "contracts/hardware/hardware-error.hpp"
#include "logic/hardware/bus/mapped-bus.hpp"
#include "logic/hardware/cpu-6502/cpu-6502.hpp"
#include "logic/hardware/debug/recorder-table.hpp"
#include "logic/hardware/lines/wired-or-line.hpp"
#include "logic/hardware/scheduler/master-scheduler.hpp"
#include "shared/errors/core-error.hpp"

#ifndef EMBER_TEST_DATA_DIR
#error "EMBER_TEST_DATA_DIR 由 CMake 传入：测试数据目录"
#endif

namespace {

namespace hw = ember::contracts::hardware;
using ember::logic::hardware::RecorderTable;

constexpr std::size_t NES_HEADER_SIZE = 16;
constexpr std::size_t PRG_SIZE = 16 * 1024;

std::vector<std::uint8_t> readBinary(const std::string& path) {
  std::ifstream file(path, std::ios::binary);
  return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

std::vector<std::string> readLines(const std::string& path) {
  std::ifstream file(path);
  std::vector<std::string> lines;
  std::string line;
  while (std::getline(file, line)) {
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (!line.empty()) lines.push_back(line);
  }
  return lines;
}

/** 2K 内存（镜像到 $1FFF）。 */
class Ram final : public hw::BusDevice {
 public:
  hw::BusRead read(std::uint16_t address) override { return hw::driveBus(bytes[address & 0x07FF]); }
  void write(std::uint16_t address, std::uint8_t value) override { bytes[address & 0x07FF] = value; }
  std::uint8_t bytes[0x0800]{};
};

/** 16K 的 PRG-ROM（镜像到 $8000-$FFFF）；复位向量读成 $C000。 */
class Rom final : public hw::BusDevice {
 public:
  explicit Rom(std::vector<std::uint8_t> prg) : prg_(std::move(prg)) {}
  hw::BusRead read(std::uint16_t address) override {
    if (address == 0xFFFC) return hw::driveBus(0x00);
    if (address == 0xFFFD) return hw::driveBus(0xC0);
    return hw::driveBus(prg_[address & (PRG_SIZE - 1)]);
  }
  void write(std::uint16_t /*address*/, std::uint8_t /*value*/) override {}
  [[nodiscard]] std::uint8_t byteAt(std::uint16_t address) const { return prg_[address & (PRG_SIZE - 1)]; }

 private:
  std::vector<std::uint8_t> prg_;
};

/** 带 RAM 与 PRG 的 CPU，用来跑 nestest。 */
struct Rig {
  Rig(const std::vector<std::uint8_t>& rom) : prg(std::vector<std::uint8_t>(rom.begin() + NES_HEADER_SIZE, rom.begin() + NES_HEADER_SIZE + PRG_SIZE)), cpu(bus, scheduler, hw::ClockDomain{12}, irq, nmi, rdy) {
    bus.mount(0x0000, 0x1FFF, ram);
    bus.mount(0x8000, 0xFFFF, prg);
    cpu.reset(true);
  }

  std::uint8_t peek(std::uint16_t address) const {
    if (address < 0x2000) return ram.bytes[address & 0x07FF];
    if (address >= 0x8000) return prg.byteAt(address);
    // 日志里 $4000-$401F 的取值是 nestest 作者的模拟器读出的，这里没有 APU 与手柄，照日志给 FF。
    if (address >= 0x4000 && address < 0x4020) return 0xFF;
    return 0;
  }

  ember::logic::hardware::MasterScheduler scheduler;
  ember::logic::hardware::CpuAddressBus bus;
  ember::logic::hardware::WiredOrIrqLine irq;
  ember::logic::hardware::WiredOrIrqLine nmi;
  ember::logic::hardware::WiredOrRdyLine rdy;
  Ram ram;
  Rom prg;
  ember::logic::hardware::Cpu6502 cpu;
};

/** 小程序：RAM 里放几条指令，供记录表测试，peek 直接读这块内存。 */
class SmallMemory {
 public:
  SmallMemory() : bytes(0x10000, 0) {}
  std::uint8_t peek(std::uint16_t address) const { return bytes[address]; }
  std::vector<std::uint8_t> bytes;
};

hw::CpuRegisters registersAt(std::uint16_t pc, std::uint64_t cycles = 0) {
  hw::CpuRegisters registers;
  registers.pc = pc;
  registers.status = 0x24;
  registers.sp = 0xFD;
  registers.cycles = cycles;
  return registers;
}

TEST(RecorderTable, InstructionTraceMatchesTheNestestLogLineByLine) {
  const std::string directory = std::string(EMBER_TEST_DATA_DIR) + "/nestest/";
  const auto rom = readBinary(directory + "nestest.nes");
  const auto log = readLines(directory + "nestest.log");
  ASSERT_GE(rom.size(), NES_HEADER_SIZE + PRG_SIZE);
  ASSERT_EQ(log.size(), 8991U);

  Rig rig(rom);
  RecorderTable table([&rig](std::uint16_t address) { return rig.peek(address); });
  hw::RecorderSpec spec;
  spec.kind = hw::RecorderKind::Instruction;
  spec.maxLines = RecorderTable::MAX_LINES;
  const auto id = table.start(spec);
  ASSERT_TRUE(table.needsInstructionHook());
  for (std::size_t index = 0; index < log.size(); ++index) {
    table.onInstruction(rig.cpu.registers());
    rig.cpu.run(1);
  }
  const auto output = table.stop(id);
  ASSERT_EQ(output.lines.size(), log.size());
  EXPECT_FALSE(output.truncated);
  static const std::regex ppuColumn(R"(PPU:[ 0-9]+,[ 0-9]+ )");
  for (std::size_t index = 0; index < log.size(); ++index) {
    ASSERT_EQ(output.lines[index], std::regex_replace(log[index], ppuColumn, "")) << "第 " << (index + 1) << " 行不一致";
  }
}

TEST(RecorderTable, UniqueRecordsEachAddressOnceAndMaxLinesTruncates) {
  SmallMemory memory;
  // $0200: INX、JMP $0200
  memory.bytes[0x200] = 0xE8;
  memory.bytes[0x201] = 0x4C;
  memory.bytes[0x202] = 0x00;
  memory.bytes[0x203] = 0x02;
  RecorderTable table([&memory](std::uint16_t address) { return memory.peek(address); });

  hw::RecorderSpec unique;
  unique.unique = true;
  const auto uniqueId = table.start(unique);
  hw::RecorderSpec limited;
  limited.maxLines = 3;
  const auto limitedId = table.start(limited);
  for (int round = 0; round < 5; ++round) {
    table.onInstruction(registersAt(0x200));
    table.onInstruction(registersAt(0x201));
  }
  const auto uniqueOutput = table.stop(uniqueId);
  EXPECT_EQ(uniqueOutput.lines.size(), 2U);
  EXPECT_FALSE(uniqueOutput.truncated);
  const auto limitedOutput = table.stop(limitedId);
  EXPECT_EQ(limitedOutput.lines.size(), 3U);
  EXPECT_TRUE(limitedOutput.truncated);
  EXPECT_FALSE(table.needsInstructionHook());
}

TEST(RecorderTable, CallLogRecordsRegistersStackAndMemoryAtTheAddress) {
  SmallMemory memory;
  memory.bytes[0x1FE] = 0xAA;
  memory.bytes[0x1FF] = 0xBB;
  memory.bytes[0x100] = 0xCC;
  memory.bytes[0x300] = 0x11;
  memory.bytes[0x301] = 0x22;
  RecorderTable table([&memory](std::uint16_t address) { return memory.peek(address); });
  hw::RecorderSpec spec;
  spec.kind = hw::RecorderKind::Call;
  spec.address = 0x8123;
  spec.stackBytes = 3;
  spec.ranges.push_back({0x300, 2});
  const auto id = table.start(spec);

  auto registers = registersAt(0x8123, 77);
  registers.a = 1;
  registers.x = 2;
  registers.y = 3;
  registers.sp = 0xFD;
  table.onInstruction(registersAt(0x8000));
  table.onInstruction(registers);
  const auto output = table.stop(id);
  ASSERT_EQ(output.lines.size(), 1U);
  // 栈顶从 SP 之上开始，到页尾回到页首
  EXPECT_EQ(output.lines[0], "8123  A:01 X:02 Y:03 P:24 SP:FD CYC:77 | STACK AA BB CC | 0300: 11 22");
}

TEST(RecorderTable, IoRecordsRegisterAccessesAndMapperWritesWithTheCurrentInstruction) {
  SmallMemory memory;
  RecorderTable table([&memory](std::uint16_t address) { return memory.peek(address); });
  hw::RecorderSpec spec;
  spec.kind = hw::RecorderKind::Io;
  const auto id = table.start(spec);
  ASSERT_TRUE(table.recordsIo());
  table.onInstruction(registersAt(0xC005, 1234));
  table.onIo(hw::AccessKind::Read, 0x2002, 0x80);
  table.onIo(hw::AccessKind::Write, 0x4015, 0x1F);
  table.onIo(hw::AccessKind::Write, 0x8000, 0x03);
  // 不记：RAM、PRG-RAM、对 ROM 的读
  table.onIo(hw::AccessKind::Write, 0x0010, 1);
  table.onIo(hw::AccessKind::Read, 0x6000, 1);
  table.onIo(hw::AccessKind::Read, 0x8000, 1);
  const auto output = table.stop(id);
  EXPECT_EQ(output.lines, (std::vector<std::string>{"R $2002 = 80  PC:C005 CYC:1234", "W $4015 = 1F  PC:C005 CYC:1234", "W $8000 = 03  PC:C005 CYC:1234"}));
  EXPECT_FALSE(table.recordsIo());
}

TEST(RecorderTable, InterruptRecordsTheKindTheReturnAddressAndTheHandler) {
  SmallMemory memory;
  RecorderTable table([&memory](std::uint16_t address) { return memory.peek(address); });
  hw::RecorderSpec spec;
  spec.kind = hw::RecorderKind::Irq;
  const auto id = table.start(spec);
  ASSERT_TRUE(table.recordsIrq());
  EXPECT_FALSE(table.needsInstructionHook());
  table.onInterrupt(hw::InterruptKind::Nmi, 0xC0AB, 0xC100, 500);
  table.onInterrupt(hw::InterruptKind::Irq, 0xC0AD, 0xC200, 900);
  table.onInterrupt(hw::InterruptKind::Brk, 0xC0AF, 0xC200, 950);
  const auto output = table.stop(id);
  EXPECT_EQ(output.lines, (std::vector<std::string>{"NMI  return:C0AB handler:C100 CYC:500", "IRQ  return:C0AD handler:C200 CYC:900", "BRK  return:C0AF handler:C200 CYC:950"}));
}

TEST(RecorderTable, LimitsTheNumberOfRecordsAndRejectsUnknownIds) {
  SmallMemory memory;
  RecorderTable table([&memory](std::uint16_t address) { return memory.peek(address); });
  std::vector<std::uint32_t> ids;
  for (std::size_t index = 0; index < RecorderTable::MAX_RECORDERS; ++index) ids.push_back(table.start(hw::RecorderSpec{}));
  try {
    static_cast<void>(table.start(hw::RecorderSpec{}));
    FAIL() << "应该抛出 hardware.recorderLimit";
  } catch (const ember::shared::CoreError& error) {
    EXPECT_EQ(error.code(), hw::ERROR_RECORDER_LIMIT);
  }
  static_cast<void>(table.stop(ids.front()));
  try {
    static_cast<void>(table.stop(ids.front()));
    FAIL() << "应该抛出 hardware.noRecorder";
  } catch (const ember::shared::CoreError& error) {
    EXPECT_EQ(error.code(), hw::ERROR_NO_RECORDER);
  }
  EXPECT_NO_THROW(static_cast<void>(table.start(hw::RecorderSpec{})));
}

}  // namespace
