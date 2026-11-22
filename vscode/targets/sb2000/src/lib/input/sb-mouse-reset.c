/*
 * ------------------------------------------------------------------------
 * 名称：sb-mouse-reset.c
 * 说明：sb_mouse_reset 的实现：复位鼠标，返回鼠标是否接入。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $14，结果在 $0633。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_mouse_reset(void)
{
    sb_syscall(SB_FN_MOUSE_RESET, 0);
    return SB_BYTE_AT(SB_MOUSE_PRESENT);
}
