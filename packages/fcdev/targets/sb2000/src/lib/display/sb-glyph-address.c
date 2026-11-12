/*
 * ------------------------------------------------------------------------
 * 名称：sb-glyph-address.c
 * 说明：sb_glyph_address 的实现：查字模地址。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $3B：字符码 $064A（高）、$064B（低），结果页在 $0644，偏移在 $0E/$0F（高 4 位是窗口位，要去掉），字模形式在 $0638 低 3 位。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_glyph_address(unsigned int code, unsigned char *page, unsigned int *offset, unsigned char *shape)
{
    unsigned char status;

    SB_BYTE_AT(SB_PARAM_064A) = (unsigned char)(code >> 8);
    SB_BYTE_AT(SB_PARAM_064B) = (unsigned char)code;
    SB_BYTE_AT(SB_SYS_STATUS) = 0;
    sb_syscall(SB_FN_GLYPH_ADDR, 0);
    status = SB_BYTE_AT(SB_SYS_STATUS);
    if (status == 0) {
        *page = SB_BYTE_AT(SB_GLYPH_PAGE);
        *offset = SBI_GET16(SB_GLYPH_OFFSET_LO) & 0x0FFF;
        *shape = SB_BYTE_AT(SB_DISPLAY_MODE) & 0x07;
    }
    return status;
}
