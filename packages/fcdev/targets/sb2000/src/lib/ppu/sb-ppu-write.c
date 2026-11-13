/*
 * ------------------------------------------------------------------------
 * 名称：sb-ppu-write.c
 * 说明：sb_ppu_write 的实现：把一段数据写进显存。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：要求显示已关；地址设一次，之后靠 $2007 的自动加 1。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_ppu_write(unsigned int address, const unsigned char *data, unsigned int count)
{
    unsigned int i;

    sb_ppu_address(address);
    for (i = 0; i < count; ++i) {
        SB_BYTE_AT(SB_HW_PPU_DATA) = data[i];
    }
}
