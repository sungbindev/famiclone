/*
 * ------------------------------------------------------------------------
 * 名称：sb-screen-flush.c
 * 说明：sb_screen_flush 的实现：名称表缓冲刷新到显存。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $30 子功能 0-3。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_screen_flush(unsigned char table)
{
    sb_syscall(SB_FN_DMA_NAMETABLE, table & 3);
}
