/*
 * ------------------------------------------------------------------------
 * 名称：cpu-6502-test.cpp
 * 说明：6502 CPU 核的测试：复位序列、各寻址方式的周期数与总线访问顺序（空读、假写）、标志位与运算、栈与跳转、
 *       BRK、IRQ 与 NMI 的采样时机和向量劫持、RDY 暂停、分支周期。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：全部官方指令与非官方指令的逐条核对由 nestest 日志比对完成，见 nestest-test.cpp。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "contracts/hardware/hardware-error.hpp"
#include "shared/errors/core-error.hpp"
#include "test-support.hpp"

namespace {

using ember::shared::CoreError;
using ember::test::CpuRig;
using ember::test::LineEvent;
namespace hw = ember::contracts::hardware;

constexpr std::uint8_t NOP = 0xEA;

/** 状态寄存器的位。 */
constexpr std::uint8_t STATUS_C = 0x01;
constexpr std::uint8_t STATUS_Z = 0x02;
constexpr std::uint8_t STATUS_I = 0x04;
constexpr std::uint8_t STATUS_D = 0x08;
constexpr std::uint8_t STATUS_B = 0x10;
constexpr std::uint8_t STATUS_U = 0x20;
constexpr std::uint8_t STATUS_V = 0x40;
constexpr std::uint8_t STATUS_N = 0x80;

/** 在地址起填充 NOP。 */
void fillNops(CpuRig& rig, std::uint16_t from, int count) {
  for (int i = 0; i < count; ++i) rig.memory.poke(static_cast<std::uint16_t>(from + i), NOP);
}

/** 把程序放在 $0200 并从那里开始。 */
void startProgram(CpuRig& rig, std::initializer_list<std::uint8_t> program) {
  rig.memory.load(0x0200, program);
  rig.startAt(0x0200);
}

/** 访问记录的简写，便于比较。 */
struct Seen {
  bool write;
  std::uint16_t address;
  std::uint8_t value;
  bool operator==(const Seen&) const = default;
};

std::vector<Seen> seenOf(const CpuRig& rig) {
  std::vector<Seen> seen;
  for (const auto& access : rig.memory.log) seen.push_back(Seen{access.write, access.address, access.value});
  return seen;
}

// ---------------------------------------------------------------- 复位与时间

TEST(Cpu6502Reset, PowerOnSequenceTakesSevenCyclesAndLoadsVector) {
  CpuRig rig;
  rig.memory.poke(0xFFFC, 0x34);
  rig.memory.poke(0xFFFD, 0x12);

  rig.cpu.reset(true);

  const auto regs = rig.cpu.registers();
  EXPECT_EQ(regs.pc, 0x1234);
  EXPECT_EQ(regs.a, 0);
  EXPECT_EQ(regs.x, 0);
  EXPECT_EQ(regs.y, 0);
  EXPECT_EQ(regs.sp, 0xFD);
  EXPECT_EQ(regs.status, STATUS_U | STATUS_I);
  EXPECT_EQ(regs.cycles, 7U);
  EXPECT_EQ(rig.scheduler.now(), 7 * 12);
  // 两次空读、三次只读不写的“压栈”、两次读向量。
  const std::vector<Seen> expected{{false, 0x0000, 0}, {false, 0x0000, 0}, {false, 0x0100, 0}, {false, 0x01FF, 0}, {false, 0x01FE, 0}, {false, 0xFFFC, 0x34}, {false, 0xFFFD, 0x12}};
  EXPECT_EQ(seenOf(rig), expected);
}

TEST(Cpu6502Reset, SoftResetKeepsRegistersAndCountsCycles) {
  CpuRig rig;
  startProgram(rig, {0xA9, 0x55, 0x58});  // LDA #$55; CLI
  rig.step();
  rig.step();
  ASSERT_EQ(rig.cpu.registers().status, STATUS_U);

  rig.cpu.reset(false);

  const auto regs = rig.cpu.registers();
  EXPECT_EQ(regs.pc, 0x0200);
  EXPECT_EQ(regs.a, 0x55);
  EXPECT_EQ(regs.sp, 0xFA);
  EXPECT_EQ(regs.status, STATUS_U | STATUS_I);
  EXPECT_EQ(regs.cycles, 7U + 2 + 2 + 7);
}

