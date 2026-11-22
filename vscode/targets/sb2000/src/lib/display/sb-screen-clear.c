/*
 * ------------------------------------------------------------------------
 * 名称：sb-screen-clear.c
 * 说明：sb_screen_clear 的实现：清屏缓冲。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $30 子功能 $0A：$0652 的位 0-3 选名称表，单元值写 $064C/$064D。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_screen_clear(unsigned char tables, unsigned int cell)
{
    SB_BYTE_AT(SB_PARAM_0652) = tables & 0x0F;
    SB_BYTE_AT(SB_PARAM_064C) = (unsigned char)cell;
    SB_BYTE_AT(SB_PARAM_064D) = (unsigned char)(cell >> 8);
    sb_syscall(SB_FN_DMA_NAMETABLE, 0x0A);
}
