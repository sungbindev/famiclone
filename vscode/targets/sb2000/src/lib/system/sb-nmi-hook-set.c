/*
 * ------------------------------------------------------------------------
 * 名称：sb-nmi-hook-set.c
 * 说明：sb_nmi_hook_set 的实现：登记一个 NMI 钩子的页组与入口。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：钩子的地址在零页，页组在内核 RAM；写入时关中断，避免被钩子调用打断而读到只写了一半的地址。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_nmi_hook_set(unsigned char hook, unsigned char group, unsigned int entry)
{
    unsigned char vector;
    unsigned int page;
    unsigned char saved;

    if (hook == 5) {
        vector = SB_NMI_HOOK5_VEC_LO;
        page = SB_NMI_HOOK5_PAGE;
    } else if (hook == 4) {
        vector = SB_NMI_HOOK4_VEC_LO;
        page = SB_NMI_HOOK4_PAGE;
    } else {
        return;
    }
    saved = sbi_interrupts_off();
    SB_BYTE_AT(page) = group;
    SB_BYTE_AT(vector) = (unsigned char)entry;
    SB_BYTE_AT(vector + 1) = (unsigned char)(entry >> 8);
    sbi_interrupts_restore(saved);
}
