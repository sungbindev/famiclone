/*
 * ------------------------------------------------------------------------
 * 名称：fdc-upd765-test.cpp
 * 说明：软驱驱动器与软驱控制器的测试：盘片格式推断、插入与取出、轮询中断、读扇区编号、回零与寻道、读写扇区（程序查询方式与 DMA 数据口方式）、写保护、各种注入的故障、空闲时不登记事件。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：时间用主时钟推进：控制器每 64 个主时钟周期执行一步；盘片镜像是 1.44MB，每个字节的内容由位置算出，便于核对扇区位置。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include "contracts/hardware/interrupt-sink.hpp"
#include "logic/hardware/fdc-upd765/fdc-upd765.hpp"
#include "logic/hardware/floppy-drive/standard-floppy-drive.hpp"
#include "logic/hardware/scheduler/master-scheduler.hpp"

namespace {

namespace hw = ember::contracts::hardware;
using ember::logic::hardware::FdcUpd765;
using ember::logic::hardware::MasterScheduler;
using ember::logic::hardware::StandardFloppyDrive;

constexpr hw::MasterTime STEP = 64;
constexpr std::uint8_t MSR_READY = 0x80;
constexpr std::uint8_t MSR_DIRECTION = 0x40;
/** DOR：驱动器 0、马达、允许中断与 DMA、不复位。 */
constexpr std::uint8_t DOR_RUN = 0x1C;

/** 每个字节的内容由它在镜像里的位置算出。 */
std::uint8_t patternAt(std::size_t offset) { return static_cast<std::uint8_t>(offset * 7 + 3); }

std::vector<std::uint8_t> makeImage(std::size_t size = 1440 * 1024) {
  std::vector<std::uint8_t> image(size);
  for (std::size_t index = 0; index < size; ++index) image[index] = patternAt(index);
  return image;
}

class CountingSink final : public hw::InterruptSink {
 public:
  void raise(std::uint8_t sourceMask) override {
    ++count;
    last = sourceMask;
  }
  int count{0};
  std::uint8_t last{0};
};

struct Bench {
  Bench() : fdc(scheduler, sink, 0x08, STEP, 91) {
    fdc.attachDrive(0, &drive);
    drive.insertImage(makeImage(), false, "disk.img");
    fdc.reset(true);
  }

  void run(unsigned steps) { scheduler.advance(static_cast<hw::MasterTime>(steps) * STEP); }

  /** 推进到主状态满足条件，最多推进 limit 步；没有满足时返回 false。 */
  bool waitMsr(std::uint8_t mask, std::uint8_t value, unsigned limit = 5000) {
    for (unsigned step = 0; step < limit; ++step) {
      if ((fdc.mainStatus() & mask) == value) return true;
      run(1);
    }
    return (fdc.mainStatus() & mask) == value;
  }

  /** 写一个命令或数据字节（等控制器就绪、方向是写）。 */
  void put(std::uint8_t byte) {
    ASSERT_TRUE(waitMsr(MSR_READY | MSR_DIRECTION, MSR_READY));
    fdc.writeRegister(5, byte);
  }

  /** 读一个结果或数据字节（等控制器就绪、方向是读）。 */
  std::uint8_t get() {
    EXPECT_TRUE(waitMsr(MSR_READY | MSR_DIRECTION, MSR_READY | MSR_DIRECTION));
    return fdc.readRegister(5);
  }

  void command(std::initializer_list<std::uint8_t> bytes) {
    for (const std::uint8_t byte : bytes) put(byte);
  }

  std::vector<std::uint8_t> results(std::size_t count) {
    std::vector<std::uint8_t> values;
    for (std::size_t index = 0; index < count; ++index) values.push_back(get());
    return values;
  }

  /** 启动马达并等轮询完成，取走轮询产生的中断状态。 */
  void spinUp() {
    fdc.writeRegister(2, DOR_RUN);
    run(1000);
    command({0x08});
    results(2);
  }

  /** 设定命令：usePio 为 true 时用程序查询方式（ND=1），否则用 DMA 数据口方式。 */
  void specify(bool usePio) { command({0x03, 0xDF, static_cast<std::uint8_t>(usePio ? 0x03 : 0x02)}); }

  /** 读扇区命令（单扇区，EOT=R）。 */
  void readCommand(std::uint8_t cylinder, std::uint8_t head, std::uint8_t sector) { command({0x46, static_cast<std::uint8_t>(head << 2), cylinder, head, sector, 2, sector, 0x1B, 0xFF}); }

  MasterScheduler scheduler;
  CountingSink sink;
  StandardFloppyDrive drive;
  FdcUpd765 fdc;
};

