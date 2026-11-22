/*
 * ------------------------------------------------------------------------
 * 名称：sb-int40-to-text.c
 * 说明：sb_int40_to_text 的实现：40 位整数转数字文字。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $4E 子功能 1：整数放 $06AF-$06B3，$069A 是有效位数，$069B-$06A7 是 13 个数字；调用前把 $06A8 清 0，使数按非负处理。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_int40_to_text(const unsigned char *value, char *digits, unsigned char *length)
{
    unsigned char status;

    SB_BYTE_AT(SB_ARG_06A8) = 0;
    sbi_poke(SB_ARG_06AF, value, 5);
    sb_syscall(SB_FN_DECIMAL_FORMAT, 1);
    status = SB_BYTE_AT(SB_SYS_STATUS);
    if (status == 0) {
        *length = SB_BYTE_AT(SB_ARG_069A);
        sbi_peek((unsigned char *)digits, SB_ARG_069A + 1, 13);
        digits[13] = 0;
    }
    return status;
}
