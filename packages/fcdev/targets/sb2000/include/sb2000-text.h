/*
 * ------------------------------------------------------------------------
 * 名称：sb2000-text.h
 * 说明：SB-DOS 程序的文字输出：在光标处显示字符串、字符、数字，移动光标，发提示音。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：由 sb2000.h 引入，程序不必单独包含。显示借用 ROM 里 DOS 的运行库，屏幕每行 32 列（列号从 1 起，可用 1-31），
 *       字符码不小于 $A1 的两个字节按 GB2312 显示一个汉字，占 2 列。
 *       字符串必须放在程序自己的窗口里（$8000-$BFFF），程序的起始页号要是 4 的倍数（DOS 从 $80 起分配，一般成立）；
 *       返回 DOS 后 DOS 会清屏，所以输出完要先等键再返回。
 * ------------------------------------------------------------------------
 */
#ifndef SB2000_TEXT_H
#define SB2000_TEXT_H

/**
 * 从行首显示零结尾字符串并换行。
 * 字符码不小于 0xA1 的两个字节按 GB2312 显示一个汉字。
 *
 * @param s 零结尾的字符串。
 */
void __fastcall__ sb_puts(const char *s);

/**
 * 回到行首并换行，行满时自动滚动屏幕。
 */
void sb_newline(void);

/**
 * 在光标处显示字符串，光标移到字符串之后，行满时自动换行。
 *
 * @param s 零结尾的字符串。
 */
void sb_print(const char *s);

/**
 * 在光标处显示一个字符，光标右移一格。换行符 \n 换行，回车符 \r 回到行首。
 *
 * @param c 字符。
 */
void sb_putc(char c);

/**
 * 在光标处显示一个字节的两位十六进制数（大写），光标移到它之后。
 *
 * @param value 要显示的字节。
 */
void sb_print_hex(unsigned char value);

/**
 * 在光标处显示一个无符号十进制数，光标移到它之后。
 *
 * @param value 要显示的数，0 到 65535。
 */
void sb_print_uint(unsigned int value);

/**
 * 光标左移一列，到行首时移到上一行的末尾。
 */
void sb_cursor_left(void);

/**
 * 光标右移一列，到第 32 列时回到第 1 列并换行。
 */
void sb_cursor_right(void);

/**
 * 光标移到上一行的末尾；已在第一行时不动。
 */
void sb_cursor_up(void);

/**
 * 设置光标所在的列。列号超出 1-31 时不改。
 *
 * @param column 列号，1 到 31。
 */
void sb_cursor_set_column(unsigned char column);

/**
 * 取光标所在的列。
 *
 * @return 列号，从 1 起。
 */
unsigned char sb_cursor_column(void);

/**
 * 取光标所在的显示行。
 *
 * @return 行号，从 0 起。
 */
unsigned char sb_cursor_row(void);

/**
 * 发一声提示音，用方波通道 2。
 */
void sb_beep(void);

#endif
