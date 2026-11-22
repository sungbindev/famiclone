/*
 * ------------------------------------------------------------------------
 * 名称：sb-window-save.c
 * 说明：sb_window_save 的实现：保存屏幕矩形。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $59 子功能 0-3（名称表号）：左上角 $0646/$0647，右下角 $0648/$0649，窗口号返回在 $0652。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_window_save(unsigned char table, unsigned char left, unsigned char top, unsigned char right, unsigned char bottom)
{
    SB_BYTE_AT(SB_PARAM_0646) = left;
    SB_BYTE_AT(SB_PARAM_0647) = top;
    SB_BYTE_AT(SB_PARAM_0648) = right;
    SB_BYTE_AT(SB_PARAM_0649) = bottom;
    sb_syscall(SB_FN_WINDOW, table & 3);
    return SB_BYTE_AT(SB_PARAM_0652);
}
