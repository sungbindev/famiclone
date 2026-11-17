/*
 * ------------------------------------------------------------------------
 * 名称：bbk-peripherals-test.cpp
 * 说明：步步高键盘、鼠标与 $4016-$4017 输入口的测试：按固件的流程读整个矩阵、位置与半边的规则、鼠标的接通检测、命令与帧格式、位移与按键的编码，以及输入口把手柄、键盘、鼠标与 APU 写入接在一起。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-16
 * 备注：键盘的扫描流程是固件用的：写 $05 复位，随后每个位置写 $04 读前半边、写 $06 读后半边，位置 9 之后多发一对 $04、$06 跳过位置 10。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <vector>

#include "bootstrap/machines/common/keyboard-pad-input-port.hpp"
#include "logic/hardware/joypad/joypad.hpp"
#include "logic/hardware/keyboard-bbk/keyboard-bbk.hpp"
#include "logic/hardware/mouse-bbk/mouse-bbk.hpp"
#include "logic/hardware/pad-2a03/pad-2a03.hpp"

namespace {

namespace hw = ember::contracts::hardware;
using ember::bootstrap::KeyboardPadInputPort;
using ember::logic::hardware::ControllerSocket;
using ember::logic::hardware::Joypad;
using ember::logic::hardware::KeyboardBbk;
using ember::logic::hardware::MouseBbk;

constexpr std::uint8_t RELEASED = 0x1E;

/** 按固件的流程读整个矩阵：12 个字节，下标 x 对应位置 x + 1（x 不小于 9 时对应位置 x + 2），低 4 位是前半边，高 4 位是后半边，按下为 1。 */
std::array<std::uint8_t, 12> scan(KeyboardBbk& keyboard) {
  std::array<std::uint8_t, 12> result{};
  keyboard.write(0x05);
  for (unsigned x = 0; x < 12; ++x) {
    if (x == 9) {
      keyboard.write(0x04);
      keyboard.write(0x06);
    }
    keyboard.write(0x04);
    const std::uint8_t first = keyboard.read();
    keyboard.write(0x06);
    const std::uint8_t second = keyboard.read();
    result[x] = static_cast<std::uint8_t>((~((first & 0x1E) >> 1) & 0x0F) | ((~((second & 0x1E) >> 1) & 0x0F) << 4));
  }
  return result;
}

TEST(KeyboardBbk, WithNoKeyPressedEveryPositionReadsOne) {
  KeyboardBbk keyboard;
  keyboard.write(0x05);
  for (unsigned position = 1; position <= KeyboardBbk::POSITIONS; ++position) {
    keyboard.write(0x04);
    if (position != 10) EXPECT_EQ(keyboard.read(), RELEASED) << "位置 " << position << " 前半边";
    keyboard.write(0x06);
  }
}

TEST(KeyboardBbk, ScanControlWritesMoveThePositionAndFlipTheHalf) {
  KeyboardBbk keyboard;
  keyboard.write(0x05);
  EXPECT_EQ(keyboard.position(), 0U);
  EXPECT_FALSE(keyboard.firstHalf());
  keyboard.write(0x04);
  EXPECT_EQ(keyboard.position(), 1U);
  EXPECT_TRUE(keyboard.firstHalf());
  keyboard.write(0x06);
  EXPECT_EQ(keyboard.position(), 1U);
  EXPECT_FALSE(keyboard.firstHalf());
  for (unsigned index = 0; index < 12; ++index) keyboard.write(0x04);
  EXPECT_EQ(keyboard.position(), 13U);
  keyboard.write(0x04);
  EXPECT_EQ(keyboard.position(), 0U) << "超过 13 回到 0";
  keyboard.write(0x00);
  keyboard.write(0x01);
  EXPECT_EQ(keyboard.position(), 0U) << "位 2 为 0 的写入不是扫描控制";
}

TEST(KeyboardBbk, PositionZeroHasNoKeys) {
  KeyboardBbk keyboard;
  keyboard.setKey("KeyA", true);
  keyboard.write(0x05);
  EXPECT_EQ(keyboard.read(), RELEASED);
  keyboard.write(0x06);
  EXPECT_EQ(keyboard.read(), RELEASED);
}

