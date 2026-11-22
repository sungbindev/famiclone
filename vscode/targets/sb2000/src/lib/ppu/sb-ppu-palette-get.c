/*
 * ------------------------------------------------------------------------
 * 名称：sb-ppu-palette-get.c
 * 说明：sb_ppu_palette_get 的实现：读调色板的一项。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：真机读调色板得到“值 & $3F | $40”，所以去掉位 6。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_ppu_palette_get(unsigned char index)
{
    return SB_BYTE_AT(SB_HW_PALETTE + index) & 0x3F;
}
