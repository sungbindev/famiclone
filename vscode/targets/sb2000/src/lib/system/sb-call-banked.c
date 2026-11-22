/*
 * ------------------------------------------------------------------------
 * 名称：sb-call-banked.c
 * 说明：sb_call_banked 的实现：映射页组后调用其中的例程。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $25，页组写 $0642，入口地址写 $0C/$0D；入口返回后窗口恢复成调用前的样子。
 * ------------------------------------------------------------------------
 */
#include <sb2000.h>

void sb_call_banked(unsigned char group, unsigned int entry)
{
    SB_BYTE_AT(SB_SYS_GROUP) = group;
    SB_BYTE_AT(SB_PROG_ENTRY_LO) = (unsigned char)entry;
    SB_BYTE_AT(SB_PROG_ENTRY_HI) = (unsigned char)(entry >> 8);
    sb_syscall(SB_FN_CALL_BANKED, 0);
}