TEST(KeyboardBbk, APressedKeyShowsUpInTheScan) {
  KeyboardBbk keyboard;
  keyboard.setKey("Digit4", true);
  EXPECT_EQ(scan(keyboard)[0], 0x01) << "位置 1 前半边的第 1 个键";
  keyboard.setKey("Digit4", false);
  keyboard.setKey("KeyV", true);
  EXPECT_EQ(scan(keyboard)[0], 0x80) << "位置 1 后半边的第 4 个键";
  keyboard.releaseAll();
  keyboard.setKey("Enter", true);
  keyboard.setKey("F12", true);
  const auto matrix = scan(keyboard);
  EXPECT_EQ(matrix[4], 0x02) << "位置 5 前半边的第 2 个键";
  EXPECT_EQ(matrix[9], 0x10) << "位置 11 后半边的第 1 个键";
}

TEST(KeyboardBbk, PositionTenSecondHalfFirstKeyIsAlwaysDown) {
  KeyboardBbk keyboard;
  keyboard.write(0x05);
  for (unsigned index = 0; index < 9; ++index) {
    keyboard.write(0x04);
    keyboard.write(0x06);
  }
  keyboard.write(0x04);
  ASSERT_EQ(keyboard.position(), 10U);
  keyboard.write(0x06);
  EXPECT_EQ(keyboard.read(), static_cast<std::uint8_t>(RELEASED & ~0x02));
}

TEST(KeyboardBbk, LeftAndRightModifiersShareOneKeyAndStayDownUntilBothAreReleased) {
  KeyboardBbk keyboard;
  keyboard.setKey("ShiftLeft", true);
  keyboard.setKey("ShiftRight", true);
  EXPECT_EQ(scan(keyboard)[7], 0x80) << "位置 8 后半边的第 4 个键";
  keyboard.setKey("ShiftLeft", false);
  EXPECT_EQ(scan(keyboard)[7], 0x80) << "右 Shift 还按着";
  keyboard.setKey("ShiftRight", false);
  EXPECT_EQ(scan(keyboard)[7], 0x00);
}

TEST(KeyboardBbk, UnknownKeysAreRejected) {
  KeyboardBbk keyboard;
  EXPECT_FALSE(keyboard.setKey("MediaPlayPause", true));
  EXPECT_TRUE(keyboard.setKey("KeyQ", true));
}

TEST(KeyboardBbk, EveryKeyHasOneMatrixPositionAndFitsTheMatrix) {
  unsigned position = 0;
  bool first = false;
  unsigned bit = 0;
  ASSERT_TRUE(KeyboardBbk::locate("Space", position, first, bit));
  EXPECT_EQ(position, 9U);
  EXPECT_TRUE(first);
  EXPECT_EQ(bit, 3U);
  ASSERT_TRUE(KeyboardBbk::locate("Numpad0", position, first, bit));
  EXPECT_EQ(position, 13U);
  EXPECT_FALSE(first);
  EXPECT_EQ(bit, 3U);
}

TEST(KeyboardBbk, ResetRestoresTheScanStartButKeepsPressedKeys) {
  KeyboardBbk keyboard;
  keyboard.setKey("KeyQ", true);
  keyboard.write(0x05);
  keyboard.write(0x04);
  keyboard.reset();
  EXPECT_EQ(keyboard.position(), 0U);
  EXPECT_EQ(scan(keyboard)[5], 0x01) << "按着的 Q 仍在位置 6";
}

// ---------------------------------------------------------------- 鼠标

/** 按固件的流程让鼠标接通：连续读 $4016 九次。 */
void connect(MouseBbk& mouse) {
  for (unsigned index = 0; index < 9; ++index) mouse.noteStatusRead();
}

/** 发一个命令：先写 $05，再逐位写入（位 2 为 1，位 0 是命令位的反相），共 22 位。 */
void sendCommand(MouseBbk& mouse, std::uint8_t command) {
  mouse.write(0x05);
  for (unsigned bit = 0; bit < 22; ++bit) {
    const bool value = bit < 8 && ((command >> bit) & 1U) != 0;
    mouse.write(static_cast<std::uint8_t>(0x04 | (value ? 0x00 : 0x01)));
  }
}

