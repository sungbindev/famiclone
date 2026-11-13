/*
 * ------------------------------------------------------------------------
 * 名称：fc-machine-test.cpp
 * 说明：fc 主机型号的装配测试：CPU 总线与 PPU 总线的译码、卡槽转交、名称表镜像、精灵 DMA、手柄、NMI、运行一帧、电源与复位。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：用手写的小程序放进测试用 NROM 卡带运行，再从内部 RAM、PPU 总线与 PPU 内部状态读取结果。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <cstdint>

#include "contracts/hardware/button-input.hpp"
#include "contracts/hardware/hardware-error.hpp"
#include "fc-test-support.hpp"
#include "shared/errors/core-error.hpp"

namespace {

namespace hw = ember::contracts::hardware;
using ember::test::Assembler;
using ember::test::FcRig;
using ember::test::makeNromFile;

/** 运行主程序一段时间（程序以空转结束）。 */
void runCycles(FcRig& rig, int cycles) { rig.machine.run(static_cast<hw::MasterTime>(cycles) * 12); }

std::uint8_t ram(FcRig& rig, std::size_t offset) { return rig.machine.internalRam().read(offset); }

TEST(FcMachine, PowerOnRunsTheResetSequenceAndLoadsTheResetVector) {
  Assembler main(0x8000);
  main.jmpSelf();
  FcRig rig(makeNromFile(main));
  EXPECT_TRUE(rig.machine.isPoweredOn());
  const hw::CpuRegisters registers = rig.machine.cpuRegisters();
  EXPECT_EQ(registers.pc, 0x8000);
  EXPECT_EQ(registers.cycles, 7U);
  EXPECT_EQ(rig.machine.clock().now(), 7 * 12);
}

TEST(FcMachine, InternalRamIsMirroredEveryTwoKilobytes) {
  Assembler main(0x8000);
  main.ldaImm(0xA5).staAbs(0x0001);
  main.ldaAbs(0x0801).staAbs(0x0010);  // 读镜像
  main.ldaImm(0x3C).staAbs(0x1FFE);    // 写镜像
  main.ldaAbs(0x07FE).staAbs(0x0011);
  main.jmpSelf();
  FcRig rig(makeNromFile(main));
  runCycles(rig, 200);
  EXPECT_EQ(ram(rig, 0x10), 0xA5);
  EXPECT_EQ(ram(rig, 0x11), 0x3C);
  EXPECT_EQ(ram(rig, 0x07FE), 0x3C);
}

TEST(FcMachine, ApuPlaceholderAndUnusedAddressesReadTheOpenBus) {
  Assembler main(0x8000);
  main.ldaAbs(0x4015).staAbs(0x0010);  // APU 的空设备
  main.ldaAbs(0x4018).staAbs(0x0011);  // 没有设备的地址
  main.ldaAbs(0x4014).staAbs(0x0012);  // DMA 寄存器不可读
  main.ldaImm(0x55).staAbs(0x4000);    // 写 APU 的空设备被忽略
  main.ldaAbs(0x5000).staAbs(0x0013);  // 卡槽的 $4020-$5FFF 卡带不驱动数据线
  main.jmpSelf();
  FcRig rig(makeNromFile(main));
  runCycles(rig, 200);
  // 读数据时总线上是指令的最后一个操作数（地址高字节）。
  EXPECT_EQ(ram(rig, 0x10), 0x40);
  EXPECT_EQ(ram(rig, 0x11), 0x40);
  EXPECT_EQ(ram(rig, 0x12), 0x40);
  EXPECT_EQ(ram(rig, 0x13), 0x50);
}

TEST(FcMachine, PpuRegistersAreMirroredEveryEightBytesUpToTheEndOfTheRange) {
  Assembler main(0x8000);
  main.ldaImm(0x20).staAbs(0x2E06);  // $2E06 是 $2006 的镜像
  main.ldaImm(0x00).staAbs(0x2E06);
  main.ldaImm(0x77).staAbs(0x3FFF);  // $3FFF 是 $2007 的镜像
  main.jmpSelf();
  FcRig rig(makeNromFile(main));
  runCycles(rig, 200);
  EXPECT_EQ(rig.machine.peekPpuBus(0x2000), 0x77);
}

