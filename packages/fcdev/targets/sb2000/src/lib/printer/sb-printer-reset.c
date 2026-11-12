/*
 * ------------------------------------------------------------------------
 * 名称：sb-printer-reset.c
 * 说明：sb_printer_reset 的实现：打印机复位。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $76 子功能 1。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_printer_reset(void)
{
    sb_syscall(SB_FN_PRINTER, 1);
}
