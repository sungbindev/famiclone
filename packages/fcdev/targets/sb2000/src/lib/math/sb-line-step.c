/*
 * ------------------------------------------------------------------------
 * 名称：sb-line-step.c
 * 说明：sb_line_step 的实现：直线步进。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $53：零页 $08/$09 指向 18 字节参数块，偏移 4 是步数，落点写回偏移 0、1。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_line_step(sb_line_t *line, unsigned char steps)
{
    line->steps = steps;
    sbi_poke(SB_LINE_BLOCK_BUFFER, (const unsigned char *)line, sizeof(sb_line_t));
    SB_BYTE_AT(SB_LINE_BLOCK_PTR) = (unsigned char)SB_LINE_BLOCK_BUFFER;
    SB_BYTE_AT(SB_LINE_BLOCK_PTR + 1) = (unsigned char)(SB_LINE_BLOCK_BUFFER >> 8);
    sb_syscall(SB_FN_LINE_STEP, 0);
    sbi_peek((unsigned char *)line, SB_LINE_BLOCK_BUFFER, sizeof(sb_line_t));
}