/** 向 $2000、$2400、$2800、$2C00 依次写 0x11、0x22、0x33、0x44，返回四个名称表地址读回的值。 */
std::array<std::uint8_t, 4> nametableValues(bool vertical) {
  Assembler main(0x8000);
  main.ppuWrite(0x2000, 0x11).ppuWrite(0x2400, 0x22).ppuWrite(0x2800, 0x33).ppuWrite(0x2C00, 0x44).jmpSelf();
  FcRig rig(makeNromFile(main, nullptr, vertical));
  runCycles(rig, 400);
  return {rig.machine.peekPpuBus(0x2000), rig.machine.peekPpuBus(0x2400), rig.machine.peekPpuBus(0x2800), rig.machine.peekPpuBus(0x2C00)};
}

TEST(FcMachine, NametableMirroringFollowsTheCartridge) {
  // 水平镜像：$2000 与 $2400 是同一页，$2800 与 $2C00 是同一页。
  EXPECT_EQ(nametableValues(false), (std::array<std::uint8_t, 4>{0x22, 0x22, 0x44, 0x44}));
  // 垂直镜像：$2000 与 $2800 是同一页，$2400 与 $2C00 是同一页。
  EXPECT_EQ(nametableValues(true), (std::array<std::uint8_t, 4>{0x33, 0x44, 0x33, 0x44}));
}

TEST(FcMachine, NametablesAreMirroredAt3000AndPatternTablesGoToTheCartridge) {
  Assembler main(0x8000);
  main.ppuWrite(0x2123, 0x5A).ppuWrite(0x0010, 0xC3).jmpSelf();
  FcRig rig(makeNromFile(main));
  runCycles(rig, 300);
  EXPECT_EQ(rig.machine.peekPpuBus(0x3123), 0x5A);
  // 测试卡带的 CHR 是 RAM，图案表的写入由卡带响应。
  EXPECT_EQ(rig.machine.peekPpuBus(0x0010), 0xC3);
}

TEST(FcMachine, OamDmaCopiesAPageIntoOamAndTheCpuResumesAfterwards) {
  Assembler main(0x8000);
  main.ldxImm(0x00);
  const std::uint16_t loop = main.here();
  main.txa().staAbsX(0x0200).inx().bne(loop);  // 页 2 的第 i 个字节为 i
  main.ldaImm(0x02).staAbs(0x4014);
  main.ldaImm(0x77).staAbs(0x0030);  // DMA 结束后继续执行
  main.jmpSelf();
  FcRig rig(makeNromFile(main));
  runCycles(rig, 5000);
  const auto oam = rig.machine.ppu().oam();
  for (std::size_t i = 0; i < 256; ++i) ASSERT_EQ(oam[i], static_cast<std::uint8_t>(i)) << "OAM 第 " << i << " 字节";
  EXPECT_EQ(ram(rig, 0x30), 0x77);
}

TEST(FcMachine, ControllerPortsAreReadThroughTheCpuBus) {
  Assembler main(0x8000);
  main.ldaImm(1).staAbs(0x4016).ldaImm(0).staAbs(0x4016);
  main.ldxImm(0);
  const std::uint16_t loop = main.here();
  main.ldaAbs(0x4016).andImm(0x01).staAbsX(0x0040).inx().cpxImm(8).bne(loop);
  main.jmpSelf();
  FcRig rig(makeNromFile(main));
  rig.machine.setButtons(0, hw::pad_button::A | hw::pad_button::START | hw::pad_button::LEFT);
  runCycles(rig, 500);
  const std::uint8_t expected[8] = {1, 0, 0, 1, 0, 0, 1, 0};
  for (std::size_t i = 0; i < 8; ++i) EXPECT_EQ(ram(rig, 0x40 + i), expected[i]) << "第 " << i << " 位";
}

TEST(FcMachine, SetButtonsRejectsUnknownPorts) {
  Assembler main(0x8000);
  main.jmpSelf();
  FcRig rig(makeNromFile(main));
  EXPECT_NO_THROW(rig.machine.setButtons(1, hw::pad_button::B));
  try {
    rig.machine.setButtons(2, 0);
    FAIL() << "应当抛出异常";
  } catch (const ember::shared::CoreError& error) {
    EXPECT_EQ(error.code(), hw::ERROR_PORT_NOT_FOUND);
  }
}

