/*
 * ------------------------------------------------------------------------
 * 名称：sb-irq-mask.c
 * 说明：sb_irq_mask 的实现：屏蔽一个中断源。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $13 子功能 $00-$06；中断源超出 0-6 时不调用。
 * ------------------------------------------------------------------------
 */
#include <sb2000.h>

void sb_irq_mask(unsigned char source)
{
    if (source > 6) {
        return;
    }
    sb_syscall(SB_FN_IRQ_MASK, source);
}
