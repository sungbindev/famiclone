/*
 * ------------------------------------------------------------------------
 * 名称：bus-test.cpp
 * 说明：总线的测试：地址范围挂载与分发、挂载冲突与越界报错、开放总线（含只驱动部分数据位）、PPU 总线的地址宽度。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <string_view>
#include <utility>
#include <vector>

#include "contracts/hardware/hardware-error.hpp"
#include "logic/hardware/bus/mapped-bus.hpp"
#include "shared/errors/core-error.hpp"

namespace {

namespace hw = ember::contracts::hardware;
using ember::logic::hardware::CpuAddressBus;
using ember::logic::hardware::PpuAddressBus;
using ember::shared::CoreError;

/** 读出固定的值与驱动掩码，记录收到的地址。 */
class FixedDevice final : public hw::BusDevice {
 public:
  FixedDevice(std::uint8_t value, std::uint8_t drivenMask) : value_(value), drivenMask_(drivenMask) {}

  hw::BusRead read(std::uint16_t address) override {
    readAddresses.push_back(address);
    return hw::driveBus(value_, drivenMask_);
  }

  void write(std::uint16_t address, std::uint8_t value) override {
    writes.emplace_back(address, value);
  }

  std::vector<std::uint16_t> readAddresses;
  std::vector<std::pair<std::uint16_t, std::uint8_t>> writes;

 private:
  std::uint8_t value_;
  std::uint8_t drivenMask_;
};

/** 期望挂载失败并带指定错误码。 */
void expectMountError(CpuAddressBus& bus, std::uint16_t first, std::uint16_t last, hw::BusDevice& device, std::string_view code) {
  try {
    bus.mount(first, last, device);
    FAIL() << "应当报错：" << code;
  } catch (const CoreError& error) {
    EXPECT_EQ(error.code(), code);
  }
}

TEST(Bus, DispatchesReadsAndWritesByRangeWithAbsoluteAddress) {
  CpuAddressBus bus;
  FixedDevice low(0x11, 0xFF);
  FixedDevice high(0x22, 0xFF);
  bus.mount(0x0000, 0x1FFF, low);
  bus.mount(0x8000, 0xFFFF, high);

  EXPECT_EQ(bus.read(0x1234), 0x11);
  EXPECT_EQ(bus.read(0x8000), 0x22);
  EXPECT_EQ(bus.read(0xFFFF), 0x22);
  bus.write(0x0002, 0x55);
  bus.write(0xC000, 0x66);

  EXPECT_EQ(low.readAddresses, std::vector<std::uint16_t>{0x1234});
  EXPECT_EQ(high.readAddresses, (std::vector<std::uint16_t>{0x8000, 0xFFFF}));
  ASSERT_EQ(low.writes.size(), 1U);
  EXPECT_EQ(low.writes[0], (std::pair<std::uint16_t, std::uint8_t>{0x0002, 0x55}));
  ASSERT_EQ(high.writes.size(), 1U);
  EXPECT_EQ(high.writes[0], (std::pair<std::uint16_t, std::uint8_t>{0xC000, 0x66}));
}

TEST(Bus, MountOrderDoesNotMatterAndAdjacentRangesAreAllowed) {
  CpuAddressBus bus;
  FixedDevice a(0xA1, 0xFF);
  FixedDevice b(0xB2, 0xFF);
  FixedDevice c(0xC3, 0xFF);
  bus.mount(0x4000, 0x7FFF, b);
  bus.mount(0x8000, 0xBFFF, c);
  bus.mount(0x0000, 0x3FFF, a);

  EXPECT_EQ(bus.read(0x3FFF), 0xA1);
  EXPECT_EQ(bus.read(0x4000), 0xB2);
  EXPECT_EQ(bus.read(0x7FFF), 0xB2);
  EXPECT_EQ(bus.read(0x8000), 0xC3);
}