TEST(StandardFloppyDrive, InfersTheFormatFromTheImageSize) {
  EXPECT_EQ(StandardFloppyDrive::formatFor(1440 * 1024).sectors, 18U);
  EXPECT_EQ(StandardFloppyDrive::formatFor(1440 * 1024).dataRate, 500U);
  EXPECT_EQ(StandardFloppyDrive::formatFor(720 * 1024).sectors, 9U);
  EXPECT_EQ(StandardFloppyDrive::formatFor(720 * 1024).dataRate, 250U);
  EXPECT_EQ(StandardFloppyDrive::formatFor(360 * 1024).cylinders, 40U);
  EXPECT_EQ(StandardFloppyDrive::formatFor(2880 * 1024).sectors, 36U);
  EXPECT_EQ(StandardFloppyDrive::formatFor(12345).sectors, 18U) << "非标准大小按 1.44MB 处理";
}

TEST(StandardFloppyDrive, ReportsOnlyRealMotorChanges) {
  StandardFloppyDrive drive;
  std::vector<bool> changes;
  drive.onMotorChanged = [&changes](bool on) { changes.push_back(on); };
  drive.setMotor(false);
  drive.setMotor(true);
  drive.setMotor(true);
  drive.setMotor(false);
  EXPECT_EQ(changes, (std::vector<bool>{true, false}));
  StandardFloppyDrive absent(false);
  changes.clear();
  absent.onMotorChanged = [&changes](bool on) { changes.push_back(on); };
  absent.setMotor(true);
  EXPECT_EQ(changes, (std::vector<bool>{false})) << "没装的驱动器马达不算在转";
}

TEST(StandardFloppyDrive, InsertEjectAndTheChangedFlag) {
  StandardFloppyDrive drive;
  EXPECT_FALSE(drive.inserted());
  drive.insertImage(makeImage(), true, "a.img");
  EXPECT_TRUE(drive.inserted());
  EXPECT_TRUE(drive.writeProtected());
  EXPECT_EQ(drive.source(), "a.img");
  EXPECT_FALSE(drive.diskChanged());
  drive.writeByte(10, 0x55);
  EXPECT_EQ(drive.readByte(10), 0x55);
  EXPECT_TRUE(drive.modified());
  drive.markSaved();
  EXPECT_FALSE(drive.modified());
  drive.ejectImage();
  EXPECT_FALSE(drive.inserted());
  EXPECT_TRUE(drive.diskChanged());
  EXPECT_EQ(drive.readByte(10), 0);
  EXPECT_TRUE(drive.image().empty());
}

TEST(StandardFloppyDrive, TheDirtyRangeCoversEveryWrittenByteAndIsClearedWhenSaved) {
  StandardFloppyDrive drive;
  drive.insertImage(makeImage(), false, "a.img");
  EXPECT_TRUE(drive.dirtyRange().empty());
  drive.writeByte(100, 0x55);
  drive.writeByte(20, 0x66);
  drive.writeByte(300, 0x77);
  EXPECT_EQ(drive.dirtyRange().begin, 20U);
  EXPECT_EQ(drive.dirtyRange().end, 301U);
  EXPECT_TRUE(drive.modified());
  drive.markSaved();
  EXPECT_TRUE(drive.dirtyRange().empty());
  EXPECT_FALSE(drive.modified());
  drive.writeByte(5, 0x11);
  drive.ejectImage();
  EXPECT_TRUE(drive.dirtyRange().empty()) << "取出盘片后区间清空";
}

TEST(StandardFloppyDrive, TheModifiedListenerIsCalledOnlyWhenTheRangeGoesFromEmptyToNonEmpty) {
  StandardFloppyDrive drive;
  int calls = 0;
  drive.setModifiedListener([&calls] { ++calls; });
  drive.insertImage(makeImage(), false, "a.img");
  drive.writeByte(10, 1);
  drive.writeByte(11, 2);
  EXPECT_EQ(calls, 1) << "区间已经非空时继续写不再通知";
  drive.markSaved();
  drive.writeByte(12, 3);
  EXPECT_EQ(calls, 2);
  drive.markSaved();
  drive.ejectImage();
  drive.writeByte(13, 4);
  EXPECT_EQ(calls, 2) << "没有盘片时写入被丢弃，不通知";
}

