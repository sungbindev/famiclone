/*
 * ------------------------------------------------------------------------
 * 名称：sb-printer-init-port.c
 * 说明：sb_printer_init_port 的实现：初始化并口。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $76 子功能 $0E。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_printer_init_port(void)
{
    sb_syscall(SB_FN_PRINTER, 14);
}
