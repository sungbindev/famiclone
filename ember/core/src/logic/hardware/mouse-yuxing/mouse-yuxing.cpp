/*
 * ------------------------------------------------------------------------
 * 名称：mouse-yuxing.cpp
 * 说明：裕兴鼠标的实现：数据包的编码与逐位移出。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * ------------------------------------------------------------------------
 */

#include "logic/hardware/mouse-yuxing/mouse-yuxing.hpp"

#include <algorithm>

namespace ember::logic::hardware {

namespace {

/** 写 $4016 的位：位 0 是时钟，位 2 为 1 时不移位。 */
constexpr std::uint8_t WRITE_CLOCK = 0x01;
constexpr std::uint8_t WRITE_HOLD = 0x04;
/** 数据包第 1 个字节的固定位与按键位。 */
constexpr std::uint8_t PACKET_BASE = 0x40;
constexpr std::uint8_t PACKET_LEFT = 0x20;
constexpr std::uint8_t PACKET_RIGHT = 0x10;
/** 位移的范围（8 位有符号数）。 */
constexpr int DELTA_MAX = 127;
constexpr int DELTA_MIN = -128;
/** 一个字节里的数据位数。 */
constexpr unsigned DATA_BITS = 7;

}  // namespace

void MouseYuxing::reset() {
  count_ = 0;
  position_ = 0;
  clock_ = false;
  pendingX_ = 0;
  pendingY_ = 0;
  frameX_ = 0;
  frameY_ = 0;
  buttons_ = 0;
  frameButtons_ = 0;
}

void MouseYuxing::write(std::uint8_t value) {
  const bool clockHigh = (value & WRITE_CLOCK) != 0;
  if ((value & WRITE_HOLD) == 0 && !clock_ && clockHigh) {
    if (position_ >= count_) loadPacket();
    else ++position_;
  }
  clock_ = clockHigh;
}

std::uint8_t MouseYuxing::read() const {
  if (!clock_ || position_ >= count_) return 0;
  return bits_[position_] ? 0 : 1;
}

void MouseYuxing::move(int dx, int dy, std::uint8_t buttons) {
  pendingX_ += dx;
  pendingY_ += dy;
  buttons_ = buttons;
}

void MouseYuxing::syncFrame() {
  frameX_ = std::clamp(pendingX_, DELTA_MIN, DELTA_MAX);
  frameY_ = std::clamp(pendingY_, DELTA_MIN, DELTA_MAX);
  frameButtons_ = buttons_;
  pendingX_ = 0;
  pendingY_ = 0;
}

void MouseYuxing::loadPacket() {
  std::uint8_t first = PACKET_BASE;
  if ((frameButtons_ & BUTTON_LEFT) != 0) first = static_cast<std::uint8_t>(first | PACKET_LEFT);
  if ((frameButtons_ & BUTTON_RIGHT) != 0) first = static_cast<std::uint8_t>(first | PACKET_RIGHT);
  first = static_cast<std::uint8_t>(first | ((frameX_ >> 4) & 0x0C) | ((frameY_ >> 6) & 0x03));
  unsigned index = 0;
  appendByte(first, index);
  appendByte(static_cast<std::uint8_t>(frameY_ & 0x3F), index);
  appendByte(static_cast<std::uint8_t>(frameX_ & 0x3F), index);
  count_ = index;
  position_ = 0;
}

void MouseYuxing::appendByte(std::uint8_t data, unsigned& index) {
  bits_[index++] = false;
  for (unsigned bit = 0; bit < DATA_BITS; ++bit) {
    bits_[index++] = (data & 0x40U) != 0;
    data = static_cast<std::uint8_t>(data << 1);
  }
  bits_[index++] = true;
}

}  // namespace ember::logic::hardware
