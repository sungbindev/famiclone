/*
 * ------------------------------------------------------------------------
 * 名称：poke.c
 * 说明：按偏移写文件的 SB-DOS 程序：在 DOS 下输入“poke 文件名 十六进制偏移 文字”，把文字（大写）写进已有文件的该偏移处。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：演示按偏移写入 sb_file_write：以 SB_MODE_UPDATE 打开已有文件，写入范围不能超出文件当前大小（否则出错码 0x14），
 *       关闭时才把改动写回目录项。文字最长 60 个字符；出错时先关闭文件再显示“ERROR 码”。
 * ------------------------------------------------------------------------
 */
#include <sb2000.h>

/** 文字的最大长度 */
#define TEXT_MAX 60

static char path[29];
static char offset_text[9];
static char text[TEXT_MAX + 1];

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
static unsigned long parse_hex(const char *digits)
{
    unsigned long value = 0;
    unsigned char digit;

    for (; *digits != 0; ++digits) {
        if (*digits >= '0' && *digits <= '9') {
            digit = *digits - '0';
        } else if (*digits >= 'A' && *digits <= 'F') {
            digit = *digits - 'A' + 10;
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
    unsigned char length = 0;

    if (!next_word(&tail, path, sizeof(path)) || !next_word(&tail, offset_text, sizeof(offset_text))) {
        sb_puts("USAGE: POKE FILE HEXOFF TEXT");
        sb_getkey();
        return 0;
    }
    while (*tail == ' ') {
        ++tail;
    }
    while (*tail != 0 && length < TEXT_MAX) {
        text[length++] = *tail++;
    }
    text[length] = 0;
    if (length == 0) {
        sb_puts("USAGE: POKE FILE HEXOFF TEXT");
        sb_getkey();
        return 0;
    }

    error = sb_file_open(path, SB_MODE_UPDATE, &handle);
    if (error == 0) {
        error = sb_file_write(handle, parse_hex(offset_text), text, length);
        sb_file_close(handle);
    }

    if (error == 0) {
        sb_puts("POKE OK");
    } else {
        sb_cursor_set_column(1);
        sb_print("ERROR ");
        sb_print_hex(error);
        sb_newline();
    }
    sb_getkey();
    return 0;
}
