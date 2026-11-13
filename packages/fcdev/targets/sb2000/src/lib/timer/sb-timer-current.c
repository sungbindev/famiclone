/*
 * ------------------------------------------------------------------------
 * 名称：sb-timer-current.c
 * 说明：sb_timer_current 的实现：读定时器当前值。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：$4036 递减计数。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_timer_current(void)
{
    return SB_BYTE_AT(SB_HW_TIMER_CURRENT);
}
