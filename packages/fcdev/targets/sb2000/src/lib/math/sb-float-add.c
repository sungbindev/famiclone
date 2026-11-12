/*
 * ------------------------------------------------------------------------
 * 名称：sb-float-add.c
 * 说明：sb_float_add 的实现：浮点加法。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $51：操作数放 $069A 起与 $06A1 起，每个 7 个字节，结果在 $06A8 起。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_float_add(const sb_float_t *a, const sb_float_t *b, sb_float_t *result)
{
    sbi_poke(SB_ARG_069A, (const unsigned char *)a, 7);
    sbi_poke(SB_ARG_06A1, (const unsigned char *)b, 7);
    sb_syscall(SB_FN_FLOAT_ADD, 0);
    sbi_peek((unsigned char *)result, SB_ARG_06A8, 7);
}
