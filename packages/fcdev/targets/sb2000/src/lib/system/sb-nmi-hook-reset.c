/*
 * ------------------------------------------------------------------------
 * 名称：sb-nmi-hook-reset.c
 * 说明：sb_nmi_hook_reset 的实现：把一个 NMI 钩子恢复成空钩子。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $12 子功能 $88，$0652 为 0 复位钩子 5，非 0 复位钩子 4；钩子号不是 4、5 时不调用。
 * ------------------------------------------------------------------------
 */
#include <sb2000.h>

void sb_nmi_hook_reset(unsigned char hook)
{
    if (hook != 4 && hook != 5) {
        return;
    }
    SB_BYTE_AT(SB_PARAM_0652) = (hook == 4);
    sb_syscall(SB_FN_PPU_CONTROL, 0x88);
}
