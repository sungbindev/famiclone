/*
 * ------------------------------------------------------------------------
 * 名称：peek.c
 * 说明：按偏移读文件的 SB-DOS 程序：在 DOS 下输入“peek 文件名 十六进制偏移”，显示文件里从该偏移起的 30 个字节。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：演示按偏移读取 sb_file_read：偏移不小于文件大小、或偏移加长度超过文件大小时出错码为 SB_ERR_BAD_POSITION（0x14）。
 *       省略偏移时从 0 开始；控制字符显示成“.”；出错时先关闭文件再显示“ERROR 码”。
 * ------------------------------------------------------------------------
 */
#include <sb2000.h>

/** 显示的字节数 */
#define SHOW_BYTES 30

static char path[29];
static char offset_text[9];
static unsigned char data[SHOW_BYTES];
static char line[SHOW_BYTES + 1];

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

/** 解析大写十六进制数，遇到非十六进制字符停止 */
static unsigned long parse_hex(const char *text)
{
    unsigned long value = 0;
    unsigned char digit;

    for (; *text != 0; ++text) {
        if (*text >= '0' && *text <= '9') {
            digit = *text - '0';
        } else if (*text >= 'A' && *text <= 'F') {
            digit = *text - 'A' + 10;
        } else {
            break;
        }
        value = (value << 4) | digit;
    }
    return value;
}

int main(void)
{
    const char *tail = (const char *)SB_CMD_TAIL;
    unsigned char handle;
    unsigned char error;
    unsigned char i;
    unsigned long offset = 0;

    if (!next_word(&tail, path, sizeof(path))) {
        sb_puts("USAGE: PEEK FILE HEXOFFSET");
        sb_getkey();
        return 0;
    }
    if (next_word(&tail, offset_text, sizeof(offset_text))) {
        offset = parse_hex(offset_text);
    }

    error = sb_file_open(path, SB_MODE_OPEN, &handle);
    if (error != 0) {
        sb_cursor_set_column(1);
        sb_print("ERROR ");
        sb_print_hex(error);
        sb_newline();
        sb_getkey();
        return 0;
    }
    error = sb_file_read(handle, offset, data, SHOW_BYTES);
    sb_file_close(handle);

    if (error != 0) {
        sb_cursor_set_column(1);
        sb_print("ERROR ");
        sb_print_hex(error);
        sb_newline();
    } else {
        for (i = 0; i < SHOW_BYTES; ++i) {
            line[i] = data[i] < 0x20 ? '.' : data[i];
        }
        line[SHOW_BYTES] = 0;
        sb_puts(line);
    }
    sb_getkey();
    return 0;
}