TEST(Cpu6502Reset, RejectsNonPositiveClockDivision) {
  hw::ClockDomain bad{0};
  ember::logic::hardware::MasterScheduler scheduler;
  ember::logic::hardware::CpuAddressBus bus;
  ember::logic::hardware::WiredOrIrqLine irq;
  ember::logic::hardware::WiredOrIrqLine nmi;
  ember::logic::hardware::WiredOrRdyLine rdy;
  try {
    ember::logic::hardware::Cpu6502 cpu(bus, scheduler, bad, irq, nmi, rdy);
    FAIL() << "分频比为 0 应当报错";
  } catch (const CoreError& error) {
    EXPECT_EQ(error.code(), hw::ERROR_INVALID_ARGUMENT);
  }
}

TEST(Cpu6502Timing, BusAccessHappensInTheMiddleOfEachCycle) {
  CpuRig rig;
  startProgram(rig, {0xA9, 0x01});  // LDA #$01，从主时钟 84 开始

  EXPECT_EQ(rig.step(), 2U);

  ASSERT_EQ(rig.memory.log.size(), 2U);
  EXPECT_EQ(rig.memory.log[0].time, 84 + 6);
  EXPECT_EQ(rig.memory.log[1].time, 96 + 6);
  EXPECT_EQ(rig.scheduler.now(), 108);
}

TEST(Cpu6502Timing, WorksWithOneTickPerCycle) {
  CpuRig rig(1);
  startProgram(rig, {0xA9, 0x01, 0xEA});

  rig.step();
  rig.step();

  EXPECT_EQ(rig.scheduler.now(), 7 + 2 + 2);
  EXPECT_EQ(rig.cpu.registers().cycles, 11U);
}

TEST(Cpu6502Run, RunsWholeInstructionsAtLeastTheRequestedCycles) {
  CpuRig rig;
  fillNops(rig, 0x0200, 16);
  rig.startAt(0x0200);

  EXPECT_EQ(rig.cpu.run(0), 0U);
  EXPECT_EQ(rig.cpu.run(5), 6U);
  EXPECT_EQ(rig.cpu.registers().pc, 0x0203);
  EXPECT_EQ(rig.cpu.run(6), 6U);
}

// ---------------------------------------------------------------- 周期数

struct CycleCase {
  const char* name;
  std::vector<std::uint8_t> instruction;
  std::uint8_t x;
  std::uint8_t y;
  std::uint32_t cycles;
};

