/*
 * ------------------------------------------------------------------------
 * 名称：sb97-board.cpp
 * 说明：小霸王 97 主板的实现：CPU 与 PPU 两侧的存储译码与整机寄存器。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-17
 * ------------------------------------------------------------------------
 */

#include "bootstrap/machines/sb97/sb97-board.hpp"

#include <algorithm>
#include <string>
#include <utility>

#include "contracts/hardware/hardware-error.hpp"
#include "shared/errors/core-error.hpp"

namespace ember::bootstrap {

namespace hw = contracts::hardware;

namespace {

/** 整机寄存器与软驱控制器的地址。 */
constexpr std::uint16_t REGISTER_PROGRAM_PAGE = 0x5000;
constexpr std::uint16_t REGISTER_KEYBOARD_LEDS = 0x480D;
constexpr std::uint16_t REGISTER_CONTROL = 0x5200;
constexpr std::uint16_t REGISTER_SOUND_DATA = 0x5300;
constexpr std::uint16_t FDC_WRITE_DATA_RATE = 0x5500;
constexpr std::uint16_t FDC_WRITE_DOR = 0x5501;
constexpr std::uint16_t FDC_WRITE_DATA = 0x5505;
constexpr std::uint16_t FDC_READ_STATUS = 0x5604;
constexpr std::uint16_t FDC_READ_DATA = 0x5605;
/** 软驱控制器寄存器组里 DOR、主状态、数据的端口号。 */
constexpr unsigned FDC_PORT_DOR = 2;
constexpr unsigned FDC_PORT_STATUS = 4;
constexpr unsigned FDC_PORT_DATA = 5;
constexpr unsigned FDC_PORT_DATA_RATE = 7;
/** 声音数据口读出的值：位 7 为 1 表示就绪。 */
constexpr std::uint8_t SOUND_READY = 0x80;

}  // namespace

Sb97Board::Sb97Board(std::vector<std::uint8_t> rom, logic::hardware::FdcUpd765& fdc, Sb97FdcInterrupt& fdcInterrupt)
    : fdc_(fdc), fdcInterrupt_(fdcInterrupt), rom_(std::move(rom)), ram_(RAM_SIZE, 0), characterRam_(CHARACTER_RAM_SIZE, 0) {
  if (rom_.size() != ROM_SIZE) {
    throw shared::CoreError(hw::ERROR_INVALID_ARGUMENT, "sb97 的 BIOS 必须是 " + std::to_string(ROM_SIZE) + " 字节，收到 " + std::to_string(rom_.size()) + " 字节");
  }
  reset(true);
}

void Sb97Board::reset(bool power) {
  if (power) {
    std::fill(ram_.begin(), ram_.end(), std::uint8_t{0});
    std::fill(characterRam_.begin(), characterRam_.end(), std::uint8_t{0});
    nametable_.fill(0);
  }
  programPage_ = 0;
  control_ = 0;
}

// ---------------------------------------------------------------- 存储译码

Sb97Board::CpuLocation Sb97Board::locate(std::uint16_t address) const {
  using Kind = CpuLocation::Kind;
  if (address < 0x4020U) return {};
  if (address == REGISTER_KEYBOARD_LEDS) return {Kind::Register, address};
  if (address < 0x6000U) {
    switch (address) {
      case REGISTER_PROGRAM_PAGE:
      case REGISTER_CONTROL:
      case REGISTER_SOUND_DATA:
      case FDC_WRITE_DATA_RATE:
      case FDC_WRITE_DOR:
      case FDC_WRITE_DATA:
      case FDC_READ_STATUS:
      case FDC_READ_DATA:
        return {Kind::Register, address};
      default:
        return {};
    }
  }
  if (address < 0x8000U) return {Kind::Ram, address - 0x6000U};
  if (address < 0xC000U) return {Kind::Rom, static_cast<std::size_t>(programPage_) * PAGE_SIZE + (address - 0x8000U)};
  return {Kind::Rom, address - 0xC000U};
}

hw::BusRead Sb97Board::readCpu(std::uint16_t address, bool sideEffects) {
  const CpuLocation where = locate(address);
  switch (where.kind) {
    case CpuLocation::Kind::Rom:
      return hw::driveBus(rom_[where.offset]);
    case CpuLocation::Kind::Ram:
      return hw::driveBus(ram_[where.offset]);
    case CpuLocation::Kind::Register:
      switch (address) {
        case REGISTER_SOUND_DATA:
          return hw::driveBus(SOUND_READY);
        case FDC_READ_STATUS:
          return hw::driveBus(sideEffects ? fdc_.readRegister(FDC_PORT_STATUS) : fdc_.peekRegister(FDC_PORT_STATUS));
        case FDC_READ_DATA: {
          if (!sideEffects) return hw::releaseBus();
          const std::uint8_t value = fdc_.readRegister(FDC_PORT_DATA);
          fdcInterrupt_.refresh();
          return hw::driveBus(value);
        }
        default:
          // 只写的寄存器：读不到内容。
          return hw::releaseBus();
      }
    case CpuLocation::Kind::None:
      break;
  }
  return hw::releaseBus();
}

void Sb97Board::writeCpu(std::uint16_t address, std::uint8_t value) {
  const CpuLocation where = locate(address);
  switch (where.kind) {
    case CpuLocation::Kind::Ram:
      ram_[where.offset] = value;
      break;
    case CpuLocation::Kind::Register:
      switch (address) {
        case REGISTER_PROGRAM_PAGE:
          programPage_ = static_cast<std::uint8_t>(value & (PAGE_COUNT - 1));
          break;
        case REGISTER_CONTROL:
          control_ = value;
          break;
        case FDC_WRITE_DATA_RATE:
          fdc_.writeRegister(FDC_PORT_DATA_RATE, value);
          break;
        case FDC_WRITE_DOR:
          fdc_.writeRegister(FDC_PORT_DOR, value);
          fdcInterrupt_.refresh();
          break;
        case FDC_WRITE_DATA:
          fdc_.writeRegister(FDC_PORT_DATA, value);
          fdcInterrupt_.refresh();
          break;
        default:
          break;
      }
      break;
    case CpuLocation::Kind::Rom:
    case CpuLocation::Kind::None:
      break;
  }
}

// ---------------------------------------------------------------- PPU 侧

std::size_t Sb97Board::nametableOffset(std::uint16_t address) {
  // 四屏：4 个名称表各占 1K，没有镜像。
  return static_cast<std::size_t>(address & 0x0FFFU);
}

std::size_t Sb97Board::locateVideo(std::uint16_t address, bool& isPattern) const {
  const std::uint16_t masked = address & 0x3FFFU;
  isPattern = masked < 0x2000U;
  return isPattern ? masked : nametableOffset(masked);
}

// ---------------------------------------------------------------- 总线设备

hw::BusRead Sb97Board::CpuSide::read(std::uint16_t address) { return board_.readCpu(address, true); }

hw::BusRead Sb97Board::CpuSide::peek(std::uint16_t address) { return board_.readCpu(address, false); }

void Sb97Board::CpuSide::write(std::uint16_t address, std::uint8_t value) { board_.writeCpu(address, value); }

hw::BusRead Sb97Board::VideoSide::read(std::uint16_t address) {
  const std::uint16_t masked = address & 0x3FFFU;
  return hw::driveBus(masked < 0x2000U ? board_.characterRam_[masked] : board_.nametable_[nametableOffset(masked)]);
}

hw::BusRead Sb97Board::VideoSide::peek(std::uint16_t address) { return read(address); }

void Sb97Board::VideoSide::write(std::uint16_t address, std::uint8_t value) {
  const std::uint16_t masked = address & 0x3FFFU;
  if (masked < 0x2000U) board_.characterRam_[masked] = value;
  else board_.nametable_[nametableOffset(masked)] = value;
}

}  // namespace ember::bootstrap
