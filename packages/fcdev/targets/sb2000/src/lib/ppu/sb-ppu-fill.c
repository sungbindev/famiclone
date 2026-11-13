/*
 * ------------------------------------------------------------------------
 * 名称：sb-ppu-fill.c
 * 说明：sb_ppu_fill 的实现：把显存的一段填成同一个值。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：要求显示已关。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_ppu_fill(unsigned int address, unsigned char value, unsigned int count)
{
    sb_ppu_address(address);
    while (count-- != 0) {
        SB_BYTE_AT(SB_HW_PPU_DATA) = value;
    }
}
