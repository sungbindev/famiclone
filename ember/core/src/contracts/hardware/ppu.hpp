/*
 * ------------------------------------------------------------------------
 * 名称：ppu.hpp
 * 说明：硬件契约：PPU。复位、按时间追赶、输出帧缓冲；寄存器读写经 BusDevice 挂在 CPU 总线上。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：PPU 按点时钟（dot）精度运行，但不逐周期被调用：在 CPU 访问它之前、自己的事件到期时追赶到当前时间。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>
#include <span>

#include "contracts/hardware/bus-device.hpp"
#include "contracts/hardware/clock.hpp"

namespace ember::contracts::hardware {

/** 一帧画面：每像素 32 位，格式 0xAARRGGBB，按行从上到下存放。 */
struct FrameBuffer {
  std::uint32_t width{0};
  std::uint32_t height{0};
  std::span<const std::uint32_t> pixels;
};

/** PPU。 */
class Ppu : public BusDevice {
 public:
  /**
   * 复位。
   * @param power 为 true 表示上电复位，为 false 表示软复位。
   */
  virtual void reset(bool power) = 0;

  /**
   * 追赶到指定时间：运行到该时间点之前应完成的全部点时钟。
   * @param now 目标时间（主时钟计数）；不会早于上一次追赶的时间。
   */
  virtual void catchUp(MasterTime now) = 0;

  /** 最近一次完成的一帧画面；视图在下一帧开始绘制前有效。 */
  [[nodiscard]] virtual FrameBuffer frame() const = 0;

  /** 已完成的帧数，调用方据此判断有没有新的一帧。 */
  [[nodiscard]] virtual std::uint64_t frameCount() const = 0;
};

}  // namespace ember::contracts::hardware
