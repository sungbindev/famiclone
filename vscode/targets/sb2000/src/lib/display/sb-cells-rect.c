/*
 * ------------------------------------------------------------------------
 * 名称：sb-cells-rect.c
 * 说明：sb_cells_rect 的实现：填充矩形区域。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：对每一行调用一次 sb_cells_fill。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_cells_rect(unsigned char table, unsigned char col, unsigned char row, unsigned char width, unsigned char height, unsigned int cell, unsigned char palette)
{
    unsigned char line;

    for (line = 0; line < height; ++line) {
        sb_cells_fill(table, col, row + line, width, cell, palette);
    }
}