/** 读一帧：32 位之后再读一次帧间隔；返回帧里的数据字节，起始位或格式不对时返回 -1。 */
int readFrame(MouseBbk& mouse) {
  std::array<std::uint8_t, 32> bits{};
  for (auto& bit : bits) bit = mouse.readLine();
  const std::uint8_t gap = mouse.readLine();
  if (gap != 0) return -1;
  if (bits[0] != 1 || bits[1] != 1 || bits[2] != 0 || bits[3] != 1) return -1;
  int value = 0;
  bool parity = false;
  for (unsigned index = 0; index < 8; ++index) {
    const unsigned base = 4 + index * 3;
    if (bits[base + 1] != 0 || bits[base + 2] != 1) return -1;
    const bool dataBit = bits[base] == 0;
    if (dataBit) value |= 1 << index;
    if (!dataBit) parity = !parity;
  }
  if (bits[28] != (parity ? 1U : 0U)) return -1;
  return value;
}

TEST(MouseBbk, IsNotConnectedUntilNineConsecutiveReadsOf4016) {
  MouseBbk mouse;
  EXPECT_EQ(mouse.readLine(), 0);
  for (unsigned index = 0; index < 8; ++index) mouse.noteStatusRead();
  EXPECT_FALSE(mouse.active());
  mouse.noteStatusRead();
  EXPECT_TRUE(mouse.active());
  EXPECT_EQ(mouse.readLine(), 1) << "接通后空闲时位 0 为 1";
}

TEST(MouseBbk, AReadOf4017OrAWriteBreaksTheConsecutiveReadCount) {
  MouseBbk mouse;
  for (unsigned index = 0; index < 8; ++index) mouse.noteStatusRead();
  static_cast<void>(mouse.readLine());
  mouse.noteStatusRead();
  EXPECT_FALSE(mouse.active());
  for (unsigned index = 0; index < 7; ++index) mouse.noteStatusRead();
  mouse.write(0x00);
  mouse.noteStatusRead();
  EXPECT_FALSE(mouse.active());
}

TEST(MouseBbk, SetRemoteModeAnswersWithAnAcknowledgeAndDisconnects) {
  MouseBbk mouse;
  connect(mouse);
  sendCommand(mouse, 0xF0);
  EXPECT_EQ(readFrame(mouse), 0xFA);
  EXPECT_FALSE(mouse.active()) << "应答发完后回到未接通";
}

TEST(MouseBbk, ReadDataAnswersWithAcknowledgeThenButtonsAndSignedMovement) {
  MouseBbk mouse;
  mouse.move(5, -3, MouseBbk::BUTTON_LEFT);
  connect(mouse);
  sendCommand(mouse, 0xEB);
  EXPECT_EQ(readFrame(mouse), 0xFA);
  EXPECT_EQ(readFrame(mouse), 0x34) << "左键（位 2），横向取反后为负、纵向为负";
  EXPECT_EQ(readFrame(mouse), 0xFD) << "纵向 -3";
  EXPECT_EQ(readFrame(mouse), 0xFB) << "横向取反后 -5";
  EXPECT_FALSE(mouse.active());
}

TEST(MouseBbk, MovementIsAccumulatedUntilReadAndThenCleared) {
  MouseBbk mouse;
  mouse.move(-2, 1, 0);
  mouse.move(-3, 2, MouseBbk::BUTTON_RIGHT);
  connect(mouse);
  sendCommand(mouse, 0xEB);
  static_cast<void>(readFrame(mouse));
  EXPECT_EQ(readFrame(mouse), 0x01) << "右键（位 0），横向取反后为正、纵向为正";
  EXPECT_EQ(readFrame(mouse), 3);
  EXPECT_EQ(readFrame(mouse), 5);
  connect(mouse);
  sendCommand(mouse, 0xEB);
  static_cast<void>(readFrame(mouse));
  EXPECT_EQ(readFrame(mouse), 0x01) << "按键保持";
  EXPECT_EQ(readFrame(mouse), 0);
  EXPECT_EQ(readFrame(mouse), 0);
}

TEST(MouseBbk, MovementOutOfRangeSetsTheOverflowBitAndReadsZero) {
  MouseBbk mouse;
  mouse.move(-300, 400, 0);
  connect(mouse);
  sendCommand(mouse, 0xEB);
  static_cast<void>(readFrame(mouse));
  EXPECT_EQ(readFrame(mouse), 0xC0) << "横向 300 与纵向 400 都溢出";
  EXPECT_EQ(readFrame(mouse), 0);
  EXPECT_EQ(readFrame(mouse), 0);
}

