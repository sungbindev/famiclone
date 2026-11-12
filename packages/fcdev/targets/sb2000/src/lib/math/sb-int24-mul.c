/*
 * ------------------------------------------------------------------------
 * 名称：sb-int24-mul.c
 * 说明：sb_int24_mul 的实现：3 字节整数乘法。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $54 子功能 2：符号在 $06A2，48 位乘积在 $06A3-$06A8。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_int24_mul(const sb_int24_t *a, const sb_int24_t *b, unsigned char *sign, unsigned char *product)
{
    sbi_poke(SB_ARG_069A, (const unsigned char *)a, 4);
    sbi_poke(SB_ARG_069E, (const unsigned char *)b, 4);
    sb_syscall(SB_FN_INT24, 2);
    *sign = SB_BYTE_AT(SB_ARG_069A + 8);
    sbi_peek(product, SB_ARG_069A + 9, 6);
}
