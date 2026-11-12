/*
 * ------------------------------------------------------------------------
 * 名称：say.c
 * 说明：显示命令行参数的 SB-DOS 程序：在 DOS 下输入“say hello world”，屏幕上显示 HELLO WORLD。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：DOS 把程序名之后的部分转成大写，以零结尾放在扩展 RAM 的 SB_CMD_TAIL 起，第一个字节是分隔用的空格；
 *       没有参数时第一个字节为 0，之后的内容是上一条命令的残留，不能显示。
 *       字符串要在程序窗口里才能交给 sb_puts，所以先复制进程序自己的缓冲。
 * ------------------------------------------------------------------------
 */
#include <sb2000.h>

/** 参数缓冲，最多显示 39 个字符 */
static char text[40];

int main(void)
{
    const char *tail = (const char *)SB_CMD_TAIL;
    unsigned char count = 0;

    if (tail[0] != 0) {
        while (count < sizeof(text) - 1 && tail[count + 1] != 0) {
            text[count] = tail[count + 1];
            ++count;
        }
        text[count] = 0;
        sb_puts(text);
    }

    sb_getkey();
    return 0;
}
