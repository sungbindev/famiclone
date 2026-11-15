/*
 * ------------------------------------------------------------------------
 * 名称：um6576-peripherals-test.cpp
 * 说明：6576 芯片组的外设类模块测试：DMA、键盘接口与小霸王键盘、鼠标接口与小霸王鼠标、手柄口与并口线、并口数据寄存器。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：键盘与鼠标的字节时间按“1 毫秒 = 1000 个主时钟周期”的测试时钟换算；设备经调度器推进时间。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <cstdint>
#include <map>
#include <optional>
#include <utility>
#include <vector>

#include "contracts/hardware/controller-port.hpp"
#include "contracts/hardware/dma-memory.hpp"
#include "contracts/hardware/interrupt-sink.hpp"
#include "contracts/hardware/parallel-port.hpp"
#include "contracts/hardware/serial-device.hpp"
#include "logic/hardware/bus/mapped-bus.hpp"
#include "logic/hardware/dma-um6576/dma-um6576.hpp"
#include "logic/hardware/keyboard-sb2000/keyboard-sb2000.hpp"
#include "logic/hardware/keyboard-um6576/keyboard-um6576.hpp"
#include "logic/hardware/lines/wired-or-line.hpp"
#include "logic/hardware/mouse-sb2000/mouse-sb2000.hpp"
#include "logic/hardware/mouse-um6576/mouse-um6576.hpp"
#include "logic/hardware/pad-um6576/pad-um6576.hpp"
#include "logic/hardware/printer-um6576/printer-um6576.hpp"
#include "logic/hardware/scheduler/master-scheduler.hpp"
#include "shared/errors/core-error.hpp"
namespace {

namespace hw = ember::contracts::hardware;
using ember::logic::hardware::CpuAddressBus;
using ember::logic::hardware::DmaUm6576;
using ember::logic::hardware::KeyboardSb2000;
using ember::logic::hardware::KeyboardUm6576;
using ember::logic::hardware::MasterScheduler;
using ember::logic::hardware::MouseSb2000;
using ember::logic::hardware::MouseUm6576;
using ember::logic::hardware::PadUm6576;
using ember::logic::hardware::PrinterUm6576;
using ember::logic::hardware::WiredOrRdyLine;

/** 1 毫秒的主时钟周期数。 */
constexpr hw::MasterTime MS = 1000;

/** 记录请求的中断汇集器。 */
class RecordingSink final : public hw::InterruptSink {
 public:
  void raise(std::uint8_t sourceMask) override { raised.push_back(sourceMask); }
  std::vector<std::uint8_t> raised;
};

/** 记录字节与到达时间的接收方。 */
class RecordingReceiver final : public hw::ByteReceiver {
 public:
  explicit RecordingReceiver(const hw::Clock& clock) : clock_(clock) {}
  void receiveByte(std::uint8_t value) override { bytes.emplace_back(clock_.now(), value); }
  std::vector<std::pair<hw::MasterTime, std::uint8_t>> bytes;

 private:
  const hw::Clock& clock_;
};

/** 记录主机发来的字节与控制脉冲的串行设备。 */
class FakeSerialDevice final : public hw::SerialDevice {
 public:
  void connect(hw::ByteReceiver* r) override { receiver = r; }
  void sendByte(std::uint8_t value) override { sent.push_back(value); }
  void pulseControl() override { ++pulses; }
  void reset(bool /*power*/) override {}
  hw::ByteReceiver* receiver{nullptr};
  std::vector<std::uint8_t> sent;
  int pulses{0};
};

TEST(KeyboardSb2000, MapsPhysicalKeysToScanCodes) {
  EXPECT_EQ(KeyboardSb2000::scanCodeFor("KeyA"), std::optional<std::uint8_t>(0x1E));
  EXPECT_EQ(KeyboardSb2000::scanCodeFor("Enter"), std::optional<std::uint8_t>(0x1C));
  EXPECT_EQ(KeyboardSb2000::scanCodeFor("ArrowUp"), std::optional<std::uint8_t>(0xC8));
  EXPECT_EQ(KeyboardSb2000::scanCodeFor("F12"), std::optional<std::uint8_t>(0x58));
  EXPECT_FALSE(KeyboardSb2000::scanCodeFor("LaunchApp1").has_value());
}

