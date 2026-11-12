/*
 * ------------------------------------------------------------------------
 * 名称：sb-window-compact.c
 * 说明：sb_window_compact 的实现：整理窗口保存区。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $59 子功能 5。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_window_compact(void)
{
    sb_syscall(SB_FN_WINDOW, 5);
}
