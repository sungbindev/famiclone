/*
 * ------------------------------------------------------------------------
 * 名称：sb-int40-from-text.c
 * 说明：sb_int40_from_text 的实现：数字文字转 40 位整数。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $4D 子功能 1：$069A 低 4 位是位数，$069B 起是数字，结果 5 个字节在 $06AA-$06AE。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_int40_from_text(const char *digits, unsigned char count, unsigned char *value)
{
    SB_BYTE_AT(SB_ARG_069A) = count & 0x0F;
    sbi_poke(SB_ARG_069A + 1, (const unsigned char *)digits, count);
    sb_syscall(SB_FN_DECIMAL_PARSE, 1);
    sbi_peek(value, SB_ARG_06AA, 5);
}
