/*
 * ------------------------------------------------------------------------
 * 名称：sb-cursor-column.c
 * 说明：sb_cursor_column 的实现：取光标所在的列。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：读 CURSOR_COL。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_cursor_column(void)
{
    return SB_BYTE_AT(SB_CURSOR_COL);
}