TEST(KeyboardSb2000, ResetCommandAnswersAckThenSelfTest) {
  MasterScheduler scheduler;
  KeyboardSb2000 keyboard(scheduler, MS);
  RecordingReceiver receiver(scheduler);
  keyboard.connect(&receiver);
  keyboard.sendByte(0xFF);
  scheduler.advance(300 * MS);
  ASSERT_EQ(receiver.bytes.size(), 2U);
  EXPECT_EQ(receiver.bytes[0], std::make_pair(2 * MS, std::uint8_t{0xFA}));
  EXPECT_EQ(receiver.bytes[1], std::make_pair(102 * MS, std::uint8_t{0xAA}));
}

TEST(KeyboardSb2000, ReadIdAnswersAckAndTwoBytes) {
  MasterScheduler scheduler;
  KeyboardSb2000 keyboard(scheduler, MS);
  RecordingReceiver receiver(scheduler);
  keyboard.connect(&receiver);
  keyboard.sendByte(0xF2);
  scheduler.advance(20 * MS);
  ASSERT_EQ(receiver.bytes.size(), 3U);
  EXPECT_EQ(receiver.bytes[0].second, 0xFA);
  EXPECT_EQ(receiver.bytes[1].second, 0x12);
  EXPECT_EQ(receiver.bytes[2].second, 0x34);
  EXPECT_EQ(receiver.bytes[2].first, 4 * MS);
}

TEST(KeyboardSb2000, ScanCodeSetOptionStartsTheKeyboardAndKeysAreReported) {
  MasterScheduler scheduler;
  KeyboardSb2000 keyboard(scheduler, MS);
  RecordingReceiver receiver(scheduler);
  keyboard.connect(&receiver);
  keyboard.setKey(0x1C, true);
  scheduler.advance(100 * MS);
  EXPECT_TRUE(receiver.bytes.empty()) << "没有开始工作时不报告按键";
  keyboard.sendByte(0xF0);
  keyboard.sendByte(0x01);
  scheduler.advance(10 * MS);
  EXPECT_TRUE(keyboard.working());
  ASSERT_EQ(receiver.bytes.size(), 2U) << "两次应答";
  scheduler.advance(100 * MS);
  ASSERT_EQ(receiver.bytes.size(), 3U);
  EXPECT_EQ(receiver.bytes[2].second, 0x1C) << "按下报告扫描码";
  keyboard.setKey(0x1C, false);
  scheduler.advance(100 * MS);
  ASSERT_EQ(receiver.bytes.size(), 4U);
  EXPECT_EQ(receiver.bytes[3].second, 0x9C) << "松开报告扫描码加 $80";
}

TEST(KeyboardSb2000, ExtendedKeysSendThePrefixFirst) {
  MasterScheduler scheduler;
  KeyboardSb2000 keyboard(scheduler, MS);
  RecordingReceiver receiver(scheduler);
  keyboard.connect(&receiver);
  keyboard.sendByte(0xF0);
  keyboard.sendByte(0x01);
  scheduler.advance(10 * MS);
  receiver.bytes.clear();
  keyboard.setKey(0xC8, true);
  scheduler.advance(50 * MS);
  ASSERT_EQ(receiver.bytes.size(), 2U);
  EXPECT_EQ(receiver.bytes[0].second, 0xE0);
  EXPECT_EQ(receiver.bytes[1].second, 0x48);
  keyboard.setKey(0xC8, false);
  scheduler.advance(50 * MS);
  ASSERT_EQ(receiver.bytes.size(), 4U);
  EXPECT_EQ(receiver.bytes[2].second, 0xE0);
  EXPECT_EQ(receiver.bytes[3].second, 0xC8);
}

