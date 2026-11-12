/*
 * ------------------------------------------------------------------------
 * 名称：show.c
 * 说明：显示文件开头内容的 SB-DOS 程序：在 DOS 下输入“show hello.txt”，显示文件的前 90 个字节（每行 30 个字符，共 3 行）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：演示文件接口的标准顺序：取命令行参数 → 打开 → 取大小 → 读入 → 关闭；每个调用都检查出错码，出错时先关闭文件再显示“ERROR 码”。
 *       控制字符显示成“.”。
 * ------------------------------------------------------------------------
 */
#include <sb2000.h>

/** 一次最多显示的字节数 */
#define SHOW_BYTES 90
/** 每行显示的字符数 */
#define LINE_CHARS 30

static char path[29];
static unsigned char data[SHOW_BYTES];
static char line[LINE_CHARS + 1];

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

/** 显示“ERROR 码”并等待按键 */
static void report_error(unsigned char code)
{
    sb_cursor_set_column(1);
    sb_print("ERROR ");
    sb_print_hex(code);
    sb_newline();
    sb_getkey();
}

int main(void)
{
    const char *tail = (const char *)SB_CMD_TAIL;
    unsigned char handle;
    unsigned char error;
    unsigned long size;
    unsigned int count = 0;
    unsigned int start;
    unsigned int n;

    if (!next_word(&tail, path, sizeof(path))) {
        sb_puts("USAGE: SHOW FILENAME");
        sb_getkey();
        return 0;
    }

    error = sb_file_open(path, SB_MODE_OPEN, &handle);
    if (error != 0) {
        report_error(error);
        return 0;
    }

    error = sb_file_size(handle, &size);
    if (error == 0 && size > 0) {
        count = size > SHOW_BYTES ? SHOW_BYTES : (unsigned int)size;
        error = sb_file_read(handle, 0, data, count);
    }
    sb_file_close(handle);
    if (error != 0) {
        report_error(error);
        return 0;
    }

    if (count == 0) {
        sb_puts("(EMPTY FILE)");
    }
    for (start = 0; start < count; start += LINE_CHARS) {
        n = count - start < LINE_CHARS ? count - start : LINE_CHARS;
        for (error = 0; error < n; ++error) {
            line[error] = data[start + error] < 0x20 ? '.' : data[start + error];
        }
        line[n] = 0;
        sb_puts(line);
    }

    sb_getkey();
    return 0;
}
