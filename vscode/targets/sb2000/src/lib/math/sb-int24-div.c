/*
 * ------------------------------------------------------------------------
 * 名称：sb-int24-div.c
 * 说明：sb_int24_div 的实现：3 字节整数除法。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $54 子功能 3：商符号在 $06A3、商在 $06A4-$06A6、余数在 $06A7-$06A9；除数为 0 时 $06A2 为 $FF。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_int24_div(const sb_int24_t *a, const sb_int24_t *b, sb_int24_t *quotient, sb_int24_t *remainder)
{
    sbi_poke(SB_ARG_069A, (const unsigned char *)a, 4);
    sbi_poke(SB_ARG_069E, (const unsigned char *)b, 4);
    sb_syscall(SB_FN_INT24, 3);
    if (SB_BYTE_AT(SB_ARG_069A + 8) == 0xFF) {
        return 1;
    }
    quotient->sign = SB_BYTE_AT(SB_ARG_069A + 9);
    sbi_peek(quotient->value, SB_ARG_069A + 10, 3);
    remainder->sign = SB_BYTE_AT(SB_ARG_069A + 9);
    sbi_peek(remainder->value, SB_ARG_069A + 13, 3);
    return 0;
}
