/*
 * ------------------------------------------------------------------------
 * 名称：sb-math-time-text.c
 * 说明：sb_math_time_text 的实现：时分秒转文字。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $4C：参数放 $069A-$069C，结果 8 个字符在 $06AA-$06B1。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_math_time_text(unsigned char hour, unsigned char minute, unsigned char second, char *text)
{
    SB_BYTE_AT(SB_ARG_069A) = hour;
    SB_BYTE_AT(SB_ARG_069A + 1) = minute;
    SB_BYTE_AT(SB_ARG_069A + 2) = second;
    sb_syscall(SB_FN_TIME_TEXT, 0);
    sbi_peek((unsigned char *)text, SB_ARG_06AA, 8);
    text[8] = 0;
}