TEST(StandardFloppyDrive, ExternalChangesGoIntoTheImageWithoutMarkingItModified) {
  StandardFloppyDrive drive;
  int calls = 0;
  drive.setModifiedListener([&calls] { ++calls; });
  drive.insertImage(makeImage(), false, "a.img");
  const std::vector<std::uint8_t> bytes{1, 2, 3};
  drive.applyExternalChange(10, bytes);
  EXPECT_EQ(drive.readByte(10), 1);
  EXPECT_EQ(drive.readByte(12), 3);
  EXPECT_FALSE(drive.modified());
  EXPECT_EQ(calls, 0);
  drive.applyExternalChange(drive.image().size() - 1, bytes);
  EXPECT_NE(drive.readByte(drive.image().size() - 1), 1) << "超出镜像的丢弃";
}

TEST(StandardFloppyDrive, SuspendedMediaCanBeRestoredWithItsContents) {
  StandardFloppyDrive drive;
  drive.insertImage(makeImage(), false, "a.img");
  EXPECT_TRUE(drive.suspendMedia());
  EXPECT_FALSE(drive.inserted());
  EXPECT_TRUE(drive.diskChanged());
  EXPECT_TRUE(drive.restoreMedia());
  EXPECT_TRUE(drive.inserted());
  EXPECT_EQ(drive.readByte(100), patternAt(100));
  EXPECT_FALSE(drive.restoreMedia());
}

TEST(FdcUpd765, StartsInTheCommandPhaseAndIsIdle) {
  Bench bench;
  EXPECT_EQ(bench.fdc.mainStatus(), MSR_READY);
  EXPECT_EQ(bench.fdc.dor(), 0);
  EXPECT_FALSE(bench.fdc.stepScheduled());
  EXPECT_FALSE(bench.fdc.irqRaised());
}

TEST(FdcUpd765, ReleasingResetStartsPollingAndRaisesAnInterruptForTheDrive) {
  Bench bench;
  bench.fdc.writeRegister(2, DOR_RUN);
  EXPECT_TRUE(bench.fdc.stepScheduled()) << "离开复位后要轮询驱动器";
  bench.run(1000);
  EXPECT_TRUE(bench.fdc.irqRaised());
  EXPECT_EQ(bench.sink.count, 1);
  EXPECT_EQ(bench.sink.last, 0x08);
  EXPECT_EQ(bench.fdc.readRegister(3), 0x40);
  bench.command({0x08});
  const auto status = bench.results(2);
  EXPECT_EQ(status[0], 0xC0) << "轮询到驱动器 0 就绪：ST0 = $C0 | 驱动器号";
  EXPECT_EQ(status[1], 0);
  bench.run(5);
  EXPECT_FALSE(bench.fdc.irqRaised());
  EXPECT_FALSE(bench.fdc.stepScheduled()) << "轮询完成、命令阶段在等命令字节时不再登记事件";
}

TEST(FdcUpd765, NoInterruptIsRaisedWhileDorBitThreeIsClear) {
  Bench bench;
  bench.fdc.writeRegister(2, 0x14);
  bench.run(1000);
  EXPECT_TRUE(bench.fdc.irqRaised());
  EXPECT_EQ(bench.sink.count, 0) << "DOR 位 3 为 0 时不请求中断";
  bench.fdc.writeRegister(2, 0x1C);
  EXPECT_EQ(bench.sink.count, 1) << "允许中断的那一刻出现上升沿";
}

TEST(FdcUpd765, SpecifyAndInvalidCommands) {
  Bench bench;
  bench.spinUp();
  bench.specify(true);
  bench.run(5);
  EXPECT_EQ(bench.fdc.mainStatus() & 0xD0, MSR_READY) << "设定命令没有结果阶段";
  bench.command({0x1F});
  EXPECT_EQ(bench.get(), 0x80) << "无效命令的结果是 ST0 = $80";
}

TEST(FdcUpd765, RecalibrateSeekAndSenseInterruptReportTheCylinder) {
  Bench bench;
  bench.spinUp();
  bench.command({0x0F, 0x00, 40});
  bench.run(10);
  bench.command({0x08});
  auto status = bench.results(2);
  EXPECT_EQ(status[0], 0x20) << "寻道结束";
  EXPECT_EQ(status[1], 40);
  EXPECT_EQ(bench.drive.cylinder(), 40U);
  bench.command({0x07, 0x00});
  bench.run(10);
  bench.command({0x08});
  status = bench.results(2);
  EXPECT_EQ(status[1], 0);
  EXPECT_EQ(bench.drive.cylinder(), 0U);
}

TEST(FdcUpd765, SenseDriveStatusReportsTrackZeroAndMedia) {
  Bench bench;
  bench.spinUp();
  bench.command({0x04, 0x00});
  const std::uint8_t status = bench.get();
  EXPECT_EQ(status & 0x10, 0x10) << "在 0 磁道";
  EXPECT_EQ(status & 0x20, 0x20) << "有盘片";
  EXPECT_EQ(status & 0x40, 0x00) << "没有写保护";
}

