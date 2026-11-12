/*
 * ------------------------------------------------------------------------
 * 名称：sb-ime-set-state.c
 * 说明：sb_ime_set_state 的实现：设置输入法状态。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $6D 子功能 2。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_ime_set_state(unsigned char allow, unsigned char flags)
{
    SB_BYTE_AT(SB_PARAM_0652) = allow;
    SB_BYTE_AT(SB_PARAM_0653) = flags;
    sb_syscall(SB_FN_IME, 2);
}
