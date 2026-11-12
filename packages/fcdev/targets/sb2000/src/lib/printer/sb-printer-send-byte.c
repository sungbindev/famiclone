/*
 * ------------------------------------------------------------------------
 * 名称：sb-printer-send-byte.c
 * 说明：sb_printer_send_byte 的实现：发送一个字节。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $76 子功能 7：字节放 $04A4；$0640 为 $FF 时什么都不发，调用前先清 0。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_printer_send_byte(unsigned char value)
{
    SB_BYTE_AT(SB_PRN_BYTE) = value;
    SB_BYTE_AT(SB_SYS_STATUS) = 0;
    sb_syscall(SB_FN_PRINTER, 7);
}
