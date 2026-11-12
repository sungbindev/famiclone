/*
 * ------------------------------------------------------------------------
 * 名称：sb-vram-load.c
 * 说明：sb_vram_load 的实现：扩展 RAM 到显存的 DMA。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $33：源页 $0641 与偏移 $14/$15，目的 $0646/$0647，长度 $0650/$064F。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_vram_load(unsigned char page, unsigned int offset, unsigned int address, unsigned int length)
{
    SBI_SET_FAR(page, offset);
    SB_BYTE_AT(SB_PARAM_0646) = (unsigned char)(address >> 8);
    SB_BYTE_AT(SB_PARAM_0647) = (unsigned char)address;
    SB_BYTE_AT(SB_PARAM_064F) = (unsigned char)length;
    SB_BYTE_AT(SB_PARAM_0650) = (unsigned char)(length >> 8);
    sb_syscall(SB_FN_VRAM_LOAD, 0);
}
