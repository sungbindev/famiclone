/*
 * ------------------------------------------------------------------------
 * 名称：sb-putc.c
 * 说明：sb_putc 的实现：在光标处显示一个字符并把光标右移一格。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：换行符 \n 换行，回车符 \r 回到行首；其他字符原样显示。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_putc(char c)
{
    char text[2];

    if (c == '\n') {
        sb_newline();
    } else if (c == '\r') {
        SB_BYTE_AT(SB_CURSOR_COL) = 1;
    } else {
        text[0] = c;
        text[1] = 0;
        sb_print(text);
    }
}
