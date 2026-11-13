/*
 * ------------------------------------------------------------------------
 * 名称：ntsc-palette.cpp
 * 说明：2C02 调色板 RGB 表的生成：模拟 PPU 直接输出的复合视频波形，在一个色彩副载波周期（12 个相位）上解调出 Y、U、V，再转成 RGB。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：来源为 NESdev 维基“NTSC video”页：
 *       - 终端实测电压（毫伏转为伏）：低电平 0.228/0.312/0.552/0.880，高电平 0.616/0.840/1.100/1.100，
 *         颜色强调时的衰减值 0.192/0.256/0.448/0.712 与 0.500/0.676/0.896/0.896；
 *       - 颜色 $x1-$xC 是在高低电平之间交替的方波，$x0 恒为高电平，$xD 恒为低电平，$xE/$xF 与 $1D 同电平；
 *       - 强调位 7、6、5 分别在颜色 8、4、C 的相位上衰减信号，颜色 $xE/$xF 不受影响；
 *       - 黑电平 0.312、白电平 1.100，色度饱和度修正 2*(40/140)/(0.524-0.148)，YUV 转 RGB 的系数 1.139883、0.394642、0.580622、2.032062。
 *       解调用的相位偏移为 2.5：使颜色 $x8 的方波恰好落在 -U 轴上（色同步的相位）。不模拟微分相位失真与色彩伪影。
 * ------------------------------------------------------------------------
 */

#include "logic/hardware/ppu-2c02/ntsc-palette.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace ember::logic::hardware {

namespace {

constexpr float LOW_LEVELS[4] = {0.228F, 0.312F, 0.552F, 0.880F};
constexpr float HIGH_LEVELS[4] = {0.616F, 0.840F, 1.100F, 1.100F};
constexpr float LOW_LEVELS_ATTENUATED[4] = {0.192F, 0.256F, 0.448F, 0.712F};
constexpr float HIGH_LEVELS_ATTENUATED[4] = {0.500F, 0.676F, 0.896F, 0.896F};
constexpr float BLACK_LEVEL = 0.312F;
constexpr float WHITE_LEVEL = 1.100F;
constexpr float SATURATION = 2.0F * (40.0F / 140.0F) / (0.524F - 0.148F);
constexpr float PHASE_OFFSET = 2.5F;
constexpr int PHASES = 12;

/** 颜色号 color 的方波在相位 phase 上是否处于高电平的半周期。 */
bool inColorPhase(int color, int phase) { return (color + phase) % PHASES < PHASES / 2; }

/** 某个颜色在某个相位上的信号电平。 */
float signalLevel(int paletteEntry, int emphasis, int phase) {
  const int color = paletteEntry & 0x0F;
  const int level = color > 13 ? 1 : (paletteEntry >> 4) & 3;
  // 强调位 7、6、5 对应颜色 8、4、C 的相位；$xE/$xF 不受影响。
  const bool attenuated = color < 14 && (((emphasis & 1) != 0 && inColorPhase(0xC, phase)) || ((emphasis & 2) != 0 && inColorPhase(0x4, phase)) || ((emphasis & 4) != 0 && inColorPhase(0x8, phase)));
  float low = attenuated ? LOW_LEVELS_ATTENUATED[level] : LOW_LEVELS[level];
  float high = attenuated ? HIGH_LEVELS_ATTENUATED[level] : HIGH_LEVELS[level];
  if (color == 0) low = high;
  if (color > 12) high = low;
  return inColorPhase(color, phase) ? high : low;
}

/** 把 0 到 1 的通道值量化为 8 位。 */
std::uint32_t quantize(float value) {
  const float clamped = std::clamp(value, 0.0F, 1.0F);
  return static_cast<std::uint32_t>(std::min(255.0F, clamped * 255.95F));
}

}  // namespace

std::array<std::uint32_t, 512> buildNtscPalette() {
  std::array<std::uint32_t, 512> table{};
  for (int emphasis = 0; emphasis < 8; ++emphasis) {
    for (int entry = 0; entry < 64; ++entry) {
      float y = 0.0F;
      float u = 0.0F;
      float v = 0.0F;
      for (int phase = 0; phase < PHASES; ++phase) {
        const float level = (signalLevel(entry, emphasis, phase) - BLACK_LEVEL) / (WHITE_LEVEL - BLACK_LEVEL) / static_cast<float>(PHASES);
        const float angle = std::numbers::pi_v<float> * (static_cast<float>(phase) + PHASE_OFFSET) / 6.0F;
        y += level;
        u += level * std::sin(angle) * SATURATION;
        v += level * std::cos(angle) * SATURATION;
      }
      const std::uint32_t red = quantize(y + 1.139883F * v);
      const std::uint32_t green = quantize(y - 0.394642F * u - 0.580622F * v);
      const std::uint32_t blue = quantize(y + 2.032062F * u);
      table[static_cast<std::size_t>(emphasis * 64 + entry)] = red | (green << 8) | (blue << 16) | 0xFF000000U;
    }
  }
  return table;
}

}  // namespace ember::logic::hardware
