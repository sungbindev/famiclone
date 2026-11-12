/*
 * ------------------------------------------------------------------------
 * 名称：sb-print.c
 * 说明：sb_print 的实现：在光标处显示字符串并把光标移到字符串之后。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：运行库的 LIB_PRINT_STRING 不移动光标，这里按显示宽度（西文 1 格、GB2312 汉字 2 格）逐格右移光标，行满时由运行库换行。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_print(const char *s)
{
    const unsigned char *p = (const unsigned char *)s;
    unsigned char width = 0;

    while (*p != 0) {
        if (*p >= 0xA1 && p[1] != 0) {
            width += 2;
            p += 2;
        } else {
            width += 1;
            p += 1;
        }
    }
    SB_BYTE_AT(SB_STR_PTR_LO) = (unsigned char)(unsigned int)s;
    SB_BYTE_AT(SB_STR_PTR_HI) = (unsigned char)((unsigned int)s >> 8);
    SB_BYTE_AT(SB_LIB_GROUP) = SB_BYTE_AT(SB_PROGRAM_START_PAGE);
    sb_call_banked(0, SB_LIB_PRINT_STRING);
    while (width != 0) {
        sb_call_banked(0, SB_LIB_CURSOR_RIGHT);
        --width;
    }
}
