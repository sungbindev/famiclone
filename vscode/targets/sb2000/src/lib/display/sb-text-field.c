/*
 * ------------------------------------------------------------------------
 * 名称：sb-text-field.c
 * 说明：sb_text_field 的实现：显示一行文字。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $45：列行 $0648/$0649，字符串 $0C/$0D（页组取程序起始页），单元基础值 $064C/$064D，颜色 $064E，调色板 $0654。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_text_field(unsigned char table, unsigned char col, unsigned char row, const char *text, unsigned int cell, unsigned char palette, unsigned char color)
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
    sb_syscall(SB_FN_TEXT_FIELD, table & 3);
}
