/*
 * ------------------------------------------------------------------------
 * 名称：sb-ime-get-state.c
 * 说明：sb_ime_get_state 的实现：取输入法状态。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $6D 子功能 8：结果在 $0652、$0653。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_ime_get_state(unsigned char *allow, unsigned char *flags)
{
    sb_syscall(SB_FN_IME, 8);
    *allow = SB_BYTE_AT(SB_PARAM_0652);
    *flags = SB_BYTE_AT(SB_PARAM_0653);
}
