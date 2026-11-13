/*
 * ------------------------------------------------------------------------
 * 名称：ppu-2c02.hpp
 * 说明：经典 FC 的 PPU（2C02，NTSC）：寄存器、loopy 滚动模型、按点（dot）精度的背景与精灵渲染、vblank 与 NMI、帧缓冲输出。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：PPU 不逐周期被调用：CPU 访问寄存器前、自己的调度事件到期时，才把点时钟追赶到当前时间；
 *       点 k 在主时钟 phase + 4k 时执行，访问发生在同一时刻或更晚时，就能看到该点的结果；
 *       调度事件只在会改变 NMI 线的点（241 行第 1 点、预渲染行第 1 点与第 339 点）上登记，保证 CPU 逐周期采样 NMI 时不会漏掉变化；
 *       经 PpuBus 访问图案表与名称表，调色板在 PPU 内；不模拟开放总线的衰减与上电后的预热期。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <vector>

#include "contracts/hardware/clock.hpp"
#include "contracts/hardware/irq-line.hpp"
#include "contracts/hardware/ppu-bus.hpp"
#include "contracts/hardware/ppu.hpp"
#include "contracts/hardware/scheduler.hpp"

namespace ember::logic::hardware {

/** 经典 FC 的 PPU（2C02，NTSC）。 */
class Ppu2c02 final : public contracts::hardware::Ppu, public contracts::hardware::EventListener {
 public:
  /** 画面宽度（像素）。 */
  static constexpr std::uint32_t FRAME_WIDTH = 256;
  /** 画面高度（像素）。 */
  static constexpr std::uint32_t FRAME_HEIGHT = 240;
  /** 每行的点数。 */
  static constexpr int DOTS_PER_SCANLINE = 341;
  /** 每帧的行数（NTSC）。 */
  static constexpr int SCANLINES_PER_FRAME = 262;
  /** 预渲染行的行号。 */
  static constexpr int PRE_RENDER_LINE = 261;

  /**
   * @param scheduler 调度器，既是主时钟也登记 NMI 相关的事件，必须活到本对象销毁。
   * @param bus PPU 总线：图案表与名称表的访问经它完成。
   * @param nmi NMI 线，本对象在其上登记一个来源。
   * @param domain PPU 的时钟域，每个点占的主时钟周期数必须大于 0（NTSC 为 4）。
   * @param phase 上电或复位后第 0 点相对当前时间的延迟（主时钟周期数），不能为负；决定 CPU 与 PPU 的相位关系。
   */
  Ppu2c02(contracts::hardware::Scheduler& scheduler, contracts::hardware::PpuBus& bus, contracts::hardware::IrqLine& nmi, contracts::hardware::ClockDomain domain, contracts::hardware::MasterTime phase);

  // BusDevice：$2000-$3FFF，地址每 8 个字节镜像一次
  contracts::hardware::BusRead read(std::uint16_t address) override;
  void write(std::uint16_t address, std::uint8_t value) override;

  // Ppu
  void reset(bool power) override;
  void catchUp(contracts::hardware::MasterTime now) override;
  [[nodiscard]] contracts::hardware::FrameBuffer frame() const override;
  [[nodiscard]] std::uint64_t frameCount() const override { return frameCount_; }

  // EventListener
  void onEvent(contracts::hardware::MasterTime when, std::uint32_t tag) override;

  // 测试与调试用的只读访问（不追赶时间，不产生副作用）
  [[nodiscard]] int scanline() const { return scanline_; }
  [[nodiscard]] int dot() const { return dot_; }
  [[nodiscard]] std::uint16_t vramAddress() const { return v_; }
  [[nodiscard]] std::uint16_t tempAddress() const { return t_; }
  [[nodiscard]] std::uint8_t fineX() const { return fineX_; }
  [[nodiscard]] bool writeToggle() const { return w_; }
  [[nodiscard]] bool vblankFlag() const { return vblank_; }
  [[nodiscard]] bool spriteZeroHit() const { return sprite0Hit_; }
  [[nodiscard]] bool spriteOverflow() const { return spriteOverflow_; }
  [[nodiscard]] std::span<const std::uint8_t> oam() const { return oam_; }
  [[nodiscard]] std::span<const std::uint8_t> secondaryOam() const { return secondary_; }
  [[nodiscard]] std::span<const std::uint8_t> palette() const { return palette_; }
  /** 当前每行的精灵数（下一行要画的精灵）。 */
  [[nodiscard]] int lineSpriteCount() const { return spriteCount_; }