TEST(KeyboardSb2000, OnlyOneKeyChangeIsReportedPerCheck) {
  MasterScheduler scheduler;
  KeyboardSb2000 keyboard(scheduler, MS);
  RecordingReceiver receiver(scheduler);
  keyboard.connect(&receiver);
  keyboard.sendByte(0xF0);
  keyboard.sendByte(0x01);
  scheduler.advance(10 * MS);
  receiver.bytes.clear();
  keyboard.setKey(0x10, true);
  keyboard.setKey(0x11, true);
  scheduler.advance(21 * MS);
  ASSERT_EQ(receiver.bytes.size(), 1U);
  EXPECT_EQ(receiver.bytes[0].second, 0x10);
  scheduler.advance(20 * MS);
  ASSERT_EQ(receiver.bytes.size(), 2U);
  EXPECT_EQ(receiver.bytes[1].second, 0x11);
}

TEST(KeyboardUm6576, KeyboardByteIsLatchedAndRaisesTheInterrupt) {
  MasterScheduler scheduler;
  RecordingSink sink;
  FakeSerialDevice device;
  KeyboardUm6576 keyboardPort(device, sink, scheduler, MS);
  keyboardPort.reset(true);
  ASSERT_EQ(device.receiver, &keyboardPort);
  device.receiver->receiveByte(0x1E);
  EXPECT_EQ(keyboardPort.read(0x4020).value, 0x1E);
  EXPECT_EQ(sink.raised, std::vector<std::uint8_t>({0x20}));
}

TEST(KeyboardUm6576, ControlPortReadsBitSixHighAndTheWrittenBits) {
  MasterScheduler scheduler;
  RecordingSink sink;
  FakeSerialDevice device;
  KeyboardUm6576 keyboardPort(device, sink, scheduler, MS);
  keyboardPort.reset(true);
  EXPECT_EQ(keyboardPort.read(0x4022).value, 0x40) << "键盘数据线空闲为高";
  keyboardPort.write(0x4022, 0x80);
  EXPECT_EQ(keyboardPort.read(0x4022).value, 0xC0);
}

TEST(KeyboardUm6576, RisingClockWithSystemToKeyboardDirectionSendsTheByte) {
  MasterScheduler scheduler;
  RecordingSink sink;
  FakeSerialDevice device;
  KeyboardUm6576 keyboardPort(device, sink, scheduler, MS);
  keyboardPort.reset(true);
  keyboardPort.write(0x4021, 0xED);
  keyboardPort.write(0x4022, 0x01);  // 方向为 1，时钟为 0
  EXPECT_TRUE(device.sent.empty()) << "时钟没有上升沿";
  keyboardPort.write(0x4022, 0x81);  // 时钟上升
  ASSERT_EQ(device.sent.size(), 1U);
  EXPECT_EQ(device.sent[0], 0xED);
  keyboardPort.write(0x4022, 0x01);  // 时钟下降
  keyboardPort.write(0x4022, 0x80);  // 上升但方向为键盘到系统
  EXPECT_EQ(device.sent.size(), 1U);
}

TEST(KeyboardUm6576, SendCompleteInterruptComesAboutOneMillisecondLater) {
  MasterScheduler scheduler;
  RecordingSink sink;
  FakeSerialDevice device;
  KeyboardUm6576 keyboardPort(device, sink, scheduler, MS);
  keyboardPort.reset(true);
  keyboardPort.write(0x4021, 0xF3);
  keyboardPort.write(0x4022, 0x05);
  keyboardPort.write(0x4022, 0x85);  // 方向 1、位 2、时钟上升
  EXPECT_TRUE(sink.raised.empty());
  scheduler.advance(MS - 1);
  EXPECT_TRUE(sink.raised.empty());
  scheduler.advance(1);
  EXPECT_EQ(sink.raised, std::vector<std::uint8_t>({0x20}));
}

