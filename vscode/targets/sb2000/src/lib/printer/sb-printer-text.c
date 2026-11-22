/*
 * ------------------------------------------------------------------------
 * 名称：sb-printer-text.c
 * 说明：sb_printer_text 的实现：打印一段文字。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $76 子功能 0：文字起点用 $2C 换算成远指针放 $0641 与 $14/$15，$064F 为 0 表示以 0 结尾。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_printer_text(const char *text, const sb_printer_style_t *style)
{
    unsigned char page;
    unsigned int offset;

    SB_BYTE_AT(SB_PARAM_0646) = style ? style->width : 0;
    SB_BYTE_AT(SB_PARAM_0647) = style ? style->dots : 0;
    SB_BYTE_AT(SB_PARAM_0648) = style ? style->gap : 0;
    SB_BYTE_AT(SB_PARAM_0649) = style ? style->feed : 0x18;
    SB_BYTE_AT(SB_PARAM_064E) = style ? style->transform : 0;
    SB_BYTE_AT(SB_PARAM_064F) = 0;
    sb_address_to_far(SB_BYTE_AT(SB_PROGRAM_START_PAGE), (unsigned int)text, &page, &offset);
    SB_BYTE_AT(SB_SYS_PAGE) = page;
    SB_BYTE_AT(SB_FAR_OFFSET_LO) = (unsigned char)offset;
    SB_BYTE_AT(SB_FAR_OFFSET_HI) = (unsigned char)(offset >> 8);
    sb_syscall(SB_FN_PRINTER, 0);
}