TEST(Cpu6502Cycles, EveryAddressingModeTakesTheDocumentedCycles) {
  // $10 处放指针 $1234；每个用例先用 LDX/LDY 设好变址寄存器，再测最后一条指令。
  const std::vector<CycleCase> cases{
      {"LDA #imm", {0xA9, 0x01}, 0, 0, 2},
      {"LDA zp", {0xA5, 0x10}, 0, 0, 3},
      {"LDA zp,X", {0xB5, 0x10}, 1, 0, 4},
      {"LDX zp,Y", {0xB6, 0x10}, 0, 1, 4},
      {"LDA abs", {0xAD, 0x34, 0x12}, 0, 0, 4},
      {"LDA abs,X 不跨页", {0xBD, 0x34, 0x12}, 1, 0, 4},
      {"LDA abs,X 跨页", {0xBD, 0x34, 0x12}, 0xFF, 0, 5},
      {"LDA abs,Y 不跨页", {0xB9, 0x34, 0x12}, 0, 1, 4},
      {"LDA abs,Y 跨页", {0xB9, 0x34, 0x12}, 0, 0xFF, 5},
      {"LDA (zp,X)", {0xA1, 0x10}, 0, 0, 6},
      {"LDA (zp),Y 不跨页", {0xB1, 0x10}, 0, 1, 5},
      {"LDA (zp),Y 跨页", {0xB1, 0x10}, 0, 0xFF, 6},
      {"STA zp", {0x85, 0x10}, 0, 0, 3},
      {"STA zp,X", {0x95, 0x10}, 1, 0, 4},
      {"STA abs", {0x8D, 0x34, 0x12}, 0, 0, 4},
      {"STA abs,X 不跨页", {0x9D, 0x34, 0x12}, 1, 0, 5},
      {"STA abs,Y 跨页", {0x99, 0x34, 0x12}, 0, 0xFF, 5},
      {"STA (zp,X)", {0x81, 0x10}, 0, 0, 6},
      {"STA (zp),Y", {0x91, 0x10}, 0, 1, 6},
      {"INC zp", {0xE6, 0x10}, 0, 0, 5},
      {"DEC zp", {0xC6, 0x10}, 0, 0, 5},
      {"INC zp,X", {0xF6, 0x10}, 1, 0, 6},
      {"INC abs", {0xEE, 0x34, 0x12}, 0, 0, 6},
      {"INC abs,X", {0xFE, 0x34, 0x12}, 1, 0, 7},
      {"ASL A", {0x0A}, 0, 0, 2},
      {"TAX", {0xAA}, 0, 0, 2},
      {"INX", {0xE8}, 0, 0, 2},
      {"NOP", {0xEA}, 0, 0, 2},
      {"NOP #imm（非官方）", {0x80, 0x00}, 0, 0, 2},
      {"NOP zp（非官方）", {0x04, 0x10}, 0, 0, 3},
      {"NOP zp,X（非官方）", {0x14, 0x10}, 1, 0, 4},
      {"NOP abs（非官方）", {0x0C, 0x34, 0x12}, 0, 0, 4},
      {"NOP abs,X 不跨页（非官方）", {0x1C, 0x34, 0x12}, 1, 0, 4},
      {"NOP abs,X 跨页（非官方）", {0x1C, 0x34, 0x12}, 0xFF, 0, 5},
      {"LAX zp", {0xA7, 0x10}, 0, 0, 3},
      {"LAX (zp),Y 跨页", {0xB3, 0x10}, 0, 0xFF, 6},
      {"SAX zp", {0x87, 0x10}, 0, 0, 3},
      {"SAX (zp,X)", {0x83, 0x10}, 0, 0, 6},
      {"SLO (zp,X)", {0x03, 0x10}, 0, 0, 8},
      {"RLA zp", {0x27, 0x10}, 0, 0, 5},
      {"SRE zp,X", {0x57, 0x10}, 1, 0, 6},
      {"RRA abs,Y", {0x7B, 0x34, 0x12}, 0, 1, 7},
      {"DCP (zp),Y", {0xD3, 0x10}, 0, 1, 8},
      {"ISB abs,X", {0xFF, 0x34, 0x12}, 1, 0, 7},
      {"PHA", {0x48}, 0, 0, 3},
      {"PHP", {0x08}, 0, 0, 3},
      {"PLA", {0x68}, 0, 0, 4},
      {"PLP", {0x28}, 0, 0, 4},
      {"JMP abs", {0x4C, 0x34, 0x12}, 0, 0, 3},
      {"JMP (ind)", {0x6C, 0x34, 0x12}, 0, 0, 5},
      {"JSR", {0x20, 0x34, 0x12}, 0, 0, 6},
  };

  for (const CycleCase& item : cases) {
    CpuRig rig;
    rig.memory.load(0x0200, {0xA2, item.x, 0xA0, item.y});  // LDX #x; LDY #y
    for (std::size_t i = 0; i < item.instruction.size(); ++i) rig.memory.poke(static_cast<std::uint16_t>(0x0204 + i), item.instruction[i]);
    rig.memory.load(0x0010, {0x34, 0x12});
    rig.startAt(0x0200);
    rig.step();
    rig.step();

    EXPECT_EQ(rig.step(), item.cycles) << item.name;
  }
}

