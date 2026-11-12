/*
 * ------------------------------------------------------------------------
 * 名称：sb-irq-enable.c
 * 说明：sb_irq_enable 的实现：允许一个中断源。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $13 子功能 $80-$86；中断源超出 0-6 时不调用。
 * ------------------------------------------------------------------------
 */
#include <sb2000.h>

void sb_irq_enable(unsigned char source)
{
    if (source > 6) {
        return;
    }
    sb_syscall(SB_FN_IRQ_MASK, (unsigned char)(0x80 | source));
}
