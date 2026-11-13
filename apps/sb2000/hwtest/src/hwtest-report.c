/*
 * ------------------------------------------------------------------------
 * 名称：hwtest-report.c
 * 说明：报告文件：把测试结果写成文字，存成当前目录下的 HWTEST.TXT（运行时软盘就是当前目录），不在屏幕上显示。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：文字先攒在一个小行缓冲里，满了就写进 report_open 申请的扩展 RAM 页，report_close 再整个写成文件并释放页；
 *       行尾用 CR LF，DOS 的 type 与记事本都能直接看。不能在关着 NMI、关着中断时调用 report_open 与 report_close（要调 DOS 服务）。
 * ------------------------------------------------------------------------
 */
#include "hwtest.h"

#define REPORT_FILE "HWTEST.TXT"
#define REPORT_PAGES 2
#define REPORT_CAPACITY 0x2000u
#define LINE_BYTES 48

#define ERROR_OVERFLOW 0xFE
#define ERROR_NO_MEMORY 0xFF

static unsigned char report_page;
static unsigned char report_ready;
static unsigned char report_overflow;
static unsigned int report_length;
static char *const line_buffer = ERAM_LINE;
static unsigned char line_length;

/* 申请存放报告的页，成功返回 1 */
unsigned char report_open(void)
{
    if (sb_page_alloc(REPORT_PAGES, &report_page) != 0) {
        return 0;
    }
    report_ready = 1;
    report_overflow = 0;
    report_length = 0;
    line_length = 0;
    return 1;
}

/* 把行缓冲里的文字写进页；超过容量的部分丢掉并记下 */
static void flush(void)
{
    if (line_length == 0) {
        return;
    }
    if (report_length + line_length > REPORT_CAPACITY) {
        report_overflow = 1;
    } else {
        sb_far_write(report_page, report_length, line_buffer, line_length);
        report_length += line_length;
    }
    line_length = 0;
}

static void put_char(char value)
{
    if (line_length == LINE_BYTES) {
        flush();
    }
    line_buffer[line_length++] = value;
}

void report_text(const char *text)
{
    while (*text != 0) {
        put_char(*text++);
    }
}

void report_newline(void)
{
    put_char('\r');
    put_char('\n');
}

void report_line(const char *text)
{
    report_text(text);
    report_newline();
}

/* 两位十六进制，大写 */
void report_hex(unsigned char value)
{
    static const char digits[] = "0123456789ABCDEF";

    put_char(digits[value >> 4]);
    put_char(digits[value & 15]);
}

void report_uint(unsigned int value)
{
    char digits[5];
    unsigned char count = 0;

    do {
        digits[count++] = (char)('0' + value % 10);
        value /= 10;
    } while (value != 0);
    while (count != 0) {
        put_char(digits[--count]);
    }
}

/* 标签后面跟若干字节的十六进制，字节之间空一格，最后换行 */
void report_bytes(const char *label, const unsigned char *bytes, unsigned char count)
{
    unsigned char i;

    report_text(label);
    for (i = 0; i < count; ++i) {
        report_hex(bytes[i]);
        if (i + 1 < count) {
            put_char(' ');
        }
    }
    report_newline();
}

/* 把报告写成文件并释放页，返回 DOS 出错码（0 成功）；文字超过容量时返回 ERROR_OVERFLOW（文件仍然写出，内容被截断）。length 返回写出的字节数 */
unsigned char report_close(unsigned int *length)
{
    unsigned char code;

    if (!report_ready) {
        *length = 0;
        return ERROR_NO_MEMORY;
    }
    flush();
    code = sb_file_save(eram_text(REPORT_FILE), report_page, report_length);
    *length = report_length;
    sb_page_free(report_page, REPORT_PAGES);
    report_ready = 0;
    if (code == 0 && report_overflow) {
        return ERROR_OVERFLOW;
    }
    return code;
}
