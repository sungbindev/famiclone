/*
 * ------------------------------------------------------------------------
 * 名称：sb-ppu-oam-put.c
 * 说明：sb_ppu_oam_put 的实现：写一个精灵。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：一个精灵 4 字节：Y、图块号、属性、X。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_ppu_oam_put(unsigned char index, unsigned char y, unsigned char tile, unsigned char attr, unsigned char x)
{
    SB_BYTE_AT(SB_HW_OAM_ADDRESS) = (unsigned char)(index << 2);
    SB_BYTE_AT(SB_HW_OAM_DATA) = y;
    SB_BYTE_AT(SB_HW_OAM_DATA) = tile;
    SB_BYTE_AT(SB_HW_OAM_DATA) = attr;
    SB_BYTE_AT(SB_HW_OAM_DATA) = x;
}
