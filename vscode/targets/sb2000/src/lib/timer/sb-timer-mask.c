/*
 * ------------------------------------------------------------------------
 * 名称：sb-timer-mask.c
 * 说明：sb_timer_mask 的实现：屏蔽定时器中断并清除下溢标志。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：其余位取 DOS 在影子 $062F 里的值，不能写 1 去清键盘等中断。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_timer_mask(void)
{
    SB_BYTE_AT(SB_HW_IRQ_MASK) = SB_BYTE_AT(SB_IRQ_MASK_SHADOW) | SB_HW_IRQ_TIMER;
}