TEST(Cpu6502Cycles, BranchesTakeTwoThreeOrFourCycles) {
  struct BranchCase {
    const char* name;
    std::uint16_t address;
    std::uint8_t opcode;
    std::uint8_t offset;
    std::uint32_t cycles;
    std::uint16_t nextPc;
  };
  // 上电后 Z=0。
  const std::vector<BranchCase> cases{
      {"BEQ 不成立", 0x2000, 0xF0, 0x02, 2, 0x2002},
      {"BNE 成立不跨页", 0x2000, 0xD0, 0x02, 3, 0x2004},
      {"BNE 向前跨页", 0x20FC, 0xD0, 0x10, 4, 0x210E},
      {"BNE 向后跨页", 0x2100, 0xD0, 0xFC, 4, 0x20FE},
      {"BNE 向后不跨页", 0x2150, 0xD0, 0xFC, 3, 0x214E},
  };
  for (const BranchCase& item : cases) {
    CpuRig rig;
    rig.memory.load(item.address, {item.opcode, item.offset});
    rig.startAt(item.address);

    EXPECT_EQ(rig.step(), item.cycles) << item.name;
    EXPECT_EQ(rig.cpu.registers().pc, item.nextPc) << item.name;
  }
}

TEST(Cpu6502Cycles, CrossingBranchReadsFromTheUncorrectedAddress) {
  CpuRig rig;
  rig.memory.load(0x20FC, {0xD0, 0x10});
  rig.startAt(0x20FC);

  rig.step();

  // 取指、取偏移、空读 PC、再在“低字节已加、高字节未修正”的地址上空读。
  const std::vector<Seen> expected{{false, 0x20FC, 0xD0}, {false, 0x20FD, 0x10}, {false, 0x20FE, 0}, {false, 0x200E, 0}};
  EXPECT_EQ(seenOf(rig), expected);
}

// ---------------------------------------------------------------- 总线访问顺序

TEST(Cpu6502Bus, ReadModifyWriteWritesTheOldValueBackBeforeTheNewOne) {
  CpuRig rig;
  rig.memory.poke(0x0010, 0x7F);
  startProgram(rig, {0xE6, 0x10});  // INC $10

  rig.step();

  const std::vector<Seen> expected{{false, 0x0200, 0xE6}, {false, 0x0201, 0x10}, {false, 0x0010, 0x7F}, {true, 0x0010, 0x7F}, {true, 0x0010, 0x80}};
  EXPECT_EQ(seenOf(rig), expected);
  EXPECT_EQ(rig.cpu.registers().status, STATUS_U | STATUS_I | STATUS_N);
}

TEST(Cpu6502Bus, CrossingIndexedReadDoesADummyReadAtTheUncorrectedAddress) {
  CpuRig rig;
  startProgram(rig, {0xA2, 0xFF, 0xBD, 0x34, 0x12});  // LDX #$FF; LDA $1234,X
  rig.memory.poke(0x1333, 0x99);
  rig.step();
  rig.memory.log.clear();

  rig.step();

  const std::vector<Seen> expected{{false, 0x0202, 0xBD}, {false, 0x0203, 0x34}, {false, 0x0204, 0x12}, {false, 0x1233, 0}, {false, 0x1333, 0x99}};
  EXPECT_EQ(seenOf(rig), expected);
  EXPECT_EQ(rig.cpu.registers().a, 0x99);
}

TEST(Cpu6502Bus, IndexedStoreAlwaysDoesTheDummyRead) {
  CpuRig rig;
  startProgram(rig, {0xA2, 0x01, 0xA9, 0x5A, 0x9D, 0x34, 0x12});  // LDX #1; LDA #$5A; STA $1234,X
  rig.step();
  rig.step();
  rig.memory.log.clear();

  rig.step();

  const std::vector<Seen> expected{{false, 0x0204, 0x9D}, {false, 0x0205, 0x34}, {false, 0x0206, 0x12}, {false, 0x1235, 0}, {true, 0x1235, 0x5A}};
  EXPECT_EQ(seenOf(rig), expected);
}

TEST(Cpu6502Bus, ZeroPageIndexedWrapsInsideTheZeroPageAfterADummyRead) {
  CpuRig rig;
  startProgram(rig, {0xA2, 0x20, 0xB5, 0xF0});  // LDX #$20; LDA $F0,X
  rig.memory.poke(0x0010, 0x77);
  rig.step();
  rig.memory.log.clear();

  rig.step();

  const std::vector<Seen> expected{{false, 0x0202, 0xB5}, {false, 0x0203, 0xF0}, {false, 0x00F0, 0}, {false, 0x0010, 0x77}};
  EXPECT_EQ(seenOf(rig), expected);
  EXPECT_EQ(rig.cpu.registers().a, 0x77);
}

