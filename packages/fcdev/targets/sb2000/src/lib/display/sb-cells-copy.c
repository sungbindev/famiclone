/*
 * ------------------------------------------------------------------------
 * 名称：sb-cells-copy.c
 * 说明：sb_cells_copy 的实现：复制单元到屏幕。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $40 子功能位 2、3 为 01：源 $0C/$0D（页组取程序起始页），行宽 $064B，个数 $064F/$0650。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_cells_copy(unsigned char table, unsigned char col, unsigned char row, unsigned char width, unsigned char height, const unsigned int *cells)
{
    unsigned int count = (unsigned int)width * height;

    SB_BYTE_AT(SB_PARAM_0648) = col;
    SB_BYTE_AT(SB_PARAM_0649) = row;
    SB_BYTE_AT(SB_PARAM_064A) = 0;
    SB_BYTE_AT(SB_PARAM_064B) = width;
    SB_BYTE_AT(SB_PARAM_064F) = (unsigned char)count;
    SB_BYTE_AT(SB_PARAM_0650) = (unsigned char)(count >> 8);
    SB_BYTE_AT(SB_PARAM_0651) = 0;
    SB_BYTE_AT(SB_SYS_GROUP) = SB_BYTE_AT(SB_PROGRAM_START_PAGE);
    SBI_SET_ENTRY((unsigned int)cells);
    sb_syscall(SB_FN_CELLS, (table & 3) | 0x04);
}
