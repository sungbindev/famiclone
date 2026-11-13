/*
 * ------------------------------------------------------------------------
 * 名称：sb-ppu-peek.c
 * 说明：sb_ppu_peek 的实现：读显存的一个字节。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：真机与模拟器上都是：设地址后第一次读只刷新缓冲，重设地址后第二次读才是内容。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_ppu_peek(unsigned int address)
{
    unsigned char sink;

    sb_ppu_address(address);
    sink = SB_BYTE_AT(SB_HW_PPU_DATA);
    (void)sink;
    sb_ppu_address(address);
    return SB_BYTE_AT(SB_HW_PPU_DATA);
}
