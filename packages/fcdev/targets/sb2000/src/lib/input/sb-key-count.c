/*
 * ------------------------------------------------------------------------
 * 名称：sb-key-count.c
 * 说明：sb_key_count 的实现：按键队列里的按键数。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $6B 子功能 6，结果放在 $034B。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_key_count(void)
{
    sb_syscall(SB_FN_INPUT, SB_SUB_KEY_COUNT);
    return SB_BYTE_AT(SB_KEY_ASCII);
}