TEST(Cpu6502Bus, IndirectJumpDoesNotCarryAcrossThePointerPage) {
  CpuRig rig;
  startProgram(rig, {0x6C, 0xFF, 0x03});  // JMP ($03FF)
  rig.memory.poke(0x03FF, 0x34);
  rig.memory.poke(0x0300, 0x12);  // 真机取到的高字节
  rig.memory.poke(0x0400, 0x56);  // 不会取这里

  rig.step();

  EXPECT_EQ(rig.cpu.registers().pc, 0x1234);
}

// ---------------------------------------------------------------- 运算与标志

struct ProgramCase {
  const char* name;
  std::vector<std::uint8_t> program;
  std::uint8_t zeroPage10;
  int steps;
  std::uint8_t a;
  std::uint8_t x;
  std::uint8_t status;
};

TEST(Cpu6502Alu, ProgramsProduceTheExpectedRegistersAndFlags) {
  constexpr std::uint8_t base = STATUS_U | STATUS_I;  // 上电后的状态
  const std::vector<ProgramCase> cases{
      {"ADC 有符号溢出", {0xA9, 0x50, 0x69, 0x50}, 0, 2, 0xA0, 0, base | STATUS_V | STATUS_N},
      {"SBC 借位", {0x38, 0xA9, 0x00, 0xE9, 0x01}, 0, 3, 0xFF, 0, base | STATUS_N},
      {"ASL A 移出进位与为零", {0xA9, 0x80, 0x0A}, 0, 2, 0x00, 0, base | STATUS_Z | STATUS_C},
      {"BIT 取内存的 N 与 V", {0xA9, 0xFF, 0x85, 0x10, 0xA9, 0x0F, 0x24, 0x10}, 0, 4, 0x0F, 0, base | STATUS_V | STATUS_N},
      {"PHP 压入的副本带 B", {0x08, 0x68}, 0, 2, base | STATUS_B, 0, base},
      {"PLP 忽略 B 与第 5 位", {0xA9, 0xFF, 0x48, 0x28}, 0, 3, 0xFF, 0, 0xEF},
      {"CMP 相等", {0xA9, 0x10, 0xC9, 0x10}, 0, 2, 0x10, 0, base | STATUS_Z | STATUS_C},
      {"CMP 小于", {0xA9, 0x10, 0xC9, 0x20}, 0, 2, 0x10, 0, base | STATUS_N},
      {"ROR 带入进位", {0x38, 0xA9, 0x01, 0x6A}, 0, 3, 0x80, 0, base | STATUS_N | STATUS_C},
      {"SED 后 ADC 仍是二进制", {0xF8, 0xA9, 0x09, 0x69, 0x01}, 0, 3, 0x0A, 0, base | STATUS_D},
      {"LAX", {0xA7, 0x10}, 0x80, 1, 0x80, 0x80, base | STATUS_N},
      {"SLO", {0xA9, 0x04, 0x07, 0x10}, 0x81, 2, 0x06, 0, base | STATUS_C},
      {"RLA", {0x38, 0xA9, 0xFF, 0x27, 0x10}, 0x40, 3, 0x81, 0, base | STATUS_N},
      {"SRE", {0xA9, 0xFF, 0x47, 0x10}, 0x03, 2, 0xFE, 0, base | STATUS_N | STATUS_C},
      {"RRA", {0xA9, 0x10, 0x67, 0x10}, 0x03, 2, 0x12, 0, base},
      {"DCP", {0xA9, 0x05, 0xC7, 0x10}, 0x06, 2, 0x05, 0, base | STATUS_Z | STATUS_C},
      {"ISB", {0x38, 0xA9, 0x10, 0xE7, 0x10}, 0x0F, 3, 0x00, 0, base | STATUS_Z | STATUS_C},
  };

  for (const ProgramCase& item : cases) {
    CpuRig rig;
    for (std::size_t i = 0; i < item.program.size(); ++i) rig.memory.poke(static_cast<std::uint16_t>(0x0200 + i), item.program[i]);
    rig.memory.poke(0x0010, item.zeroPage10);
    rig.startAt(0x0200);
    for (int i = 0; i < item.steps; ++i) rig.step();

    const auto regs = rig.cpu.registers();
    EXPECT_EQ(regs.a, item.a) << item.name;
    EXPECT_EQ(regs.x, item.x) << item.name;
    EXPECT_EQ(regs.status, item.status | STATUS_U) << item.name;
  }
}

