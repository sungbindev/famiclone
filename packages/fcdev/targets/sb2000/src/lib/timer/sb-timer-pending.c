/*
 * ------------------------------------------------------------------------
 * 名称：sb-timer-pending.c
 * 说明：sb_timer_pending 的实现：读定时器下溢标志。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：$4033 的位 7。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_timer_pending(void)
{
    return (SB_BYTE_AT(SB_HW_IRQ_PENDING) & SB_HW_IRQ_TIMER) != 0;
}
