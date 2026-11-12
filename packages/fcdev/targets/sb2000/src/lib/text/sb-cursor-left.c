/*
 * ------------------------------------------------------------------------
 * 名称：sb-cursor-left.c
 * 说明：sb_cursor_left 的实现：光标左移一列。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：调用运行库的 LIB_CURSOR_LEFT。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_cursor_left(void)
{
    sb_call_banked(0, SB_LIB_CURSOR_LEFT);
}
