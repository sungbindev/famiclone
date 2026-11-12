/*
 * ------------------------------------------------------------------------
 * 名称：sb-printer-feed.c
 * 说明：sb_printer_feed 的实现：进纸。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $76 子功能 4：n 取自 $0649。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_printer_feed(unsigned char units)
{
    SB_BYTE_AT(SB_PARAM_0649) = units;
    sb_syscall(SB_FN_PRINTER, 4);
}
