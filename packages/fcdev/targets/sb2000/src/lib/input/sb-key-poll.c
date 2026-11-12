/*
 * ------------------------------------------------------------------------
 * 名称：sb-key-poll.c
 * 说明：sb_key_poll 的实现：不等待，取队列里的一个按键。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $6B 子功能 3；队列为空时 ASCII 与扫描码都是 0。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned int sb_key_poll(void)
{
    sb_syscall(SB_FN_INPUT, SB_SUB_KEY_GET_NOWAIT);
    return ((unsigned int)SB_BYTE_AT(SB_KEY_SCAN) << 8) | SB_BYTE_AT(SB_KEY_ASCII);
}