TEST(Cpu6502Alu, SaxStoresAAndXAndTxsDoesNotChangeFlags) {
  CpuRig rig;
  startProgram(rig, {0xA9, 0xF0, 0xA2, 0x3C, 0x87, 0x10, 0xA2, 0x80, 0x9A});  // LDA #$F0; LDX #$3C; SAX $10; LDX #$80; TXS
  for (int i = 0; i < 4; ++i) rig.step();
  EXPECT_EQ(rig.memory.peek(0x0010), 0x30);

  const std::uint8_t before = rig.cpu.registers().status;
  rig.step();

  EXPECT_EQ(rig.cpu.registers().sp, 0x80);
  EXPECT_EQ(rig.cpu.registers().status, before);
}

// ---------------------------------------------------------------- 栈与中断

TEST(Cpu6502Stack, JsrPushesTheLastOperandAddressAndRtsReturnsAfterIt) {
  CpuRig rig;
  startProgram(rig, {0x20, 0x00, 0x03});  // JSR $0300
  rig.memory.poke(0x0300, 0x60);          // RTS

  EXPECT_EQ(rig.step(), 6U);
  EXPECT_EQ(rig.cpu.registers().pc, 0x0300);
  EXPECT_EQ(rig.cpu.registers().sp, 0xFB);
  EXPECT_EQ(rig.memory.peek(0x01FD), 0x02);
  EXPECT_EQ(rig.memory.peek(0x01FC), 0x02);

  EXPECT_EQ(rig.step(), 6U);
  EXPECT_EQ(rig.cpu.registers().pc, 0x0203);
  EXPECT_EQ(rig.cpu.registers().sp, 0xFD);
}

TEST(Cpu6502Stack, RtiRestoresStatusAndPcWithoutOffsetting) {
  CpuRig rig;
  startProgram(rig, {0xA9, 0x03, 0x48, 0xA9, 0x10, 0x48, 0xA9, 0xA5, 0x48, 0x40});  // 压入 PCH=$03、PCL=$10、P=$A5，RTI
  for (int i = 0; i < 6; ++i) rig.step();

  EXPECT_EQ(rig.step(), 6U);

  const auto regs = rig.cpu.registers();
  EXPECT_EQ(regs.pc, 0x0310);
  EXPECT_EQ(regs.status, 0xA5);
  EXPECT_EQ(regs.sp, 0xFD);
}

TEST(Cpu6502Interrupts, BrkPushesPcPlusTwoAndStatusWithBreakAndUsesIrqVector) {
  CpuRig rig;
  startProgram(rig, {0x00, 0x00});
  rig.setVectors(0x0400, 0x0500);

  EXPECT_EQ(rig.step(), 7U);

  const auto regs = rig.cpu.registers();
  EXPECT_EQ(regs.pc, 0x0400);
  EXPECT_EQ(regs.sp, 0xFA);
  EXPECT_EQ(regs.status & STATUS_I, STATUS_I);
  EXPECT_EQ(rig.memory.peek(0x01FD), 0x02);
  EXPECT_EQ(rig.memory.peek(0x01FC), 0x02);
  EXPECT_EQ(rig.memory.peek(0x01FB), STATUS_U | STATUS_I | STATUS_B);
}

TEST(Cpu6502Interrupts, IrqIsMaskedByTheInterruptFlag) {
  CpuRig rig;
  fillNops(rig, 0x0200, 8);
  rig.startAt(0x0200);
  rig.setVectors(0x0400, 0x0500);
  rig.irq.set(rig.irqSource, true);

  EXPECT_EQ(rig.step(), 2U);
  EXPECT_EQ(rig.cpu.registers().pc, 0x0201);
}