TEST(FdcUpd765, ReadIdReportsThePositionAndSize) {
  Bench bench;
  bench.spinUp();
  bench.command({0x4A, 0x00});
  const auto result = bench.results(7);
  EXPECT_EQ(result[0], 0x00);
  EXPECT_EQ(result[1], 0x00);
  EXPECT_EQ(result[3], 0) << "磁道";
  EXPECT_EQ(result[5], 1) << "扇区";
  EXPECT_EQ(result[6], 2) << "大小码：512 字节";
}

TEST(FdcUpd765, ReadsASectorByProgrammedIoAndEndsAtTheEndOfTrack) {
  Bench bench;
  bench.spinUp();
  bench.specify(true);
  bench.command({0x0F, 0x00, 1});
  bench.run(10);
  bench.command({0x08});
  bench.results(2);
  bench.readCommand(1, 1, 3);
  const std::size_t base = ((1 * 2 + 1) * 18 + 3 - 1) * 512U;
  for (std::size_t index = 0; index < 512; ++index) ASSERT_EQ(bench.get(), patternAt(base + index)) << "第 " << index << " 个字节";
  const auto result = bench.results(7);
  EXPECT_EQ(result[0] & 0xC0, 0x40) << "读到本道最后一个扇区：异常结束";
  EXPECT_EQ(result[1] & 0x80, 0x80) << "位 7：到达磁道末尾";
  EXPECT_EQ(result[3], 1);
  EXPECT_EQ(result[4], 1);
}

TEST(FdcUpd765, ReadsASectorThroughTheDmaDataPortAndEndsWithTerminalCount) {
  Bench bench;
  bench.spinUp();
  bench.specify(false);
  bench.readCommand(0, 0, 2);
  for (std::size_t index = 0; index < 512; ++index) {
    for (unsigned guard = 0; !bench.fdc.dmaRequest() && guard < 1000; ++guard) bench.run(1);
    ASSERT_TRUE(bench.fdc.dmaRequest()) << "第 " << index << " 个字节";
    if (index == 511) bench.fdc.setTerminalCount();
    ASSERT_EQ(bench.fdc.receiveFromDma(), patternAt(512 + index));
  }
  const auto result = bench.results(7);
  EXPECT_EQ(result[0] & 0xC0, 0x00) << "终止计数：正常结束";
}

TEST(FdcUpd765, WritesASectorIntoTheImageAndMarksItModified) {
  Bench bench;
  bench.spinUp();
  bench.specify(true);
  bench.command({0x45, 0x00, 0, 0, 5, 2, 5, 0x1B, 0xFF});
  for (std::size_t index = 0; index < 512; ++index) bench.put(static_cast<std::uint8_t>(index ^ 0xA5));
  const auto result = bench.results(7);
  EXPECT_EQ(result[0] & 0xC0, 0x40);
  EXPECT_TRUE(bench.drive.modified());
  for (std::size_t index = 0; index < 512; ++index) ASSERT_EQ(bench.drive.readByte(4 * 512 + index), static_cast<std::uint8_t>(index ^ 0xA5));
  EXPECT_EQ(bench.drive.readByte(5 * 512), patternAt(5 * 512)) << "下一个扇区不受影响";
}

TEST(FdcUpd765, WriteProtectedDisksRejectWrites) {
  Bench bench;
  bench.drive.setWriteProtected(true);
  bench.spinUp();
  bench.specify(true);
  bench.command({0x45, 0x00, 0, 0, 1, 2, 1, 0x1B, 0xFF});
  const auto result = bench.results(7);
  EXPECT_EQ(result[0] & 0x40, 0x40);
  EXPECT_EQ(result[1] & 0x02, 0x02) << "ST1 位 1：不可写";
  EXPECT_FALSE(bench.drive.modified());
}

TEST(FdcUpd765, ReadingWithoutADiskFails) {
  Bench bench;
  bench.drive.ejectImage();
  bench.spinUp();
  bench.specify(true);
  bench.readCommand(0, 0, 1);
  const auto result = bench.results(7);
  EXPECT_EQ(result[0] & 0x40, 0x40);
  EXPECT_EQ(result[1] & 0x01, 0x01) << "找不到地址标记";
}

TEST(FdcUpd765, ReadingWithTheWrongDataRateFails) {
  Bench bench;
  bench.spinUp();
  bench.specify(true);
  bench.fdc.writeRegister(7, 2);  // 250 千位每秒，1.44MB 的盘需要 500
  bench.readCommand(0, 0, 1);
  const auto result = bench.results(7);
  EXPECT_EQ(result[1] & 0x01, 0x01);
}