TEST(KeyboardUm6576, TimingControlByteIsKeptForTheDma) {
  MasterScheduler scheduler;
  RecordingSink sink;
  FakeSerialDevice device;
  KeyboardUm6576 keyboardPort(device, sink, scheduler, MS);
  keyboardPort.write(0x4020, 0x80);
  EXPECT_EQ(keyboardPort.timingControl(), 0x80);
  EXPECT_EQ(keyboardPort.read(0x4020).value, 0xFF) << "读 $4020 是键盘数据，不是写入的值";
}

TEST(MouseSb2000, RtsStartsThePnpIdentification) {
  MasterScheduler scheduler;
  MouseSb2000 mouse(scheduler, MS);
  RecordingReceiver receiver(scheduler);
  mouse.connect(&receiver);
  mouse.pulseControl();
  scheduler.advance(14 * MS);
  EXPECT_TRUE(receiver.bytes.empty());
  scheduler.advance(MS);
  ASSERT_EQ(receiver.bytes.size(), 1U);
  EXPECT_EQ(receiver.bytes[0].second, 0x4D);
  scheduler.advance(5 * MS);
  ASSERT_EQ(receiver.bytes.size(), 2U);
  EXPECT_EQ(receiver.bytes[1].second, 0x33);
  scheduler.advance(200 * MS);
  EXPECT_EQ(receiver.bytes.size(), 30U);
  EXPECT_EQ(receiver.bytes.back().second, 0x09);
  EXPECT_TRUE(mouse.working());
}

TEST(MouseSb2000, MovementIsReportedAsAThreeBytePacket) {
  MasterScheduler scheduler;
  MouseSb2000 mouse(scheduler, MS);
  RecordingReceiver receiver(scheduler);
  mouse.connect(&receiver);
  mouse.move(10, 10, 0);
  scheduler.advance(50 * MS);
  EXPECT_TRUE(receiver.bytes.empty()) << "没有收到 RTS 时不报告";
  mouse.pulseControl();
  scheduler.advance(300 * MS);
  receiver.bytes.clear();
  mouse.move(5, -3, MouseSb2000::BUTTON_LEFT);
  scheduler.advance(20 * MS);
  ASSERT_EQ(receiver.bytes.size(), 3U);
  EXPECT_EQ(receiver.bytes[0].second, 0x6C);
  EXPECT_EQ(receiver.bytes[1].second, 5);
  EXPECT_EQ(receiver.bytes[2].second, 0x3D);
}

TEST(MouseSb2000, MovementDuringATransferIsKeptForTheNextPacket) {
  MasterScheduler scheduler;
  MouseSb2000 mouse(scheduler, MS);
  RecordingReceiver receiver(scheduler);
  mouse.connect(&receiver);
  mouse.pulseControl();
  scheduler.advance(300 * MS);
  receiver.bytes.clear();
  mouse.move(1, 0, 0);
  mouse.move(2, 0, 0);
  scheduler.advance(30 * MS);
  ASSERT_EQ(receiver.bytes.size(), 6U);
  EXPECT_EQ(receiver.bytes[1].second, 1);
  EXPECT_EQ(receiver.bytes[4].second, 2);
}

TEST(MouseSb2000, ButtonChangeAloneIsReported) {
  MasterScheduler scheduler;
  MouseSb2000 mouse(scheduler, MS);
  RecordingReceiver receiver(scheduler);
  mouse.connect(&receiver);
  mouse.pulseControl();
  scheduler.advance(300 * MS);
  receiver.bytes.clear();
  mouse.move(0, 0, MouseSb2000::BUTTON_RIGHT);
  scheduler.advance(20 * MS);
  ASSERT_EQ(receiver.bytes.size(), 3U);
  EXPECT_EQ(receiver.bytes[0].second, 0x50);
}

