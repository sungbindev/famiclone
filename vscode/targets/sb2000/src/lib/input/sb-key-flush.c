/*
 * ------------------------------------------------------------------------
 * 名称：sb-key-flush.c
 * 说明：sb_key_flush 的实现：清空按键队列。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $6B 子功能 5。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_key_flush(void)
{
    sb_syscall(SB_FN_INPUT, SB_SUB_KEY_FLUSH);
}
