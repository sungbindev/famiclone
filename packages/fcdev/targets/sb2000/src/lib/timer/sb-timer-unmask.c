/*
 * ------------------------------------------------------------------------
 * 名称：sb-timer-unmask.c
 * 说明：sb_timer_unmask 的实现：允许定时器中断。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：其余位取 DOS 在影子 $062F 里的值。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_timer_unmask(void)
{
    SB_BYTE_AT(SB_HW_IRQ_MASK) = SB_BYTE_AT(SB_IRQ_MASK_SHADOW) & (unsigned char)~SB_HW_IRQ_TIMER;
}
