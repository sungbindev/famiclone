/*
 * ------------------------------------------------------------------------
 * 名称：sb-ime-get-method.c
 * 说明：sb_ime_get_method 的实现：取当前输入法序号。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $6D 子功能 7：结果在 $0652。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_ime_get_method(void)
{
    sb_syscall(SB_FN_IME, 7);
    return SB_BYTE_AT(SB_PARAM_0652);
}
