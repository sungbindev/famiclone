/*
 * ------------------------------------------------------------------------
 * 名称：sb-text-field-ascii.c
 * 说明：sb_text_field_ascii 的实现：显示一行西文。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $46，参数同 $45。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_text_field_ascii(unsigned char table, unsigned char col, unsigned char row, const char *text, unsigned int cell, unsigned char palette, unsigned char color)
{
    SB_BYTE_AT(SB_PARAM_0648) = col;
    SB_BYTE_AT(SB_PARAM_0649) = row;
    SB_BYTE_AT(SB_PARAM_064C) = (unsigned char)cell;
    SB_BYTE_AT(SB_PARAM_064D) = (unsigned char)(cell >> 8);
    SB_BYTE_AT(SB_PARAM_064E) = color;
    SB_BYTE_AT(SB_PARAM_0652) = 0;
    SB_BYTE_AT(SB_PARAM_0654) = palette;
    SB_BYTE_AT(SB_SYS_GROUP) = SB_BYTE_AT(SB_PROGRAM_START_PAGE);
    SBI_SET_ENTRY((unsigned int)text);
    sb_syscall(SB_FN_TEXT_FIELD_ASCII, table & 3);
}