TEST(Cpu6502Interrupts, IrqIsTakenAfterTheInstructionFollowingCli) {
  CpuRig rig;
  startProgram(rig, {0x58, NOP, NOP});  // CLI; NOP; NOP
  fillNops(rig, 0x0400, 4);
  rig.setVectors(0x0400, 0x0500);
  rig.irq.set(rig.irqSource, true);

  // CLI 本身之后不响应。
  EXPECT_EQ(rig.step(), 2U);
  EXPECT_EQ(rig.cpu.registers().pc, 0x0201);

  // 紧随其后的一条指令执行完才响应：2 周期的 NOP 加 7 周期的中断序列。
  EXPECT_EQ(rig.step(), 9U);
  const auto regs = rig.cpu.registers();
  EXPECT_EQ(regs.pc, 0x0400);
  EXPECT_EQ(regs.status & STATUS_I, STATUS_I);
  EXPECT_EQ(rig.memory.peek(0x01FD), 0x02);
  EXPECT_EQ(rig.memory.peek(0x01FC), 0x02);
  EXPECT_EQ(rig.memory.peek(0x01FB), STATUS_U);  // 硬件中断压入的状态没有 B，响应时 I 已清除

  // 处理程序里 I 已置位，电平仍有效也不会再次响应。
  EXPECT_EQ(rig.step(), 2U);
  EXPECT_EQ(rig.cpu.registers().pc, 0x0401);
}

TEST(Cpu6502Interrupts, NmiIsEdgeTriggeredAndTakesSevenCycles) {
  CpuRig rig;
  fillNops(rig, 0x0200, 8);
  fillNops(rig, 0x0500, 8);
  rig.startAt(0x0200);
  rig.setVectors(0x0400, 0x0500);
  rig.nmi.set(rig.nmiSource, true);

  EXPECT_EQ(rig.step(), 9U);
  auto regs = rig.cpu.registers();
  EXPECT_EQ(regs.pc, 0x0500);
  EXPECT_EQ(regs.sp, 0xFA);
  EXPECT_EQ(rig.memory.peek(0x01FD), 0x02);
  EXPECT_EQ(rig.memory.peek(0x01FC), 0x01);
  EXPECT_EQ(rig.memory.peek(0x01FB), STATUS_U | STATUS_I);  // NMI 不受 I 屏蔽，压入的状态没有 B

  // 电平一直有效不会再次触发。
  EXPECT_EQ(rig.step(), 2U);
  EXPECT_EQ(rig.cpu.registers().pc, 0x0501);

  // 释放后再次有效，产生新的边沿；CPU 每个周期结束才采样，所以中间要运行一条指令。
  rig.nmi.set(rig.nmiSource, false);
  EXPECT_EQ(rig.step(), 2U);
  rig.nmi.set(rig.nmiSource, true);
  EXPECT_EQ(rig.step(), 9U);
  EXPECT_EQ(rig.cpu.registers().pc, 0x0500);
  EXPECT_EQ(rig.memory.peek(0x01FA), 0x05);  // 返回地址 $0503
  EXPECT_EQ(rig.memory.peek(0x01F9), 0x03);
}

TEST(Cpu6502Interrupts, NmiArrivingInTheLastCycleWaitsForTheNextInstruction) {
  CpuRig rig;
  fillNops(rig, 0x0200, 8);
  fillNops(rig, 0x0500, 8);
  rig.startAt(0x0200);
  rig.setVectors(0x0400, 0x0500);
  LineEvent assertNmi(rig.nmi, rig.nmiSource);
  // 复位后在主时钟 84 开始第一条 NOP：第一个周期 84-96，最后一个周期 96-108。
  rig.scheduler.scheduleAt(100, assertNmi, 1);

  EXPECT_EQ(rig.step(), 2U);
  EXPECT_EQ(rig.cpu.registers().pc, 0x0201);

  EXPECT_EQ(rig.step(), 9U);
  EXPECT_EQ(rig.cpu.registers().pc, 0x0500);
  EXPECT_EQ(rig.memory.peek(0x01FC), 0x02);  // 返回地址是第二条 NOP 之后
}

