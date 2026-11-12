/*
 * ------------------------------------------------------------------------
 * 名称：sb-text-draw.c
 * 说明：sb_text_draw 的实现：画字符串到显存图块。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $3C-$3F：字符串 $0C/$0D（页组取程序起始页），结束符 $064F，图块号 $064C/$064D，颜色 $064E。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_text_draw(unsigned int tile, const char *text, unsigned char terminator, unsigned char color, unsigned char four_planes, unsigned char ascii_only)
{
    SB_BYTE_AT(SB_PARAM_064C) = (unsigned char)tile;
    SB_BYTE_AT(SB_PARAM_064D) = (unsigned char)(tile >> 8);
    SB_BYTE_AT(SB_PARAM_064E) = color;
    SB_BYTE_AT(SB_PARAM_064F) = terminator;
    SB_BYTE_AT(SB_SYS_GROUP) = SB_BYTE_AT(SB_PROGRAM_START_PAGE);
    SBI_SET_ENTRY((unsigned int)text);
    sb_syscall((ascii_only ? SB_FN_PUT_ASCII_C : SB_FN_PUT_STRING_C) + (four_planes ? 1 : 0), 0);
}
