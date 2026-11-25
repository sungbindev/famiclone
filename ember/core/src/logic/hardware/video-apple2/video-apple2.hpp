/*
 * ------------------------------------------------------------------------
 * 名称：video-apple2.hpp
 * 说明：苹果 II 类视频生成器：40 列文本、低分辨率图形（40×48）、高分辨率图形（280×192）、
 *       混合模式（下面 4 行文本）与第 2 页，每帧结束时把当前画面画成 560×384 的帧缓冲。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-25
 * 备注：画面按帧结算：每过一帧的时间（调度器事件）读一次显示开关与主存，画出整帧，
 *       不模拟逐行扫描中途改变开关的效果；
 *       文本：每个字符 7×8 个点（见 apple2-font.hpp），字节的高两位决定显示方式：$00-$3F 反相、
 *       $40-$7F 闪烁（约每 16 帧翻转一次）、$80-$FF 正常，
 *       $E0-$FF 显示小写字母与符号，其余显示大写字母与符号；
 *       高分辨率：每字节 7 个点（低位在左），位 7 选色组：独立的点按列号奇偶与色组显示为紫、绿、
 *       蓝、橙，相邻的点显示为白；不允许彩色时低分辨率显示成灰度、高分辨率只有黑白；
 *       帧缓冲的每个苹果点占 2×2 个像素，每像素 4 字节（R、G、B、A）；模块不读总线，
 *       主存与显示开关经 Apple2VideoSource 取得。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "contracts/hardware/ppu.hpp"
#include "contracts/hardware/scheduler.hpp"

namespace ember::logic::hardware {

/** 显示开关。 */
struct Apple2VideoMode {
  /** 文本模式（为假时是图形）。 */
  bool text{true};
  /** 混合模式：图形下面 4 行显示文本。 */
  bool mixed{false};
  /** 显示第 2 页。 */
  bool page2{false};
  /** 高分辨率（为假时是低分辨率）。 */
  bool hires{false};
  /** 允许彩色；为假时图形按亮度显示成灰度（高分辨率只有黑与白），文本不受影响。 */
  bool colorEnabled{true};
};

/** 视频生成器读取显示开关与主存的来源。 */
class Apple2VideoSource {
 public:
  virtual ~Apple2VideoSource() = default;

  /** 当前的显示开关。 */
  [[nodiscard]] virtual Apple2VideoMode videoMode() const = 0;

  /** 视频电路读主存的一个字节（只读主存，不经 ROM 与 I/O，没有副作用）。 */
  [[nodiscard]] virtual std::uint8_t readVideoByte(std::uint16_t address) const = 0;
};

/** 苹果 II 类视频生成器。 */
class VideoApple2 final : public contracts::hardware::EventListener {
 public:
  /** 帧缓冲的宽与高（像素）。 */
  static constexpr std::uint32_t FRAME_WIDTH = 560;
  static constexpr std::uint32_t FRAME_HEIGHT = 384;
  /** 每帧的扫描线数与每行的主时钟周期数（用来换算当前的扫描线）。 */
  static constexpr int SCANLINES_PER_FRAME = 262;

  /**
   * @param scheduler 调度器：每帧结束时的事件登记在它上面。
   * @param source 主存与显示开关的来源，必须活到本对象销毁。
   * @param lineTicks 一条扫描线占的主时钟周期数。
   */
  VideoApple2(contracts::hardware::Scheduler& scheduler, const Apple2VideoSource& source, contracts::hardware::MasterTime lineTicks);

  VideoApple2(const VideoApple2&) = delete;
  VideoApple2& operator=(const VideoApple2&) = delete;

  /** 上电或复位：丢弃旧事件，从现在起重新计帧；上电时清掉已画出的画面。 */
  void reset(bool power);

  /** 停止出帧（断电时）。 */
  void halt();

  /** 最近一次完成的画面；还没有完成过时宽高为 0。 */
  [[nodiscard]] contracts::hardware::FrameBuffer frame() const;

  /** 已完成的帧数。 */
  [[nodiscard]] std::uint64_t frameCount() const { return frameCount_; }

  /** 当前的扫描线（0 到 261），调试用。 */
  [[nodiscard]] int scanline() const;

  /** 低分辨率图形的 16 种颜色，每项内存顺序 R、G、B、A。 */
  [[nodiscard]] static std::vector<std::uint32_t> loresColors();

  /** 按当前的显示开关与主存立即画一帧到帧缓冲（不计帧），测试与调试用。 */
  void renderNow();

  // EventListener
  void onEvent(contracts::hardware::MasterTime when, std::uint32_t tag) override;

 private:
  void renderText(int firstRow, int lastRow, std::uint16_t pageBase);
  void renderLores(int lastRow, std::uint16_t pageBase);
  void renderHires(int lastLine, std::uint16_t pageBase);
  /** 在帧缓冲里画一个苹果点（2×2 像素）。 */
  void putDot(int x, int y, std::uint32_t color);

  contracts::hardware::Scheduler& scheduler_;
  const Apple2VideoSource& source_;
  contracts::hardware::MasterTime lineTicks_;
  contracts::hardware::MasterTime frameTicks_;
  contracts::hardware::EventId event_{contracts::hardware::NO_EVENT};
  contracts::hardware::MasterTime frameStart_{0};
  /** 本帧是否允许彩色（renderNow 从显示开关读来）。 */
  bool colorEnabled_{true};
  std::uint64_t frameCount_{0};
  std::vector<std::uint32_t> pixels_;
};

}  // namespace ember::logic::hardware
