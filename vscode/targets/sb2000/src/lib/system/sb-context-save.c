/*
 * ------------------------------------------------------------------------
 * 名称：sb-context-save.c
 * 说明：sb_context_save 的实现：把一个恢复点压入系统的上下文栈。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $1C，页组写 $0642、入口地址写 $0C/$0D，栈指针作为子功能号传入。
 * ------------------------------------------------------------------------
 */
#include <sb2000.h>

void sb_context_save(unsigned char group, unsigned int entry, unsigned char stack_pointer)
{
    SB_BYTE_AT(SB_SYS_GROUP) = group;
    SB_BYTE_AT(SB_PROG_ENTRY_LO) = (unsigned char)entry;
    SB_BYTE_AT(SB_PROG_ENTRY_HI) = (unsigned char)(entry >> 8);
    sb_syscall(SB_FN_SAVE_CONTEXT, stack_pointer);
}
