/*
 * ------------------------------------------------------------------------
 * 名称：sb-cell-address.c
 * 说明：sb_cell_address 的实现：求单元的远地址。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $30 子功能 4-7：列行写 $0648/$0649，结果在 $0641 与 $14/$15。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_cell_address(unsigned char table, unsigned char col, unsigned char row, unsigned char *page, unsigned int *offset)
{
    SB_BYTE_AT(SB_PARAM_0648) = col;
    SB_BYTE_AT(SB_PARAM_0649) = row;
    sb_syscall(SB_FN_DMA_NAMETABLE, (table & 3) + 4);
    *page = SB_BYTE_AT(SB_SYS_PAGE);
    *offset = SBI_GET16(SB_FAR_OFFSET_LO);
}
