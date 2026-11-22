/*
 * ------------------------------------------------------------------------
 * 名称：sb-print-uint.c
 * 说明：sb_print_uint 的实现：在光标处显示一个无符号十进制数。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：逐位取出再显示，不使用库里的格式化函数。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_print_uint(unsigned int value)
{
    char text[6];
    unsigned char index = 5;

    text[5] = 0;
    do {
        --index;
        text[index] = (char)('0' + value % 10);
        value /= 10;
    } while (value != 0);
    sb_print(&text[index]);
}
