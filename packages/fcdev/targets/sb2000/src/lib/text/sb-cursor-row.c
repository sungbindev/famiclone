/*
 * ------------------------------------------------------------------------
 * 名称：sb-cursor-row.c
 * 说明：sb_cursor_row 的实现：取光标所在的显示行。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：读 CURSOR_ROW。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_cursor_row(void)
{
    return SB_BYTE_AT(SB_CURSOR_ROW);
}
