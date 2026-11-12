/*
 * ------------------------------------------------------------------------
 * 名称：sb-random-next.c
 * 说明：sb_random_next 的实现：伪随机数步进一次。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $4A 子功能不为 0 步进一次。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned long sb_random_next(void)
{
    sb_syscall(SB_FN_RANDOM, 1);
    return (unsigned long)SB_BYTE_AT(SB_RANDOM_STATE)
        | ((unsigned long)SB_BYTE_AT(SB_RANDOM_STATE + 1) << 8)
        | ((unsigned long)SB_BYTE_AT(SB_RANDOM_STATE + 2) << 16)
        | ((unsigned long)SB_BYTE_AT(SB_RANDOM_STATE + 3) << 24);
}
