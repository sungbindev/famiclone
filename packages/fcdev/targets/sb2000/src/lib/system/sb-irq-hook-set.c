/*
 * ------------------------------------------------------------------------
 * 名称：sb-irq-hook-set.c
 * 说明：sb_irq_hook_set 的实现：登记一个中断钩子的页组与入口。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：钩子号 0 定时器、1 中断 3、2 中断 1、3 中断 4、4 鼠标、5 键盘（与 sb_irq_hook_reset 一致），6 中断 2；写入时关中断。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

/** 各钩子的地址（零页，低字节在前） */
static const unsigned char vectors[7] = {
    SB_TIMER_HOOK_VEC_LO, SB_IRQ3_HOOK_VEC_LO, SB_IRQ1_HOOK_VEC_LO, SB_IRQ4_HOOK_VEC_LO,
    SB_MOUSE_HOOK_VEC_LO, SB_KBD_HOOK_VEC_LO, SB_IRQ2_HOOK_VEC_LO
};

/** 各钩子的页组单元 */
static const unsigned int pages[7] = {
    SB_TIMER_HOOK_PAGE, SB_IRQ3_HOOK_PAGE, SB_IRQ1_HOOK_PAGE, SB_IRQ4_HOOK_PAGE,
    SB_MOUSE_HOOK_PAGE, SB_KBD_HOOK_PAGE, SB_IRQ2_HOOK_PAGE
};

void sb_irq_hook_set(unsigned char hook, unsigned char group, unsigned int entry)
{
    unsigned char vector;
    unsigned char saved;

    if (hook > 6) {
        return;
    }
    vector = vectors[hook];
    saved = sbi_interrupts_off();
    SB_BYTE_AT(pages[hook]) = group;
    SB_BYTE_AT(vector) = (unsigned char)entry;
    SB_BYTE_AT(vector + 1) = (unsigned char)(entry >> 8);
    sbi_interrupts_restore(saved);
}
