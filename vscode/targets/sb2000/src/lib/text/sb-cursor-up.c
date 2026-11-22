/*
 * ------------------------------------------------------------------------
 * 名称：sb-cursor-up.c
 * 说明：sb_cursor_up 的实现：光标移到上一行的末尾。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：调用运行库的 LIB_CURSOR_UP。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_cursor_up(void)
{
    sb_call_banked(0, SB_LIB_CURSOR_UP);
}
