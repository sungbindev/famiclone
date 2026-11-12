/*
 * ------------------------------------------------------------------------
 * 名称：sb-vram-bank.c
 * 说明：sb_vram_bank 的实现：设置显存 bank。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $2A：值写 $0642。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_vram_bank(unsigned char bank)
{
    SB_BYTE_AT(SB_SYS_GROUP) = bank;
    sb_syscall(SB_FN_VRAM_BANK, 0);
}
