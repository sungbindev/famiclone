/*
 * ------------------------------------------------------------------------
 * 名称：sb-pad-wait.c
 * 说明：sb_pad_wait 的实现：等到有手柄键按下。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $6A 子功能 2，键码在 $034D。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_pad_wait(void)
{
    sb_syscall(SB_FN_GAMEPAD, SB_SUB_PAD_WAIT);
    return SB_BYTE_AT(SB_PAD_KEY);
}
