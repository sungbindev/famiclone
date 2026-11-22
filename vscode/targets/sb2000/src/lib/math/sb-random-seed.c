/*
 * ------------------------------------------------------------------------
 * 名称：sb-random-seed.c
 * 说明：sb_random_seed 的实现：设置伪随机数寄存器。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：寄存器在 $0696-$0699，低字节在前。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_random_seed(unsigned long seed)
{
    SB_BYTE_AT(SB_RANDOM_STATE) = (unsigned char)seed;
    SB_BYTE_AT(SB_RANDOM_STATE + 1) = (unsigned char)(seed >> 8);
    SB_BYTE_AT(SB_RANDOM_STATE + 2) = (unsigned char)(seed >> 16);
    SB_BYTE_AT(SB_RANDOM_STATE + 3) = (unsigned char)(seed >> 24);
}