TEST(FdcUpd765, AbsentDrivesReportNotReady) {
  Bench bench;
  bench.spinUp();
  bench.fdc.writeRegister(2, DOR_RUN | 0x01);  // 选 1 号驱动器，没有接
  bench.command({0x4A, 0x01});
  const auto result = bench.results(7);
  EXPECT_EQ(result[0] & 0x40, 0x40);
}

TEST(FdcUpd765, DiskChangedBitIsSetByEjectAndClearedByReading) {
  Bench bench;
  bench.spinUp();
  EXPECT_EQ(bench.fdc.readRegister(7), 0x00);
  bench.drive.ejectImage();
  EXPECT_EQ(bench.fdc.readRegister(7), 0x80);
  EXPECT_EQ(bench.fdc.readRegister(7), 0x00) << "读后清除";
}

TEST(FdcUpd765, NoIndexPulseStallsReadId) {
  Bench bench;
  bench.spinUp();
  bench.drive.mutableFaults().noIndex = true;
  bench.command({0x4A, 0x00});
  bench.run(500);
  EXPECT_EQ(bench.fdc.mainStatus() & MSR_DIRECTION, 0) << "没有结果";
  EXPECT_TRUE(bench.fdc.stepScheduled()) << "命令一直在执行，状态机还在转";
  bench.drive.mutableFaults().noIndex = false;
  bench.run(10);
  EXPECT_EQ(bench.results(7)[0] & 0xC0, 0);
}

TEST(FdcUpd765, StuckHeadKeepsTheCylinderAndFailsRecalibrate) {
  Bench bench;
  bench.spinUp();
  bench.command({0x0F, 0x00, 30});
  bench.run(10);
  bench.command({0x08});
  bench.results(2);
  bench.drive.mutableFaults().seekFail = true;
  bench.command({0x0F, 0x00, 5});
  bench.run(10);
  EXPECT_EQ(bench.drive.cylinder(), 30U) << "磁头卡住，寻道不移动";
  bench.command({0x08});
  EXPECT_EQ(bench.results(2)[1], 30);
  bench.command({0x07, 0x00});
  bench.run(10);
  bench.command({0x08});
  EXPECT_EQ(bench.results(2)[0] & 0x50, 0x50) << "回零失败：ST0 位 4 设备检查、位 6 异常";
}

TEST(FdcUpd765, BadSectorsFailWithADataError) {
  Bench bench;
  bench.drive.mutableFaults().badSectors.push_back(0);
  bench.spinUp();
  bench.specify(true);
  bench.readCommand(0, 0, 1);
  const auto result = bench.results(7);
  EXPECT_EQ(result[0] & 0x40, 0x40);
  EXPECT_EQ(result[1] & 0x20, 0x20);
  EXPECT_EQ(result[2] & 0x20, 0x20);
}

TEST(FdcUpd765, AnUnresponsiveControllerNeverBecomesReadyAgain) {
  Bench bench;
  bench.spinUp();
  bench.drive.mutableFaults().unresponsive = true;
  bench.fdc.writeRegister(5, 0x08);
  bench.run(1000);
  EXPECT_EQ(bench.fdc.mainStatus() & MSR_READY, 0) << "命令字节没有被处理";
  EXPECT_FALSE(bench.fdc.stepScheduled());
}

TEST(FdcUpd765, ResettingThroughTheDorOrThePinReturnsToTheCommandPhase) {
  Bench bench;
  bench.spinUp();
  bench.command({0x4A});
  bench.fdc.writeRegister(2, DOR_RUN & ~0x04);
  EXPECT_EQ(bench.fdc.mainStatus(), MSR_READY);
  bench.fdc.writeRegister(2, DOR_RUN);
  bench.run(1000);
  bench.command({0x08});
  bench.results(2);
  bench.command({0x4A});
  bench.fdc.setResetPin(true);
  bench.fdc.setResetPin(false);
  EXPECT_EQ(bench.fdc.mainStatus(), MSR_READY);
}

TEST(FdcUpd765, ThePowerResetStopsTheMotorAndKeepsTheDisk) {
  Bench bench;
  bench.spinUp();
  EXPECT_TRUE(bench.drive.motorRunning());
  bench.fdc.reset(true);
  EXPECT_FALSE(bench.drive.motorRunning());
  EXPECT_TRUE(bench.drive.inserted());
  EXPECT_EQ(bench.fdc.dor(), 0);
}

}  // namespace
