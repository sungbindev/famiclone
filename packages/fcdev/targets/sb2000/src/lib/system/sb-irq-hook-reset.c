/*
 * ------------------------------------------------------------------------
 * 名称：sb-irq-hook-reset.c
 * 说明：sb_irq_hook_reset 的实现：把一个中断钩子恢复成空钩子。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $13 子功能 $88，钩子号写在 $0652；钩子号超出 0-5 时不调用。
 * ------------------------------------------------------------------------
 */
#include <sb2000.h>

void sb_irq_hook_reset(unsigned char hook)
{
    if (hook > 5) {
        return;
    }
    SB_BYTE_AT(SB_PARAM_0652) = hook;
    sb_syscall(SB_FN_IRQ_MASK, 0x88);
}
