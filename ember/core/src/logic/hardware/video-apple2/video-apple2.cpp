/*
 * ------------------------------------------------------------------------
 * 名称：video-apple2.cpp
 * 说明：苹果 II 类视频生成器的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-25
 * 备注：无
 * ------------------------------------------------------------------------
 */

#include "logic/hardware/video-apple2/video-apple2.hpp"

#include <algorithm>
#include <array>

#include "logic/hardware/video-apple2/apple2-font.hpp"

namespace ember::logic::hardware {

namespace {

namespace hw = contracts::hardware;

/** 文本屏每行的字符数、行数，字符的点宽与行高。 */
constexpr int TEXT_COLUMNS = 40;
constexpr int TEXT_ROWS = 24;
constexpr int GLYPH_WIDTH = 7;
constexpr int GLYPH_HEIGHT = 8;
/** 苹果点的宽与高。 */
constexpr int DOT_COLUMNS = 280;
constexpr int DOT_LINES = 192;
/** 混合模式下文本占的起始行。 */
constexpr int MIXED_FIRST_TEXT_ROW = 20;
/** 闪烁字符每隔多少帧翻转一次。 */
constexpr std::uint64_t FLASH_FRAMES = 16;

constexpr std::uint16_t TEXT_PAGE_1 = 0x0400;
constexpr std::uint16_t TEXT_PAGE_2 = 0x0800;
constexpr std::uint16_t HIRES_PAGE_1 = 0x2000;
constexpr std::uint16_t HIRES_PAGE_2 = 0x4000;

constexpr std::uint32_t rgb(std::uint32_t red, std::uint32_t green, std::uint32_t blue) { return 0xFF000000U | (blue << 16) | (green << 8) | red; }

constexpr std::uint32_t BLACK = rgb(0, 0, 0);
constexpr std::uint32_t WHITE = rgb(255, 255, 255);

/** 低分辨率的 16 种颜色。 */
constexpr std::array<std::uint32_t, 16> LORES_COLORS = {rgb(0, 0, 0),       rgb(227, 30, 96),  rgb(96, 78, 189),   rgb(255, 68, 253), rgb(0, 163, 96),    rgb(156, 156, 156), rgb(20, 207, 253), rgb(208, 195, 255),
                                                        rgb(96, 114, 3),    rgb(255, 106, 60), rgb(156, 156, 156), rgb(255, 160, 208), rgb(20, 245, 60), rgb(208, 221, 141), rgb(114, 255, 208), rgb(255, 255, 255)};

/** 高分辨率：色组 0（位 7 为 0）与色组 1 里，偶数列与奇数列的颜色。 */
constexpr std::uint32_t HIRES_VIOLET = rgb(255, 68, 253);
constexpr std::uint32_t HIRES_GREEN = rgb(20, 245, 60);
constexpr std::uint32_t HIRES_BLUE = rgb(20, 207, 253);
constexpr std::uint32_t HIRES_ORANGE = rgb(255, 106, 60);

/** 文本第 row 行（0 到 23）的起始地址偏移：每 8 行一组，组内每行间隔 $80，每组间隔 $28。 */
constexpr std::uint16_t textRowOffset(int row) { return static_cast<std::uint16_t>((row & 7) * 0x80 + (row >> 3) * 0x28); }

/** 高分辨率第 line 行（0 到 191）的起始地址偏移。 */
constexpr std::uint16_t hiresLineOffset(int line) { return static_cast<std::uint16_t>((line & 7) * 0x400 + ((line >> 3) & 7) * 0x80 + (line >> 6) * 0x28); }

}  // namespace

VideoApple2::VideoApple2(hw::Scheduler& scheduler, const Apple2VideoSource& source, hw::MasterTime lineTicks)
    : scheduler_(scheduler), source_(source), lineTicks_(lineTicks), frameTicks_(lineTicks * SCANLINES_PER_FRAME), pixels_(static_cast<std::size_t>(FRAME_WIDTH) * FRAME_HEIGHT, BLACK) {}

void VideoApple2::reset(bool power) {
  if (event_ != hw::NO_EVENT) scheduler_.cancel(event_);
  if (power) {
    std::fill(pixels_.begin(), pixels_.end(), BLACK);
    frameCount_ = 0;
  }
  frameStart_ = scheduler_.now();
  event_ = scheduler_.scheduleAt(frameStart_ + frameTicks_, *this, 0);
}

void VideoApple2::halt() {
  if (event_ != hw::NO_EVENT) scheduler_.cancel(event_);
  event_ = hw::NO_EVENT;
}

hw::FrameBuffer VideoApple2::frame() const {
  if (frameCount_ == 0) return hw::FrameBuffer{};
  return hw::FrameBuffer{FRAME_WIDTH, FRAME_HEIGHT, std::span<const std::uint32_t>(pixels_)};
}

int VideoApple2::scanline() const {
  const hw::MasterTime elapsed = std::max<hw::MasterTime>(0, scheduler_.now() - frameStart_);
  return std::min<int>(SCANLINES_PER_FRAME - 1, static_cast<int>(elapsed / lineTicks_));
}

std::vector<std::uint32_t> VideoApple2::loresColors() { return std::vector<std::uint32_t>(LORES_COLORS.begin(), LORES_COLORS.end()); }

void VideoApple2::onEvent(hw::MasterTime when, std::uint32_t /*tag*/) {
  renderNow();
  ++frameCount_;
  frameStart_ = when;
  event_ = scheduler_.scheduleAt(when + frameTicks_, *this, 0);
}

void VideoApple2::renderNow() {
  const Apple2VideoMode mode = source_.videoMode();
  const std::uint16_t textPage = mode.page2 ? TEXT_PAGE_2 : TEXT_PAGE_1;
  if (mode.text) {
    renderText(0, TEXT_ROWS - 1, textPage);
    return;
  }
  if (mode.hires) renderHires(mode.mixed ? MIXED_FIRST_TEXT_ROW * GLYPH_HEIGHT - 1 : DOT_LINES - 1, mode.page2 ? HIRES_PAGE_2 : HIRES_PAGE_1);
  else renderLores(mode.mixed ? MIXED_FIRST_TEXT_ROW - 1 : TEXT_ROWS - 1, textPage);
  if (mode.mixed) renderText(MIXED_FIRST_TEXT_ROW, TEXT_ROWS - 1, textPage);
}

void VideoApple2::putDot(int x, int y, std::uint32_t color) {
  const std::size_t index = static_cast<std::size_t>(y) * 2 * FRAME_WIDTH + static_cast<std::size_t>(x) * 2;
  pixels_[index] = color;
  pixels_[index + 1] = color;
  pixels_[index + FRAME_WIDTH] = color;
  pixels_[index + FRAME_WIDTH + 1] = color;
}

void VideoApple2::renderText(int firstRow, int lastRow, std::uint16_t pageBase) {
  const bool flashOff = ((frameCount_ / FLASH_FRAMES) & 1U) != 0;
  for (int row = firstRow; row <= lastRow; ++row) {
    for (int column = 0; column < TEXT_COLUMNS; ++column) {
      const std::uint8_t value = source_.readVideoByte(static_cast<std::uint16_t>(pageBase + textRowOffset(row) + column));
      // 字符码：$E0 以上显示小写字母与符号，其余取低 6 位显示成 $20-$5F 的字符。
      std::uint8_t ascii = 0;
      if (value >= 0xE0) {
        ascii = value & 0x7F;
      } else {
        const std::uint8_t index = value & 0x3F;
        ascii = index < 0x20 ? static_cast<std::uint8_t>(index + 0x40) : index;
      }
      const bool inverted = value < 0x40 || (value < 0x80 && flashOff);
      const std::size_t glyph = static_cast<std::size_t>(std::clamp<int>(ascii, 0x20, 0x7F) - 0x20) * apple2::FONT_GLYPH_HEIGHT;
      for (int line = 0; line < GLYPH_HEIGHT; ++line) {
        const std::uint8_t bits = apple2::TEXT_FONT[glyph + static_cast<std::size_t>(line)];
        for (int dot = 0; dot < GLYPH_WIDTH; ++dot) {
          const bool lit = (((bits >> dot) & 1U) != 0) != inverted;
          putDot(column * GLYPH_WIDTH + dot, row * GLYPH_HEIGHT + line, lit ? WHITE : BLACK);
        }
      }
    }
  }
}

void VideoApple2::renderLores(int lastRow, std::uint16_t pageBase) {
  for (int row = 0; row <= lastRow; ++row) {
    for (int column = 0; column < TEXT_COLUMNS; ++column) {
      const std::uint8_t value = source_.readVideoByte(static_cast<std::uint16_t>(pageBase + textRowOffset(row) + column));
      const std::uint32_t upper = LORES_COLORS[value & 0x0F];
      const std::uint32_t lower = LORES_COLORS[value >> 4];
      for (int line = 0; line < GLYPH_HEIGHT; ++line) {
        const std::uint32_t color = line < 4 ? upper : lower;
        for (int dot = 0; dot < GLYPH_WIDTH; ++dot) putDot(column * GLYPH_WIDTH + dot, row * GLYPH_HEIGHT + line, color);
      }
    }
  }
}

void VideoApple2::renderHires(int lastLine, std::uint16_t pageBase) {
  // 每行 280 个点，两侧各留一个熄灭的点，方便查相邻的点。
  std::array<bool, DOT_COLUMNS + 2> lit{};
  std::array<bool, TEXT_COLUMNS> shifted{};
  for (int line = 0; line <= lastLine; ++line) {
    const std::uint16_t base = static_cast<std::uint16_t>(pageBase + hiresLineOffset(line));
    lit.fill(false);
    for (int column = 0; column < TEXT_COLUMNS; ++column) {
      const std::uint8_t value = source_.readVideoByte(static_cast<std::uint16_t>(base + column));
      shifted[static_cast<std::size_t>(column)] = (value & 0x80) != 0;
      for (int dot = 0; dot < GLYPH_WIDTH; ++dot) lit[static_cast<std::size_t>(column * GLYPH_WIDTH + dot + 1)] = ((value >> dot) & 1U) != 0;
    }
    for (int x = 0; x < DOT_COLUMNS; ++x) {
      const std::size_t index = static_cast<std::size_t>(x + 1);
      std::uint32_t color = BLACK;
      if (lit[index]) {
        if (lit[index - 1] || lit[index + 1]) {
          color = WHITE;
        } else {
          const bool group = shifted[static_cast<std::size_t>(x / GLYPH_WIDTH)];
          const bool odd = (x & 1) != 0;
          color = group ? (odd ? HIRES_ORANGE : HIRES_BLUE) : (odd ? HIRES_GREEN : HIRES_VIOLET);
        }
      }
      putDot(x, line, color);
    }
  }
}

}  // namespace ember::logic::hardware
