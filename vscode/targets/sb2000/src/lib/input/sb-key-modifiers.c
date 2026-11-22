/*
 * ------------------------------------------------------------------------
 * 名称：sb-key-modifiers.c
 * 说明：sb_key_modifiers 的实现：取修饰键状态。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：读 KBD_FLAGS（$0344）。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_key_modifiers(void)
{
    return SB_BYTE_AT(SB_KBD_FLAGS);
}
