/*
 * ------------------------------------------------------------------------
 * 名称：sb-window-restore.c
 * 说明：sb_window_restore 的实现：还原窗口。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $59 子功能 4：窗口号写 $0652。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_window_restore(unsigned char id)
{
    SB_BYTE_AT(SB_PARAM_0652) = id;
    sb_syscall(SB_FN_WINDOW, 4);
}
