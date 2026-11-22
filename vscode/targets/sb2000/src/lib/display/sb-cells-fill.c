/*
 * ------------------------------------------------------------------------
 * 名称：sb-cells-fill.c
 * 说明：sb_cells_fill 的实现：线性填充单元。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $40 子功能低 2 位选名称表、位 2、3 为 0：起点列行写 $0648/$0649，单元值 $064C/$064D，个数 $064F/$0650，刷新方式 $0651=0，调色板 $0654，$0652=1 表示调色板占高 4 位。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_cells_fill(unsigned char table, unsigned char col, unsigned char row, unsigned int count, unsigned int cell, unsigned char palette)
{
    SB_BYTE_AT(SB_PARAM_0648) = col;
    SB_BYTE_AT(SB_PARAM_0649) = row;
    SB_BYTE_AT(SB_PARAM_064C) = (unsigned char)cell;
    SB_BYTE_AT(SB_PARAM_064D) = (unsigned char)(cell >> 8);
    SB_BYTE_AT(SB_PARAM_064F) = (unsigned char)count;
    SB_BYTE_AT(SB_PARAM_0650) = (unsigned char)(count >> 8);
    SB_BYTE_AT(SB_PARAM_0651) = 0;
    SB_BYTE_AT(SB_PARAM_0652) = 1;
    SB_BYTE_AT(SB_PARAM_0654) = palette;
    sb_syscall(SB_FN_CELLS, table & 3);
}
