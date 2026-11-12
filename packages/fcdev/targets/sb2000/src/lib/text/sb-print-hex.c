/*
 * ------------------------------------------------------------------------
 * 名称：sb-print-hex.c
 * 说明：sb_print_hex 的实现：在光标处显示一个字节的两位十六进制数。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：运行库自带的 LIB_PRINT_HEX 经系统调用传不进 A，所以自己转换成字符再显示。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_print_hex(unsigned char value)
{
    static const char digits[] = "0123456789ABCDEF";
    char text[3];

    text[0] = digits[value >> 4];
    text[1] = digits[value & 0x0F];
    text[2] = 0;
    sb_print(text);
}
