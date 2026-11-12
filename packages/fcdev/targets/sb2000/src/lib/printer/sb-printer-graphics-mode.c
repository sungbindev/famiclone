/*
 * ------------------------------------------------------------------------
 * 名称：sb-printer-graphics-mode.c
 * 说明：sb_printer_graphics_mode 的实现：进入位图模式。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $76 子功能 8。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_printer_graphics_mode(void)
{
    sb_syscall(SB_FN_PRINTER, 8);
}
