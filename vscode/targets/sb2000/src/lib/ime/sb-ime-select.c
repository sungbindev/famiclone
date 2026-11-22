/*
 * ------------------------------------------------------------------------
 * 名称：sb-ime-select.c
 * 说明：sb_ime_select 的实现：选择输入法。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $6D 子功能 1：序号与标志合起来写 $0652。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_ime_select(unsigned char method, unsigned char flags)
{
    SB_BYTE_AT(SB_PARAM_0652) = (method & 0x0F) | (flags & SB_IME_CHECK_ALLOWED);
    SB_BYTE_AT(SB_SYS_STATUS) = 0;
    sb_syscall(SB_FN_IME, 1);
    return SB_BYTE_AT(SB_SYS_STATUS);
}
