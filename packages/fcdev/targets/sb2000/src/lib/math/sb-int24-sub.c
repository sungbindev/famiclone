/*
 * ------------------------------------------------------------------------
 * 名称：sb-int24-sub.c
 * 说明：sb_int24_sub 的实现：3 字节整数减法。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $54 子功能 1：操作数放 $069A 起与 $069E 起（符号字节加 3 个数值字节），符号在 $06A2，结果在 $06A4-$06A6，进位在 $06A3。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_int24_sub(const sb_int24_t *a, const sb_int24_t *b, sb_int24_t *result)
{
    sbi_poke(SB_ARG_069A, (const unsigned char *)a, 4);
    sbi_poke(SB_ARG_069E, (const unsigned char *)b, 4);
    sb_syscall(SB_FN_INT24, 1);
    result->sign = SB_BYTE_AT(SB_ARG_069A + 8);
    sbi_peek(result->value, SB_ARG_069A + 10, 3);
    return SB_BYTE_AT(SB_ARG_069A + 9) != 0;
}