 private:
  // 寄存器与渲染的内部步骤见 ppu-2c02.cpp（寄存器、时间）与 ppu-2c02-render.cpp（渲染）
  std::uint8_t readStatus();
  std::uint8_t readOamData();
  std::uint8_t readData();
  void writeScroll(std::uint8_t value);
  void writeAddress(std::uint8_t value);
  void writeOamData(std::uint8_t value);
  void writeData(std::uint8_t value);
  void incrementVramAddress();
  void updateNmi();
  void scheduleNextEvent();
  void runDot();
  void publishFrame();

  [[nodiscard]] bool renderingEnabled() const { return (mask_ & 0x18) != 0; }
  [[nodiscard]] static std::uint8_t paletteIndex(std::uint16_t address);

  // 渲染
  void shiftBackground();
  void fetchBackground(int dot);
  void incrementCoarseX();
  void incrementY();
  void loadBackgroundShifters();
  void evaluateSprites(int line, int dot);
  void fetchSprites(int line, int dot);
  void drawPixel(int line, int x);
  [[nodiscard]] std::uint8_t readVram(std::uint16_t address);

  contracts::hardware::Scheduler& scheduler_;
  contracts::hardware::PpuBus& bus_;
  contracts::hardware::IrqLine& nmi_;
  contracts::hardware::SignalSource nmiSource_;
  contracts::hardware::ClockDomain domain_;
  contracts::hardware::MasterTime phase_;
  contracts::hardware::EventId eventId_{contracts::hardware::NO_EVENT};

  // 时间：下一个要执行的点与它开始的主时钟时间
  contracts::hardware::MasterTime nextDotTime_{0};
  int scanline_{0};
  int dot_{0};
  bool oddFrame_{false};
  /** 上一个点执行完时背景是否开启。 */
  bool backgroundWasEnabled_{false};
  std::uint64_t frameCount_{0};

  // 寄存器
  std::uint8_t ctrl_{0};
  std::uint8_t mask_{0};
  bool vblank_{false};
  bool sprite0Hit_{false};
  bool spriteOverflow_{false};
  /** 读 $2002 发生在 vblank 置位前一个点：本帧不置位。 */
  bool suppressVbl_{false};
  std::uint8_t oamAddr_{0};
  std::uint8_t readBuffer_{0};
  /** PPU 数据总线上最近一次的值，读只写寄存器与 $2002 低 5 位时返回。 */
  std::uint8_t openBus_{0};

  // loopy 滚动模型
  std::uint16_t v_{0};
  std::uint16_t t_{0};
  std::uint8_t fineX_{0};
  bool w_{false};

  // 存储
  std::array<std::uint8_t, 256> oam_{};
  std::array<std::uint8_t, 32> secondary_{};
  std::array<std::uint8_t, 32> palette_{};

  // 背景流水线
  std::uint8_t nextTileId_{0};
  std::uint8_t nextAttribute_{0};
  std::uint8_t nextPatternLow_{0};
  std::uint8_t nextPatternHigh_{0};
  std::uint16_t shiftPatternLow_{0};
  std::uint16_t shiftPatternHigh_{0};
  std::uint16_t shiftAttributeLow_{0};
  std::uint16_t shiftAttributeHigh_{0};

  // 精灵评估：n 是精灵号，m 是精灵内的字节号
  std::uint8_t oamBuffer_{0};
  std::uint8_t secondaryAddress_{0};
  std::uint8_t spriteN_{0};
  std::uint8_t spriteM_{0};
  bool spriteInRange_{false};
  bool evaluationDone_{false};
  bool sprite0Added_{false};
  int overflowCounter_{0};
  int nextSpriteCount_{0};
  bool nextSprite0_{false};

  // 当前行要画的精灵（在上一行的第 257-320 点取好）
  struct SpriteUnit {
    std::uint8_t patternLow{0};
    std::uint8_t patternHigh{0};
    std::uint8_t attribute{0};
    std::uint8_t x{0};
  };
  std::array<SpriteUnit, 8> sprites_{};
  int spriteCount_{0};
  bool sprite0InLine_{false};

  // 输出：两块帧缓冲轮流绘制与显示
  std::array<std::vector<std::uint32_t>, 2> frameBuffers_;
  int drawIndex_{0};
  std::array<std::uint32_t, 512> rgbaTable_{};
};

}  // namespace ember::logic::hardware