TEST(MouseUm6576, DataIsLatchedInvertedOnlyWhenTheMouseIsSelected) {
  RecordingSink sink;
  FakeSerialDevice device;
  MouseUm6576 port(device, sink);
  port.reset(true);
  EXPECT_EQ(port.read(0x4023).value, 0x80);
  device.receiver->receiveByte(0x55);
  EXPECT_TRUE(sink.raised.empty()) << "没有选择鼠标，字节丢失";
  port.write(0x4023, 0x01);
  EXPECT_EQ(port.read(0x4023).value, 0x81);
  device.receiver->receiveByte(0x55);
  EXPECT_EQ(port.read(0x4024).value, 0xAA);
  EXPECT_EQ(sink.raised, std::vector<std::uint8_t>({0x40}));
}

/** 可以读出固定数据位的手柄。 */
class FakePad final : public hw::ControllerDevice {
 public:
  void setLatch(bool high) override { latch = high; }
  std::uint8_t clockData() override { return bit; }
  bool latch{false};
  std::uint8_t bit{0};
};

/** 记录输出线、可设置状态线的并口。 */
class FakeParallelPort final : public hw::ParallelPort {
 public:
  void writeData(std::uint8_t value) override { data = value; }
  void setStrobeN(bool) override {}
  void setSelectPrinterN(bool high) override { selectN = high; }
  void setLineFeedN(bool high) override { lineFeedN = high; }
  void setReset(bool high) override { resetLine = high; }
  [[nodiscard]] bool ack() const override { return true; }
  [[nodiscard]] bool paperOut() const override { return false; }
  [[nodiscard]] bool select() const override { return true; }
  [[nodiscard]] bool error() const override { return errorLevel; }
  [[nodiscard]] bool busyN() const override { return busyNLevel; }
  void reset() override {}
  std::uint8_t data{0};
  bool selectN{true};
  bool lineFeedN{true};
  bool resetLine{false};
  bool errorLevel{false};
  bool busyNLevel{true};
};

TEST(PadUm6576, ReadsTheDataBitsAndDrivesOnlyTheirLines) {
  FakeParallelPort parallel;
  PadUm6576 pads(parallel);
  FakePad pad1;
  FakePad pad2;
  pads.port(0).plug(&pad1);
  pads.port(1).plug(&pad2);
  pad1.bit = 1;
  const hw::BusRead first = pads.read(0x4016);
  EXPECT_EQ(first.value, 0x05) << "数据位 0 加真机实测总是 1 的位 2";
  EXPECT_EQ(first.drivenMask, 0x05);
  pad2.bit = 1;
  const hw::BusRead second = pads.read(0x4017);
  EXPECT_EQ(second.value, 0x09) << "数据位 0 加 #Busy（位 3）";
  EXPECT_EQ(second.drivenMask, 0x0D);
  parallel.errorLevel = true;
  EXPECT_EQ(pads.read(0x4017).value, 0x0D);
}

TEST(PadUm6576, WritingTheFirstPortLatchesBothPadsAndDrivesTheParallelLines) {
  FakeParallelPort parallel;
  PadUm6576 pads(parallel);
  FakePad pad1;
  FakePad pad2;
  pads.port(0).plug(&pad1);
  pads.port(1).plug(&pad2);
  pads.write(0x4016, 0x07);
  EXPECT_TRUE(pad1.latch);
  EXPECT_TRUE(pad2.latch);
  EXPECT_TRUE(parallel.selectN);
  EXPECT_TRUE(parallel.lineFeedN);
  EXPECT_TRUE(parallel.resetLine);
  pads.write(0x4016, 0x00);
  EXPECT_FALSE(pad1.latch);
  EXPECT_FALSE(parallel.selectN);
  EXPECT_FALSE(parallel.lineFeedN);
  EXPECT_FALSE(parallel.resetLine);
}

TEST(PadUm6576, OnlyTwoPortsExist) {
  FakeParallelPort parallel;
  PadUm6576 pads(parallel);
  EXPECT_THROW(pads.port(2), ember::shared::CoreError);
}

