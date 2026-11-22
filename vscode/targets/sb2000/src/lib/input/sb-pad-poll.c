/*
 * ------------------------------------------------------------------------
 * 名称：sb-pad-poll.c
 * 说明：sb_pad_poll 的实现：不等待，取一个按下的手柄键。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $6A 子功能 1，键码在 $034D。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_pad_poll(void)
{
    sb_syscall(SB_FN_GAMEPAD, SB_SUB_PAD_GET);
    return SB_BYTE_AT(SB_PAD_KEY);
}
