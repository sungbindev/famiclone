/*
 * ------------------------------------------------------------------------
 * 名称：sb-ppu-render.c
 * 说明：sb_ppu_render 的实现：设置控制寄存器并打开显示。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：最后才写 $2001，显示从这里开始。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_ppu_render(unsigned char mask, unsigned char control, unsigned char ext)
{
    SB_BYTE_AT(SB_HW_PPU_CTRL) = control;
    SB_BYTE_AT(SB_HW_PPU_EXT) = ext;
    SB_BYTE_AT(SB_HW_PPU_SCROLL) = 0;
    SB_BYTE_AT(SB_HW_PPU_SCROLL) = 0;
    SB_BYTE_AT(SB_HW_PPU_MASK) = mask;
}
