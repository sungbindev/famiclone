/*
 * ------------------------------------------------------------------------
 * 名称：ntsc-palette.hpp
 * 说明：2C02 调色板的 RGB 表：按 NTSC 复合视频的信号电平与解码算法生成，含三个颜色强调位的 8 种组合。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：不使用任何现成的调色板文件；信号电平、相位与强调衰减取自 NESdev 维基“NTSC video”页公布的实测数据与解码方法，
 *       解码代码为本项目自行编写。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <array>
#include <cstdint>

namespace ember::logic::hardware {

/**
 * 生成调色板的 RGBA 表。
 * @return 512 项：下标为 强调位（$2001 的位 7-5，0 到 7）* 64 + 颜色号（0 到 63）；
 *         每项为内存中依次 R、G、B、A 的 32 位整数（小端机器上为 0xAABBGGRR），A 为 255。
 */
[[nodiscard]] std::array<std::uint32_t, 512> buildNtscPalette();

}  // namespace ember::logic::hardware
