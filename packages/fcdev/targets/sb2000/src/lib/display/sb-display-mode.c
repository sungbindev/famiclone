/*
 * ------------------------------------------------------------------------
 * 名称：sb-display-mode.c
 * 说明：sb_display_mode 的实现：设置显示模式。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $11 子功能 0-3。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_display_mode(unsigned char mode)
{
    sb_syscall(SB_FN_DISPLAY_MODE, mode & 3);
}
