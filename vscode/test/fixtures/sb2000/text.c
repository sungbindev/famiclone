/*
 * ------------------------------------------------------------------------
 * 名称：text.c
 * 说明：测试 sb2000-text.h 里的文字输出函数：光标位置的变化，以及画面上字符是否真的画出来。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：画面核对靠脚本在 sb_test_sync 处截图比较：同样的字符画两次画面相同，换一个字符画面不同。
 * ------------------------------------------------------------------------
 */
#include <sb2000.h>
#include "sb-test.h"

/** 等几帧让画面刷新后停靠，脚本在这里截图 */
static void settle(void)
{
    sb_wait_frames(3);
    sb_test_sync();
}

int main(void)
{
    unsigned char column;
    unsigned char row;

    /* 停靠点 1：基准画面 */
    settle();

    /* 在第 10 列画 HELLO，光标移到第 15 列 */
    sb_cursor_set_column(10);
    sb_print("HELLO");
    SB_CHECK_EQ(sb_cursor_column(), 15);
    settle();

    /* 再画一次，画面不变 */
    sb_cursor_set_column(10);
    sb_print("HELLO");
    settle();

    /* 换一个字符，画面变化 */
    sb_cursor_set_column(10);
    sb_print("HELLP");
    settle();

    /* GB2312 汉字占 2 列 */
    sb_cursor_set_column(10);
    sb_print("\xC4\xE3\xBA\xC3");
    SB_CHECK_EQ(sb_cursor_column(), 14);
    settle();

    /* 列号越界不改 */
    sb_cursor_set_column(0);
    SB_CHECK_EQ(sb_cursor_column(), 14);
    sb_cursor_set_column(32);
    SB_CHECK_EQ(sb_cursor_column(), 14);
    sb_cursor_set_column(31);
    SB_CHECK_EQ(sb_cursor_column(), 31);

    /* 光标左右移动 */
    sb_cursor_set_column(5);
    sb_cursor_right();
    SB_CHECK_EQ(sb_cursor_column(), 6);
    sb_cursor_left();
    SB_CHECK_EQ(sb_cursor_column(), 5);

    /* 单个字符、十六进制、十进制都会移动光标 */
    sb_cursor_set_column(3);
    sb_putc('x');
    SB_CHECK_EQ(sb_cursor_column(), 4);
    sb_print_hex(0xA5);
    SB_CHECK_EQ(sb_cursor_column(), 6);
    sb_print_uint(12345);
    SB_CHECK_EQ(sb_cursor_column(), 11);
    sb_print_uint(0);
    SB_CHECK_EQ(sb_cursor_column(), 12);

    /* \r 回到行首，\n 换行 */
    sb_putc('\r');
    SB_CHECK_EQ(sb_cursor_column(), 1);
    sb_cursor_set_column(7);
    row = sb_cursor_row();
    sb_putc('\n');
    SB_CHECK_EQ(sb_cursor_column(), 1);
    SB_CHECK(sb_cursor_row() == row || sb_cursor_row() == row + (SB_BYTE_AT(SB_FONT_SIZE) ? 2 : 1));

    /* sb_puts 从行首显示并换行 */
    sb_cursor_set_column(9);
    sb_puts("puts");
    SB_CHECK_EQ(sb_cursor_column(), 1);

    /* 行首向左：移到上一行末尾（列 31） */
    sb_cursor_set_column(1);
    sb_cursor_left();
    column = sb_cursor_column();
    SB_CHECK_EQ(column, 31);

    /* 光标上移：列变成 31 */
    sb_cursor_up();
    SB_CHECK_EQ(sb_cursor_column(), 31);

    sb_beep();

    sb_test_done();
    return 0;
}