TEST(FcMachine, NmiReachesTheCpuOncePerFrame) {
  Assembler main(0x8000);
  main.ldaImm(0x80).staAbs(0x2000).jmpSelf();
  Assembler nmi(0x8100);
  nmi.incAbs(0x0030).rti();
  FcRig rig(makeNromFile(main, &nmi));
  for (int i = 0; i < 10; ++i) rig.machine.runFrame();
  // 第 k 次 runFrame 在第 k 帧的第 240 行结束，此时已经过了 k-1 次 vblank。
  EXPECT_EQ(ram(rig, 0x30), 9);
}

TEST(FcMachine, RunFrameCompletesExactlyOneFrameEachTime) {
  Assembler main(0x8000);
  main.jmpSelf();
  FcRig rig(makeNromFile(main));
  EXPECT_EQ(rig.machine.frame().width, 0U);
  rig.machine.runFrame();
  EXPECT_EQ(rig.machine.ppu().frameCount(), 1U);
  EXPECT_EQ(rig.machine.frame().width, 256U);
  EXPECT_EQ(rig.machine.frame().height, 240U);
  const hw::MasterTime first = rig.machine.clock().now();
  rig.machine.runFrame();
  EXPECT_EQ(rig.machine.ppu().frameCount(), 2U);
  // 一帧 89342 个点，每点 4 个主时钟周期；两次返回时各多走不到一条指令的时间，所以允许 8 个 CPU 周期的误差。
  const hw::MasterTime length = rig.machine.clock().now() - first;
  EXPECT_NEAR(static_cast<double>(length), 89342.0 * 4.0, 8.0 * 12.0);
}

TEST(FcMachine, RunAdvancesTheClockByAtLeastTheRequestedTicks) {
  Assembler main(0x8000);
  main.jmpSelf();
  FcRig rig(makeNromFile(main));
  const hw::MasterTime start = rig.machine.clock().now();
  rig.machine.run(1000);
  const hw::MasterTime elapsed = rig.machine.clock().now() - start;
  EXPECT_GE(elapsed, 1000);
  EXPECT_LT(elapsed, 1000 + 12 * 6);
}

TEST(FcMachine, PowerOffStopsRunningAndSoftResetKeepsRamAndCartridgeState) {
  Assembler main(0x8000);
  main.ldaImm(0x99).staAbs(0x0020).staAbs(0x6000).jmpSelf();
  FcRig rig(makeNromFile(main));
  runCycles(rig, 100);
  EXPECT_EQ(ram(rig, 0x20), 0x99);

  rig.machine.softReset();
  EXPECT_EQ(rig.machine.cpuRegisters().pc, 0x8000);
  EXPECT_EQ(ram(rig, 0x20), 0x99);
  EXPECT_EQ(rig.cartridge.prgRam()[0], 0x99);

  rig.machine.powerOff();
  EXPECT_FALSE(rig.machine.isPoweredOn());
  const hw::MasterTime stopped = rig.machine.clock().now();
  rig.machine.run(1000);
  rig.machine.runFrame();
  rig.machine.softReset();
  EXPECT_EQ(rig.machine.clock().now(), stopped);

  // 上电：RAM 与卡带的状态重新初始化。
  rig.machine.powerOn();
  EXPECT_TRUE(rig.machine.isPoweredOn());
  EXPECT_EQ(ram(rig, 0x20), 0x00);
  EXPECT_EQ(rig.cartridge.prgRam()[0], 0x00);
  EXPECT_EQ(rig.machine.cpuRegisters().cycles, 7U);
}

TEST(FcMachine, AnEmptySlotReadsTheOpenBusAndTheMachineStillRuns) {
  ember::bootstrap::FcMachine machine;
  EXPECT_FALSE(machine.cartridgeSlot().occupied());
  machine.powerOn();
  // 复位向量读到开放总线上的值（零）；之后 CPU 照常运行，整机不会卡住。
  EXPECT_EQ(machine.cpuRegisters().pc, 0x0000);
  machine.runFrame();
  EXPECT_EQ(machine.ppu().frameCount(), 1U);
}

}  // namespace
