/*
 * ------------------------------------------------------------------------
 * 名称：sb-printer-page-left.c
 * 说明：sb_printer_page_left 的实现：取本页剩余走纸量。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：读 $04BC（高）、$04BD（低）。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned int sb_printer_page_left(void)
{
    return ((unsigned int)SB_BYTE_AT(SB_PRN_PAGE_LEFT) << 8) | SB_BYTE_AT(SB_PRN_PAGE_LEFT + 1);
}
