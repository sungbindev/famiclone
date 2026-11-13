/*
 * ------------------------------------------------------------------------
 * 名称：button-input.hpp
 * 说明：硬件契约：手柄按键的输入接口。外部（界面、测试、调试服务）经它设置标准手柄当前按下的按键。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：按键用一个 8 位掩码表示，位序与标准手柄串行输出的顺序一致：A 最先、右最后。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>

namespace ember::contracts::hardware {

/** 标准手柄的按键掩码位。 */
namespace pad_button {
inline constexpr std::uint8_t A = 0x01;
inline constexpr std::uint8_t B = 0x02;
inline constexpr std::uint8_t SELECT = 0x04;
inline constexpr std::uint8_t START = 0x08;
inline constexpr std::uint8_t UP = 0x10;
inline constexpr std::uint8_t DOWN = 0x20;
inline constexpr std::uint8_t LEFT = 0x40;
inline constexpr std::uint8_t RIGHT = 0x80;
}  // namespace pad_button

/** 按键输入：设置手柄当前按下的按键。 */
class ButtonInput {
 public:
  virtual ~ButtonInput() = default;

  /**
   * 设置当前按下的按键。
   * @param buttons 按键掩码，位见 pad_button；1 表示按下。上下、左右同时按下不做限制，由游戏自己处理。
   */
  virtual void setButtons(std::uint8_t buttons) = 0;
};

}  // namespace ember::contracts::hardware
