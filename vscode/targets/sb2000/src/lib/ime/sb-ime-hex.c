/*
 * ------------------------------------------------------------------------
 * 名称：sb-ime-hex.c
 * 说明：sb_ime_hex 的实现：内码转十六进制字符。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $6D 子功能 $0E：内码放 $064A（高）、$064B（低），结果在 $6923-$6926。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_ime_hex(unsigned int code, char *text)
{
    SB_BYTE_AT(SB_PARAM_064A) = (unsigned char)(code >> 8);
    SB_BYTE_AT(SB_PARAM_064B) = (unsigned char)code;
    sb_syscall(SB_FN_IME, 14);
    sbi_peek((unsigned char *)text, SB_IME_KEY_OUT, 4);
    text[4] = 0;
}
