/*
 * ------------------------------------------------------------------------
 * 名称：lines-and-contracts-test.cpp
 * 说明：线或信号线的测试，以及硬件契约头文件的编译与形状检查（接口都是抽象类，辅助函数取值正确）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <type_traits>

#include "contracts/hardware/bus-device.hpp"
#include "contracts/hardware/cartridge-slot.hpp"
#include "contracts/hardware/cartridge.hpp"
#include "contracts/hardware/clock.hpp"
#include "contracts/hardware/controller-port.hpp"
#include "contracts/hardware/cpu-bus.hpp"
#include "contracts/hardware/cpu.hpp"
#include "contracts/hardware/device.hpp"
#include "contracts/hardware/hardware-error.hpp"
#include "contracts/hardware/irq-line.hpp"
#include "contracts/hardware/machine.hpp"
#include "contracts/hardware/ppu-bus.hpp"
#include "contracts/hardware/ppu.hpp"
#include "contracts/hardware/rdy-line.hpp"
#include "contracts/hardware/scheduler.hpp"
#include "contracts/hardware/storage-region.hpp"
#include "logic/hardware/lines/wired-or-line.hpp"
#include "shared/errors/core-error.hpp"

namespace {

namespace hw = ember::contracts::hardware;
using ember::logic::hardware::WiredOrIrqLine;
using ember::logic::hardware::WiredOrRdyLine;
using ember::shared::CoreError;

static_assert(std::is_abstract_v<hw::Clock>);
static_assert(std::is_abstract_v<hw::Scheduler>);
static_assert(std::is_abstract_v<hw::EventListener>);
static_assert(std::is_abstract_v<hw::BusDevice>);
static_assert(std::is_abstract_v<hw::Bus>);
static_assert(std::is_abstract_v<hw::CpuBus>);
static_assert(std::is_abstract_v<hw::PpuBus>);
static_assert(std::is_abstract_v<hw::SignalLine>);
static_assert(std::is_abstract_v<hw::IrqLine>);
static_assert(std::is_abstract_v<hw::RdyLine>);
static_assert(std::is_abstract_v<hw::Cpu>);
static_assert(std::is_abstract_v<hw::Ppu>);
static_assert(std::is_abstract_v<hw::Device>);
static_assert(std::is_abstract_v<hw::StorageRegion>);
static_assert(std::is_abstract_v<hw::ControllerDevice>);
static_assert(std::is_abstract_v<hw::ControllerPort>);
static_assert(std::is_abstract_v<hw::Cartridge>);
static_assert(std::is_abstract_v<hw::CartridgeSlot>);
static_assert(std::is_abstract_v<hw::Machine>);
static_assert(std::is_base_of_v<hw::Clock, hw::Scheduler>);
static_assert(std::is_base_of_v<hw::Bus, hw::CpuBus>);
static_assert(std::is_base_of_v<hw::Bus, hw::PpuBus>);
static_assert(std::is_base_of_v<hw::SignalLine, hw::IrqLine>);
static_assert(std::is_base_of_v<hw::SignalLine, hw::RdyLine>);
// IRQ 线与 RDY 线是不同的类型，不会接错。
static_assert(!std::is_base_of_v<hw::IrqLine, hw::RdyLine>);
static_assert(!std::is_base_of_v<hw::RdyLine, hw::IrqLine>);

TEST(WiredOrLine, IsActiveWhenAnySourceIsActive) {
  WiredOrIrqLine line;
  const hw::SignalSource first = line.addSource();
  const hw::SignalSource second = line.addSource();
  EXPECT_NE(first, second);
  EXPECT_FALSE(line.active());

  line.set(first, true);
  EXPECT_TRUE(line.active());
  line.set(second, true);
  line.set(first, false);
  EXPECT_TRUE(line.active());
  line.set(second, false);
  EXPECT_FALSE(line.active());
}

TEST(WiredOrLine, SettingTheSameStateTwiceIsHarmless) {
  WiredOrRdyLine line;
  const hw::SignalSource source = line.addSource();
  line.set(source, true);
  line.set(source, true);
  EXPECT_TRUE(line.active());
  line.set(source, false);
  line.set(source, false);
  EXPECT_FALSE(line.active());
}

TEST(WiredOrLine, RejectsTooManySourcesAndUnknownSources) {
  WiredOrIrqLine line;
  for (unsigned i = 0; i < WiredOrIrqLine::MAX_SOURCES; ++i) line.addSource();
  try {
    line.addSource();
    FAIL() << "第 33 个来源应当报错";
  } catch (const CoreError& error) {
    EXPECT_EQ(error.code(), hw::ERROR_LINE_SOURCE_LIMIT);
  }
  EXPECT_THROW(line.set(WiredOrIrqLine::MAX_SOURCES, true), CoreError);

  // 最后一个来源（第 32 位）也能正常工作。
  line.set(WiredOrIrqLine::MAX_SOURCES - 1, true);
  EXPECT_TRUE(line.active());
}

TEST(BusReadHelpers, BuildDrivenAndReleasedResults) {
  EXPECT_EQ(hw::driveBus(0x5A).value, 0x5A);
  EXPECT_EQ(hw::driveBus(0x5A).drivenMask, 0xFF);
  EXPECT_EQ(hw::driveBus(0x01, 0x1F).drivenMask, 0x1F);
  EXPECT_EQ(hw::releaseBus().drivenMask, 0x00);
}

}  // namespace
