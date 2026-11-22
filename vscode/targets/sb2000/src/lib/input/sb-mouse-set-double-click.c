/*
 * ------------------------------------------------------------------------
 * 名称：sb-mouse-set-double-click.c
 * 说明：sb_mouse_set_double_click 的实现：设置鼠标双击的间隔。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $6C 子功能 1，把 $0652 保存到 $035E。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_mouse_set_double_click(unsigned char interval)
{
    SB_BYTE_AT(SB_PARAM_0652) = interval;
    sb_syscall(SB_FN_MOUSE, 1);
}
