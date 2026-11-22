/*
 * ------------------------------------------------------------------------
 * 名称：sb-vram-fill.c
 * 说明：sb_vram_fill 的实现：填充显存。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $32：显存地址 $0646（高）、$0647（低），长度 $0650（高）、$064F（低），填充值 $064C。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_vram_fill(unsigned int address, unsigned int length, unsigned char value)
{
    SB_BYTE_AT(SB_PARAM_0646) = (unsigned char)(address >> 8);
    SB_BYTE_AT(SB_PARAM_0647) = (unsigned char)address;
    SB_BYTE_AT(SB_PARAM_064F) = (unsigned char)length;
    SB_BYTE_AT(SB_PARAM_0650) = (unsigned char)(length >> 8);
    SB_BYTE_AT(SB_PARAM_064C) = value;
    sb_syscall(SB_FN_VRAM_FILL, 0);
}
