/*
 * ------------------------------------------------------------------------
 * 名称：sb-mouse-set-limits.c
 * 说明：sb_mouse_set_limits 的实现：设置鼠标坐标的范围。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $6C 子功能 0 同时把 $0652 保存成双击间隔，并在返回前拿 $0652 当子功能号继续比较（原程序的疏漏），
 *       所以这里先写 0，执行完再用子功能 1 把原来的双击间隔写回。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_mouse_set_limits(unsigned char x_min, unsigned char y_min, unsigned char x_max, unsigned char y_max)
{
    unsigned char interval = SB_BYTE_AT(SB_MOUSE_PARAM);

    SB_BYTE_AT(SB_PARAM_0646) = x_min;
    SB_BYTE_AT(SB_PARAM_0647) = y_min;
    SB_BYTE_AT(SB_PARAM_0648) = x_max;
    SB_BYTE_AT(SB_PARAM_0649) = y_max;
    SB_BYTE_AT(SB_PARAM_0652) = 0;
    sb_syscall(SB_FN_MOUSE, SB_SUB_MOUSE_SET_LIMITS);
    SB_BYTE_AT(SB_PARAM_0652) = interval;
    sb_syscall(SB_FN_MOUSE, 1);
}
