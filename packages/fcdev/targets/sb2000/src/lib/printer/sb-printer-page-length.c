/*
 * ------------------------------------------------------------------------
 * 名称：sb-printer-page-length.c
 * 说明：sb_printer_page_length 的实现：设置页长计数。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $76 子功能 $0F：纸张类型写 $0652，页长写 $04BC（高）、$04BD（低）。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_printer_page_length(unsigned char paper)
{
    SB_BYTE_AT(SB_PARAM_0652) = paper;
    sb_syscall(SB_FN_PRINTER, 15);
}
