/*
 * ------------------------------------------------------------------------
 * 名称：sb-ppu-address.c
 * 说明：sb_ppu_address 的实现：设置显存地址。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：先读一次 $2002 复位地址锁存，再依次写高、低字节。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_ppu_address(unsigned int address)
{
    unsigned char sink = SB_BYTE_AT(SB_HW_PPU_STATUS);

    (void)sink;
    SB_BYTE_AT(SB_HW_PPU_ADDRESS) = (unsigned char)(address >> 8);
    SB_BYTE_AT(SB_HW_PPU_ADDRESS) = (unsigned char)address;
}