TEST(PrinterUm6576, WritesGoToTheDataLines) {
  FakeParallelPort parallel;
  PrinterUm6576 printer(parallel);
  printer.write(0x4025, 0x1B);
  EXPECT_EQ(parallel.data, 0x1B);
  EXPECT_EQ(printer.data(), 0x1B);
  EXPECT_EQ(printer.read(0x4025).drivenMask, 0);
}

/** 记录写入的 DMA 存储：源是递增的字节。 */
class FakeDmaMemory final : public hw::DmaMemory {
 public:
  std::uint8_t readSource(bool extendedRam, std::uint32_t offset) override {
    sourceReads.emplace_back(extendedRam, offset);
    return static_cast<std::uint8_t>(offset & 0xFF);
  }
  void writeVideo(std::uint16_t address, std::uint8_t value) override { video.emplace_back(address, value); }
  void writeWork(std::uint16_t address, std::uint8_t value) override { work.emplace_back(address, value); }
  std::vector<std::pair<bool, std::uint32_t>> sourceReads;
  std::vector<std::pair<std::uint16_t, std::uint8_t>> video;
  std::vector<std::pair<std::uint16_t, std::uint8_t>> work;
};

/** DMA 测试台：只接 CPU 总线、调度器与 RDY 线，通用 DMA 不用总线。 */
struct DmaBench {
  DmaBench() : dma(bus, scheduler, rdy, hw::ClockDomain{16}, memory) {}
  MasterScheduler scheduler;
  WiredOrRdyLine rdy;
  CpuAddressBus bus;
  FakeDmaMemory memory;
  DmaUm6576 dma;
};

/** 设置通用 DMA 的源、目的与长度（长度寄存器为字节数减 1）。 */
void setUpDma(DmaUm6576& dma, std::uint8_t mode, std::uint8_t sourceBank, std::uint16_t source, std::uint16_t destination, std::uint16_t lengthMinusOne) {
  dma.write(0x4048, mode & 0x7F);
  dma.write(0x4049, sourceBank);
  dma.write(0x404A, static_cast<std::uint8_t>(source & 0xFF));
  dma.write(0x404B, static_cast<std::uint8_t>(source >> 8));
  dma.write(0x404C, static_cast<std::uint8_t>(destination & 0xFF));
  dma.write(0x404D, static_cast<std::uint8_t>(destination >> 8));
  dma.write(0x404E, static_cast<std::uint8_t>(lengthMinusOne & 0xFF));
  dma.write(0x404F, static_cast<std::uint8_t>(lengthMinusOne >> 8));
}

TEST(DmaUm6576, GeneralDmaToVideoCopiesLengthPlusOneBytes) {
  DmaBench bench;
  setUpDma(bench.dma, 0x00, 0x00, 0x8100, 0x1000, 3);
  bench.dma.write(0x4048, 0x80);
  ASSERT_EQ(bench.memory.video.size(), 4U);
  EXPECT_EQ(bench.memory.video[0], std::make_pair(std::uint16_t{0x1000}, std::uint8_t{0x00}));
  EXPECT_EQ(bench.memory.video[3], std::make_pair(std::uint16_t{0x1003}, std::uint8_t{0x03}));
  EXPECT_EQ(bench.memory.sourceReads[0], std::make_pair(false, std::uint32_t{0x100}));
}

TEST(DmaUm6576, SourceAddressUsesTheBankAndSelectsExtendedRam) {
  DmaBench bench;
  setUpDma(bench.dma, 0x00, 0x13, 0xC234, 0x0000, 0);
  bench.dma.write(0x4048, 0x80);
  ASSERT_EQ(bench.memory.sourceReads.size(), 1U);
  EXPECT_TRUE(bench.memory.sourceReads[0].first);
  EXPECT_EQ(bench.memory.sourceReads[0].second, (3U << 15) | 0x4234U);
}

