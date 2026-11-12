/*
 * ------------------------------------------------------------------------
 * 名称：copy.c
 * 说明：复制文件的 SB-DOS 程序：在 DOS 下输入“cp 源文件 目标文件”，把源文件整个复制成目标文件。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：程序名取 CP，避开 DOS 内部的 COPY 命令。
 *       演示整个文件的加载与保存：sb_file_load 把源文件读入新分配的扩展 RAM 页，sb_file_save 把这些页写成目标文件（已存在则覆盖），
 *       最后释放页。出错时也要释放页，再显示“ERROR 码”。
 * ------------------------------------------------------------------------
 */
#include <sb2000.h>

static char source[29];
static char target[29];

/** 从命令行取下一个以空格分隔的词放进 out，没有词时返回 0 */
static unsigned char next_word(const char **cursor, char *out, unsigned char size)
{
    const char *p = *cursor;
    unsigned char count = 0;

    while (*p == ' ') {
        ++p;
    }
    *cursor = p;
    if (*p == 0) {
        return 0;
    }
    while (*p != 0 && *p != ' ') {
        if (count < size - 1) {
            out[count++] = *p;
        }
        ++p;
    }
    out[count] = 0;
    *cursor = p;
    return 1;
}

int main(void)
{
    const char *tail = (const char *)SB_CMD_TAIL;
    unsigned char page = 0;
    unsigned char pages;
    unsigned char error;
    unsigned long size = 0;

    if (!next_word(&tail, source, sizeof(source)) || !next_word(&tail, target, sizeof(target))) {
        sb_puts("USAGE: CP SRC DST");
        sb_getkey();
        return 0;
    }

    error = sb_file_load(source, &page, &size);
    if (error == 0) {
        error = sb_file_save(target, page, size);
    }
    /* 空文件没有分配页，页号为 0 */
    pages = (unsigned char)((size + 4095UL) / 4096UL);
    if (page != 0 && pages != 0) {
        sb_page_free(page, pages);
    }

    if (error == 0) {
        sb_puts("COPY OK");
    } else {
        sb_cursor_set_column(1);
        sb_print("ERROR ");
        sb_print_hex(error);
        sb_newline();
    }
    sb_getkey();
    return 0;
}
