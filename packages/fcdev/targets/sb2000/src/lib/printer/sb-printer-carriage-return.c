/*
 * ------------------------------------------------------------------------
 * 名称：sb-printer-carriage-return.c
 * 说明：sb_printer_carriage_return 的实现：复位并回车。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $76 子功能 3。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_printer_carriage_return(void)
{
    sb_syscall(SB_FN_PRINTER, 3);
}
