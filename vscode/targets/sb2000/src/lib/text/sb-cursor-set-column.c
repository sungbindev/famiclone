/*
 * ------------------------------------------------------------------------
 * 名称：sb-cursor-set-column.c
 * 说明：sb_cursor_set_column 的实现：设置光标所在的列。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：列号超出 1-31 时不改。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_cursor_set_column(unsigned char column)
{
    if (column >= 1 && column <= 31) {
        SB_BYTE_AT(SB_CURSOR_COL) = column;
    }
}
