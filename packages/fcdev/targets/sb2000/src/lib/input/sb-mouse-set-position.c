/*
 * ------------------------------------------------------------------------
 * 名称：sb-mouse-set-position.c
 * 说明：sb_mouse_set_position 的实现：设置鼠标光标的坐标。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $6C 子功能 4，X 写 $0652，Y 写 $0653。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_mouse_set_position(unsigned char x, unsigned char y)
{
    SB_BYTE_AT(SB_PARAM_0652) = x;
    SB_BYTE_AT(SB_PARAM_0653) = y;
    sb_syscall(SB_FN_MOUSE, SB_SUB_MOUSE_SET_POS);
}
