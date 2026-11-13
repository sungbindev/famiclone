/*
 * ------------------------------------------------------------------------
 * 名称：sb-ppu-read.c
 * 说明：sb_ppu_read 的实现：读一段显存。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：逐字节用 sb_ppu_peek 读。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_ppu_read(unsigned int address, unsigned char *dest, unsigned int count)
{
    unsigned int i;

    for (i = 0; i < count; ++i) {
        dest[i] = sb_ppu_peek(address + i);
    }
}
