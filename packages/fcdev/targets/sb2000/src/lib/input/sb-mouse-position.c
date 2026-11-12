/*
 * ------------------------------------------------------------------------
 * 名称：sb-mouse-position.c
 * 说明：sb_mouse_position 的实现：取鼠标光标的坐标。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $6C 子功能 2，X 在 $0646，Y 在 $0647。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_mouse_position(unsigned char *x, unsigned char *y)
{
    sb_syscall(SB_FN_MOUSE, SB_SUB_MOUSE_GET_POS);
    *x = SB_BYTE_AT(SB_PARAM_0646);
    *y = SB_BYTE_AT(SB_PARAM_0647);
}