TEST(Bus, RejectsOverlappingRangesAndKeepsExistingMounts) {
  CpuAddressBus bus;
  FixedDevice existing(0x77, 0xFF);
  FixedDevice other(0x99, 0xFF);
  bus.mount(0x2000, 0x2FFF, existing);

  expectMountError(bus, 0x2000, 0x2FFF, other, hw::ERROR_BUS_RANGE_CONFLICT);
  expectMountError(bus, 0x1000, 0x2000, other, hw::ERROR_BUS_RANGE_CONFLICT);
  expectMountError(bus, 0x2FFF, 0x3000, other, hw::ERROR_BUS_RANGE_CONFLICT);
  expectMountError(bus, 0x2400, 0x2500, other, hw::ERROR_BUS_RANGE_CONFLICT);
  expectMountError(bus, 0x0000, 0xFFFF, other, hw::ERROR_BUS_RANGE_CONFLICT);

  EXPECT_EQ(bus.read(0x2000), 0x77);
  EXPECT_EQ(bus.read(0x2FFF), 0x77);
  EXPECT_TRUE(other.readAddresses.empty());
}

TEST(Bus, RejectsReversedAndOutOfWidthRanges) {
  CpuAddressBus cpuBus;
  PpuAddressBus ppuBus;
  FixedDevice device(0, 0xFF);

  expectMountError(cpuBus, 0x3000, 0x2000, device, hw::ERROR_BUS_RANGE_INVALID);
  EXPECT_NO_THROW(cpuBus.mount(0xFFFF, 0xFFFF, device));

  EXPECT_THROW(ppuBus.mount(0x3F00, 0x4000, device), CoreError);
  try {
    ppuBus.mount(0x4000, 0x4000, device);
    FAIL() << "PPU 总线只有 14 位地址，应当报错";
  } catch (const CoreError& error) {
    EXPECT_EQ(error.code(), hw::ERROR_BUS_RANGE_INVALID);
  }
  EXPECT_NO_THROW(ppuBus.mount(0x0000, 0x3FFF, device));
}

TEST(Bus, UnmappedReadReturnsLastValueOnTheBus) {
  CpuAddressBus bus;
  FixedDevice device(0xAB, 0xFF);
  bus.mount(0x0000, 0x00FF, device);

  // 上电后总线上是 0。
  EXPECT_EQ(bus.read(0x4000), 0x00);
  EXPECT_EQ(bus.read(0x0010), 0xAB);
  EXPECT_EQ(bus.read(0x4000), 0xAB);
  EXPECT_EQ(bus.read(0x5000), 0xAB);
}

TEST(Bus, WritesAlsoPutTheValueOnTheBus) {
  CpuAddressBus bus;
  FixedDevice device(0x00, 0xFF);
  bus.mount(0x0000, 0x00FF, device);

  // 写到没有设备的地址：数据被丢弃，但总线上留下了它。
  bus.write(0x6000, 0x5A);
  EXPECT_EQ(bus.read(0x7000), 0x5A);
  bus.write(0x0001, 0xC3);
  EXPECT_EQ(bus.read(0x7000), 0xC3);
}

TEST(Bus, PartiallyDrivenReadKeepsUndrivenBitsFromTheBus) {
  CpuAddressBus bus;
  FixedDevice controller(0x01, 0x1F);  // 只驱动低 5 位，如手柄口
  FixedDevice silent(0xFF, 0x00);      // 完全不驱动
  bus.mount(0x4016, 0x4016, controller);
  bus.mount(0x5000, 0x5000, silent);

  bus.write(0x7000, 0xE0);
  EXPECT_EQ(bus.read(0x4016), 0xE1);  // 高 3 位来自总线上最近的值
  EXPECT_EQ(bus.read(0x4016), 0xE1);
  EXPECT_EQ(bus.read(0x5000), 0xE1);  // 完全不驱动：保持原值
}

TEST(Bus, PpuBusIgnoresAddressBitsAboveFourteen) {
  PpuAddressBus bus;
  FixedDevice device(0x42, 0xFF);
  bus.mount(0x0000, 0x3FFF, device);

  EXPECT_EQ(bus.read(0x4005), 0x42);
  bus.write(0xC006, 0x01);
  EXPECT_EQ(device.readAddresses, std::vector<std::uint16_t>{0x0005});
  ASSERT_EQ(device.writes.size(), 1U);
  EXPECT_EQ(device.writes[0].first, 0x0006);
}

}  // namespace