TEST(DmaUm6576, TransferNeedsSourceAddressBitFifteen) {
  DmaBench bench;
  setUpDma(bench.dma, 0x00, 0x00, 0x0100, 0x1000, 3);
  bench.dma.write(0x4048, 0x80);
  EXPECT_TRUE(bench.memory.video.empty());
}

TEST(DmaUm6576, VideoAddressWrapsAtSixteenBits) {
  DmaBench bench;
  setUpDma(bench.dma, 0x00, 0x00, 0x8000, 0xFFFF, 1);
  bench.dma.write(0x4048, 0x80);
  ASSERT_EQ(bench.memory.video.size(), 2U);
  EXPECT_EQ(bench.memory.video[0].first, 0xFFFF);
  EXPECT_EQ(bench.memory.video[1].first, 0x0000);
}

TEST(DmaUm6576, WorkRamDestinationNeedsTheTimingControlBit) {
  DmaBench bench;
  setUpDma(bench.dma, 0x20, 0x00, 0x8000, 0x6000, 1);
  bench.dma.write(0x4048, 0xA0);
  EXPECT_EQ(bench.memory.video.size(), 2U) << "$4020 位 7 为 0 时目的总是显存";
  EXPECT_TRUE(bench.memory.work.empty());
  bench.memory.video.clear();
  bench.dma.timingControlOn = [] { return true; };
  bench.dma.write(0x4048, 0xA0);
  EXPECT_TRUE(bench.memory.video.empty());
  ASSERT_EQ(bench.memory.work.size(), 2U);
  EXPECT_EQ(bench.memory.work[0].first, 0x6000);
}

TEST(DmaUm6576, WorkRamWritesStopAtTheProgramWindow) {
  DmaBench bench;
  bench.dma.timingControlOn = [] { return true; };
  setUpDma(bench.dma, 0x20, 0x00, 0x8000, 0x7FFF, 2);
  bench.dma.write(0x4048, 0xA0);
  ASSERT_EQ(bench.memory.work.size(), 1U) << "$8000 以上的目的写不进去";
  EXPECT_EQ(bench.memory.work[0].first, 0x7FFF);
}

TEST(DmaUm6576, GeneralRegistersCanBeReadBack) {
  DmaBench bench;
  bench.dma.write(0x404A, 0x42);
  EXPECT_EQ(bench.dma.read(0x404A).value, 0x42);
}

/** 记录 $2004 写入的设备与内存。 */
class OamSink final : public hw::BusDevice {
 public:
  hw::BusRead read(std::uint16_t) override { return hw::releaseBus(); }
  void write(std::uint16_t, std::uint8_t value) override { values.push_back(value); }
  std::vector<std::uint8_t> values;
};

class PageMemory final : public hw::BusDevice {
 public:
  hw::BusRead read(std::uint16_t address) override { return hw::driveBus(static_cast<std::uint8_t>(address & 0xFF)); }
  void write(std::uint16_t, std::uint8_t) override {}
};

TEST(DmaUm6576, SpriteDmaCopiesAPageAndHoldsTheCpuFor513Or514Cycles) {
  DmaBench bench;
  OamSink oam;
  PageMemory memory;
  bench.bus.mount(0x0000, 0x1FFF, memory);
  bench.bus.mount(0x2004, 0x2004, oam);
  bench.scheduler.advance(16 * 10);  // 偶数周期 10
  bench.dma.write(0x4014, 0x02);
  EXPECT_TRUE(bench.rdy.active());
  EXPECT_TRUE(bench.dma.spriteDmaActive());
  bench.scheduler.advance(16 * 514);
  EXPECT_FALSE(bench.rdy.active()) << "偶数周期写入，写周期之后起 513 个周期释放"; 
  EXPECT_FALSE(bench.dma.spriteDmaActive());
  ASSERT_EQ(oam.values.size(), 256U);
  EXPECT_EQ(oam.values[5], 5);
}

}  // namespace
