/*
 * ------------------------------------------------------------------------
 * 名称：sb-float-to-text.c
 * 说明：sb_float_to_text 的实现：浮点数转十进制文字。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $4E 子功能 0：浮点数放 $06A8 起，文字 13 个字符在 $069A 起。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_float_to_text(const sb_float_t *value, char *text)
{
    sbi_poke(SB_ARG_06A8, (const unsigned char *)value, 7);
    sb_syscall(SB_FN_DECIMAL_FORMAT, 0);
    sbi_peek((unsigned char *)text, SB_ARG_069A, 13);
    text[13] = 0;
}