TEST(MouseBbk, ReceivingACommandTogglesTheLineOnEveryRead) {
  MouseBbk mouse;
  connect(mouse);
  mouse.write(0x05);
  EXPECT_EQ(mouse.readLine(), 1);
  EXPECT_EQ(mouse.readLine(), 0);
  EXPECT_EQ(mouse.readLine(), 1);
}

// ---------------------------------------------------------------- 输入口

/** 记下写入的设备，代替 APU。 */
struct RecordingDevice final : hw::BusDevice {
  std::vector<std::pair<std::uint16_t, std::uint8_t>> writes;
  hw::BusRead read(std::uint16_t) override { return hw::releaseBus(); }
  void write(std::uint16_t address, std::uint8_t value) override { writes.emplace_back(address, value); }
};

struct Rig {
  ControllerSocket socket;
  Joypad joypad;
  RecordingDevice apu;
  KeyboardBbk keyboard;
  MouseBbk mouse;
  KeyboardPadInputPort port{socket, apu, keyboard, &mouse};
  Rig() { socket.plug(&joypad); }
};

TEST(KeyboardPadInputPort, Reads4016AsTheJoypadDataLineOnly) {
  Rig rig;
  rig.joypad.setButtons(0x01);
  rig.port.write(0x4016, 0x01);
  rig.port.write(0x4016, 0x00);
  const hw::BusRead first = rig.port.read(0x4016);
  EXPECT_EQ(first.value, 0x01) << "A 键";
  EXPECT_EQ(first.drivenMask, 0xFF);
  EXPECT_EQ(rig.port.read(0x4016).value, 0x00) << "B 键没按";
}

TEST(KeyboardPadInputPort, Reads4017AsKeyboardAndMouseLinesWithHighBitsSet) {
  Rig rig;
  EXPECT_EQ(rig.port.read(0x4017).value, 0xFE | 0x00) << "没有扫描位置时 4 个键位都是 1，鼠标没接通";
  rig.keyboard.setKey("Digit4", true);
  rig.port.write(0x4016, 0x05);
  rig.port.write(0x4016, 0x04);
  EXPECT_EQ(rig.port.read(0x4017).value, 0xFC) << "位置 1 前半边第 1 个键按下，位 1 为 0";
  rig.port.write(0x4016, 0x06);
  EXPECT_EQ(rig.port.read(0x4017).value, 0xFE) << "后半边没有键按下";
}

TEST(KeyboardPadInputPort, NineReadsOf4016ConnectTheMouseAndTheLineShowsIt) {
  Rig rig;
  for (unsigned index = 0; index < 9; ++index) static_cast<void>(rig.port.read(0x4016));
  EXPECT_EQ(rig.port.read(0x4017).value & 0x01, 0x01);
}

TEST(KeyboardPadInputPort, Writes4016GoToTheJoypadLatchKeyboardAndMouseAndWrites4017GoToTheApu) {
  Rig rig;
  rig.port.write(0x4016, 0x05);
  EXPECT_FALSE(rig.keyboard.firstHalf());
  rig.port.write(0x4016, 0x04);
  EXPECT_EQ(rig.keyboard.position(), 1U);
  rig.port.write(0x4017, 0x40);
  ASSERT_EQ(rig.apu.writes.size(), 1U);
  EXPECT_EQ(rig.apu.writes[0].first, 0x4017);
  EXPECT_EQ(rig.apu.writes[0].second, 0x40);
}

TEST(KeyboardPadInputPort, WithoutAMouseTheMouseLineStaysZeroAndWritesStillReachTheKeyboard) {
  ControllerSocket socket;
  Joypad joypad;
  RecordingDevice apu;
  KeyboardBbk keyboard;
  KeyboardPadInputPort port{socket, apu, keyboard, nullptr};
  socket.plug(&joypad);
  for (unsigned index = 0; index < 9; ++index) static_cast<void>(port.read(0x4016));
  EXPECT_EQ(port.read(0x4017).value & 0x01, 0x00) << "没有鼠标，位 0 总是 0";
  port.write(0x4016, 0x05);
  port.write(0x4016, 0x04);
  EXPECT_EQ(keyboard.position(), 1U);
}

}  // namespace
