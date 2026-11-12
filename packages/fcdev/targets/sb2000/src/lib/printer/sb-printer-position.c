/*
 * ------------------------------------------------------------------------
 * 名称：sb-printer-position.c
 * 说明：sb_printer_position 的实现：横向定位。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $76 子功能 5（绝对）与 6（相对）：位置写 $04B3/$04B4。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_printer_position(unsigned char relative, unsigned int position)
{
    SB_BYTE_AT(SB_PRN_POS_LO) = (unsigned char)position;
    SB_BYTE_AT(SB_PRN_POS_HI) = (unsigned char)(position >> 8);
    sb_syscall(SB_FN_PRINTER, relative ? 6 : 5);
}
