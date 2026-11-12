/*
 * ------------------------------------------------------------------------
 * 名称：sb-math-div16.c
 * 说明：sb_math_div16 的实现：16 位除以 8 位。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $4B：$069A/$069B 除以 $069C，商回写 $069A/$069B，余数写 $069C；除数为 0 时 $0640 为 $FF。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_math_div16(unsigned int dividend, unsigned char divisor, unsigned int *quotient, unsigned char *remainder)
{
    unsigned char status;

    SB_BYTE_AT(SB_ARG_069A) = (unsigned char)dividend;
    SB_BYTE_AT(SB_ARG_069A + 1) = (unsigned char)(dividend >> 8);
    SB_BYTE_AT(SB_ARG_069A + 2) = divisor;
    sb_syscall(SB_FN_DIV16, 0);
    status = SB_BYTE_AT(SB_SYS_STATUS);
    if (status == 0) {
        *quotient = SBI_GET16(SB_ARG_069A);
        *remainder = SB_BYTE_AT(SB_ARG_069A + 2);
    }
    return status;
}
