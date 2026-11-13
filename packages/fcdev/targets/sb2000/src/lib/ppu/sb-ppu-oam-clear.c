/*
 * ------------------------------------------------------------------------
 * 名称：sb-ppu-oam-clear.c
 * 说明：sb_ppu_oam_clear 的实现：把精灵 RAM 填成 $F0。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：Y 为 $F0 的精灵在屏幕之外。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_ppu_oam_clear(void)
{
    unsigned int i;

    SB_BYTE_AT(SB_HW_OAM_ADDRESS) = 0;
    for (i = 0; i < 256; ++i) {
        SB_BYTE_AT(SB_HW_OAM_DATA) = 0xF0;
    }
}
