/*
 * ------------------------------------------------------------------------
 * 名称：sb-ppu-palette-set.c
 * 说明：sb_ppu_palette_set 的实现：写调色板的一项。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：调色板在 $2040 起，共 64 项。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_ppu_palette_set(unsigned char index, unsigned char color)
{
    SB_BYTE_AT(SB_HW_PALETTE + index) = color;
}
