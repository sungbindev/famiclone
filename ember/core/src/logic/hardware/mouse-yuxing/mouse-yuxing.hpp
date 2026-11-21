/*
 * ------------------------------------------------------------------------
 * 名称：mouse-yuxing.hpp
 * 说明：裕兴学习机的鼠标：经 $4016、$4017 的数据位 0 串行输出 3 个字节的数据包，由写 $4016 的位 0 上升沿一位一位移出。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：接口只有两个动作：写 $4016（write）与读数据位（read，$4016 与 $4017 的位 0 相同）；
 *       移位：写入的位 2 为 0、位 0 由 0 变 1（上升沿）时，如果移位寄存器是空的，就装入一个新的数据包（这一次不移出），否则移出一位；
 *       数据包：3 个字节，每个字节 9 位——起始位 0、7 个数据位（高位在前）、停止位 1；
 *       第 1 个字节 = $40 | 左键 $20 | 右键 $10 | 横向位移的位 7、6（放在位 3、2）| 纵向位移的位 7、6（放在位 1、0），第 2 个字节是纵向位移的低 6 位，第 3 个字节是横向位移的低 6 位；
 *       读：上次写入的位 0 为 1 且移位寄存器不空时，给出当前位的反相，否则为 0；
 *       位移的单位是屏幕像素；移动事件的位移与按键先累加，每帧开始（syncFrame）时锁存成这一帧的位移与按键（位移限在 -128 到 127），同一帧里装的每个数据包都带这一帧的值，不因装包而清零。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <array>
#include <cstdint>

namespace ember::logic::hardware {

/** 裕兴鼠标。 */
class MouseYuxing final {
 public:
  /** 按键位：左键、右键。 */
  static constexpr std::uint8_t BUTTON_LEFT = 0x01;
  static constexpr std::uint8_t BUTTON_RIGHT = 0x02;
  /** 一个数据包的位数。 */
  static constexpr unsigned PACKET_BITS = 27;

  /** 清空移位寄存器、位移与按键（上电与复位时）。 */
  void reset();

  /** 写 $4016。 */
  void write(std::uint8_t value);

  /** 读数据位（0 或 1），对应读 $4016 与 $4017 的位 0。 */
  [[nodiscard]] std::uint8_t read() const;

  /**
   * 鼠标移动或按键变化。
   * @param dx 横向位移（屏幕像素，向右为正），累加到下一帧。
   * @param dy 纵向位移（向下为正），同上。
   * @param buttons 当前按住的按键位（BUTTON_*）。
   */
  void move(int dx, int dy, std::uint8_t buttons);

  /** 每帧开始时调用：把累加的位移与当前的按键锁存成这一帧的值，累加清零。 */
  void syncFrame();

  /** 移位寄存器里还剩的位数，供测试。 */
  [[nodiscard]] unsigned bitsLeft() const { return count_ - position_; }

 private:
  /** 装入一个新的数据包。 */
  void loadPacket();
  /** 追加一个字节（起始位、7 个数据位、停止位）。 */
  void appendByte(std::uint8_t data, unsigned& index);

  std::array<bool, PACKET_BITS> bits_{};
  unsigned count_{0};
  unsigned position_{0};
  bool clock_{false};
  /** 累加中的位移（下一帧用）。 */
  int pendingX_{0};
  int pendingY_{0};
  /** 这一帧锁存的位移与按键。 */
  int frameX_{0};
  int frameY_{0};
  std::uint8_t frameButtons_{0};
  /** 最近一次事件里按住的按键。 */
  std::uint8_t buttons_{0};
};

}  // namespace ember::logic::hardware