TEST(Cpu6502Interrupts, NmiArrivingBeforeTheLastCycleIsTakenRightAfterTheInstruction) {
  CpuRig rig;
  fillNops(rig, 0x0200, 8);
  fillNops(rig, 0x0500, 8);
  rig.startAt(0x0200);
  rig.setVectors(0x0400, 0x0500);
  LineEvent assertNmi(rig.nmi, rig.nmiSource);
  rig.scheduler.scheduleAt(90, assertNmi, 1);

  EXPECT_EQ(rig.step(), 9U);
  EXPECT_EQ(rig.cpu.registers().pc, 0x0500);
  EXPECT_EQ(rig.memory.peek(0x01FC), 0x01);
}

TEST(Cpu6502Interrupts, NmiDuringBrkHijacksTheVectorButKeepsTheBreakFlag) {
  CpuRig rig;
  startProgram(rig, {0x00, 0x00});
  fillNops(rig, 0x0500, 8);
  rig.setVectors(0x0400, 0x0500);
  LineEvent assertNmi(rig.nmi, rig.nmiSource);
  // BRK 从主时钟 84 开始，第 3 个周期（压栈）是 108-120。
  rig.scheduler.scheduleAt(111, assertNmi, 1);

  EXPECT_EQ(rig.step(), 7U);

  EXPECT_EQ(rig.cpu.registers().pc, 0x0500);
  EXPECT_EQ(rig.memory.peek(0x01FB), STATUS_U | STATUS_I | STATUS_B);
  // NMI 已经用掉，不会在处理程序开头再次响应。
  EXPECT_EQ(rig.step(), 2U);
  EXPECT_EQ(rig.cpu.registers().pc, 0x0501);
}

// ---------------------------------------------------------------- RDY

TEST(Cpu6502Rdy, HaltsOnReadCyclesWithoutTouchingTheBusAndResumesWhenReleased) {
  CpuRig rig;
  startProgram(rig, {0xA9, 0x01});  // LDA #$01
  LineEvent releaseRdy(rig.rdy, rig.rdySource);
  rig.rdy.set(rig.rdySource, true);
  rig.scheduler.scheduleAt(84 + 5 * 12, releaseRdy, 0);  // 5 个周期后释放

  EXPECT_EQ(rig.step(), 2U + 5U);

  ASSERT_EQ(rig.memory.log.size(), 2U);
  EXPECT_EQ(rig.memory.log[0].address, 0x0200);
  EXPECT_EQ(rig.memory.log[0].time, 84 + 5 * 12 + 6);  // 暂停期间没有访问总线
  EXPECT_EQ(rig.cpu.registers().cycles, 7U + 7U);
  EXPECT_EQ(rig.cpu.registers().a, 0x01);
}

TEST(Cpu6502Rdy, DoesNotHaltWriteCycles) {
  CpuRig rig;
  startProgram(rig, {0xA9, 0x00, 0x85, 0x10, NOP});  // LDA #0; STA $10; NOP
  rig.step();                                        // 此时主时钟为 84 + 24 = 108
  LineEvent assertRdy(rig.rdy, rig.rdySource);
  LineEvent releaseRdy(rig.rdy, rig.rdySource);
  // STA 的前两个周期是 108-132，写周期是 132-144；在写周期开始前置位 RDY。
  rig.scheduler.scheduleAt(132, assertRdy, 1);
  rig.scheduler.scheduleAt(192, releaseRdy, 0);
  rig.memory.log.clear();

  EXPECT_EQ(rig.step(), 3U);  // 写周期照常完成
  ASSERT_EQ(rig.memory.log.size(), 3U);
  EXPECT_TRUE(rig.memory.log[2].write);
  EXPECT_EQ(rig.memory.log[2].time, 138);

  // 下一个读周期（NOP 的取指）才被暂停，直到 192。
  EXPECT_EQ(rig.step(), 2U + 4U);
  EXPECT_EQ(rig.memory.log[3].time, 192 + 6);
}

TEST(Cpu6502Rdy, ThrowsWhenNobodyReleasesTheLine) {
  CpuRig rig;
  startProgram(rig, {NOP});
  rig.rdy.set(rig.rdySource, true);

  try {
    rig.step();
    FAIL() << "RDY 一直有效应当报错";
  } catch (const CoreError& error) {
    EXPECT_EQ(error.code(), hw::ERROR_CPU_STALLED);
  }
}

}  // namespace
