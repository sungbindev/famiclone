/*
 * ------------------------------------------------------------------------
 * 名称：sb-key-wait.c
 * 说明：sb_key_wait 的实现：等到有按键，取出它的 ASCII 码与扫描码。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $6B 子功能 4，结果在 $034B（ASCII）与 $034C（扫描码）。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned int sb_key_wait(void)
{
    sb_syscall(SB_FN_INPUT, SB_SUB_KEY_GET_WAIT);
    return ((unsigned int)SB_BYTE_AT(SB_KEY_SCAN) << 8) | SB_BYTE_AT(SB_KEY_ASCII);
}
