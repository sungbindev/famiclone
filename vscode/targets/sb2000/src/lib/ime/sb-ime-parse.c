/*
 * ------------------------------------------------------------------------
 * 名称：sb-ime-parse.c
 * 说明：sb_ime_parse 的实现：输入字符转内码。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $6D 子功能 $0D：输入放 $6905 起，$6904 是个数，结果在 $064A（高）、$064B（低），不合法时 $064A 为 0。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_ime_parse(const char *chars, unsigned int *code)
{
    sbi_poke(SB_IME_INPUT_BUFFER, (const unsigned char *)chars, 4);
    SB_BYTE_AT(SB_IME_INPUT_COUNT) = 4;
    sb_syscall(SB_FN_IME, 13);
    if (SB_BYTE_AT(SB_PARAM_064A) == 0) {
        return 1;
    }
    *code = ((unsigned int)SB_BYTE_AT(SB_PARAM_064A) << 8) | SB_BYTE_AT(SB_PARAM_064B);
    return 0;
}
