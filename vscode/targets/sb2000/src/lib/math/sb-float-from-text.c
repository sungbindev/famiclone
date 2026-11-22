/*
 * ------------------------------------------------------------------------
 * 名称：sb-float-from-text.c
 * 说明：sb_float_from_text 的实现：十进制文字转浮点数。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $4D 子功能 0：文字放 $069A 起 13 个字节，结果 7 个字节在 $06A8 起。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_float_from_text(const char *text, sb_float_t *result)
{
    sbi_poke(SB_ARG_069A, (const unsigned char *)text, 13);
    sb_syscall(SB_FN_DECIMAL_PARSE, 0);
    sbi_peek((unsigned char *)result, SB_ARG_06A8, 7);
}
