/*
 * ------------------------------------------------------------------------
 * 名称：input.c
 * 说明：测试 sb2000-input.h 里的键盘、手柄、鼠标函数。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：按键、手柄、鼠标的动作由脚本在 sb_test_sync 处注入；每个停靠点之后程序先等足够的帧，让系统读到输入。
 * ------------------------------------------------------------------------
 */
#include <sb2000.h>
#include "sb-test.h"

/** 先等几帧再停靠：脚本在前一个停靠点注入的动作要跑完它自己的帧，程序不能抢在前面到达下一个停靠点 */
static void stop_here(void)
{
    sb_wait_frames(30);
    sb_test_sync();
}

/** 按 ESC 中断后的落点：新的执行起点，不返回 */
static void esc_landing(void)
{
    SB_CHECK_EQ(SB_BYTE_AT(SB_CONTEXT_COUNT), 0xFF);
    sb_break_disable();
    SB_CHECK_EQ(SB_BYTE_AT(SB_ESC_BREAK), 0);
    sb_wait_frames(30);
    sb_test_done();
    for (;;) {
    }
}

int main(void)
{
    unsigned int key;
    unsigned char x;
    unsigned char y;
    sb_mouse_buttons_t buttons;
    unsigned char program_page = SB_BYTE_AT(SB_PROGRAM_START_PAGE);

    /* 键盘：停靠点 1，脚本依次按 a、b */
    sb_key_flush();
    stop_here();
    sb_wait_frames(40);
    SB_CHECK_EQ(sb_key_count(), 2);
    key = sb_key_poll();
    SB_CHECK_EQ(key & 0xFF, 'a');
    SB_CHECK_EQ(key >> 8, 0x1E);
    key = sb_key_wait();
    SB_CHECK(((key & 0xFF) == 'b') && (key >> 8) == 0x30);
    SB_CHECK_EQ(sb_key_poll(), 0);

    /* 停靠点 2：脚本按 c，sb_getkey 等到它 */
    stop_here();
    SB_CHECK_EQ(sb_getkey(), 'c');

    /* 停靠点 3：按 3 个键后清空队列 */
    stop_here();
    sb_wait_frames(40);
    SB_CHECK_EQ(sb_key_count(), 3);
    sb_key_flush();
    SB_CHECK_EQ(sb_key_count(), 0);

    /* 停靠点 4：功能键 F1 没有字符，扫描码 $3B；修饰键状态与系统变量一致 */
    stop_here();
    key = sb_key_wait();
    SB_CHECK(((key & 0xFF) == 0) && (key >> 8) == 0x3B);
    SB_CHECK_EQ(sb_key_modifiers(), SB_BYTE_AT(SB_KBD_FLAGS));

    /* 手柄：初始化后键码单元是 $80；停靠点 5 按住 A */
    sb_pad_init();
    SB_CHECK_EQ(SB_BYTE_AT(SB_PAD_KEY), 0x80);
    stop_here();
    sb_wait_frames(3);
    SB_CHECK_EQ(sb_pad_poll(), 0x0F);
    SB_CHECK_EQ(sb_pad_state(0), 0x80);

    /* 停靠点 6：松开 */
    stop_here();
    sb_wait_frames(3);
    SB_CHECK_EQ(sb_pad_poll(), 0xFF);

    /* 停靠点 7：程序先进入等待，脚本再按 B */
    stop_here();
    SB_CHECK_EQ(sb_pad_wait(), 0x0E);

    /* 鼠标：复位、允许中断、设置范围与坐标 */
    SB_CHECK(sb_mouse_reset() != 0);
    sb_irq_enable(SB_IRQ_SOURCE_MOUSE);
    sb_mouse_set_limits(0, 0, 200, 150);
    sb_mouse_set_position(100, 80);
    sb_mouse_position(&x, &y);
    SB_CHECK(x == 100 && y == 80);

    /* 停靠点 8：脚本移动鼠标 (10, 5)，坐标按 1:1 累加 */
    stop_here();
    sb_wait_frames(40);
    sb_mouse_position(&x, &y);
    SB_CHECK(x == 110 && y == 85);

    /* 停靠点 9：按住按键 1 */
    stop_here();
    sb_wait_frames(40);
    sb_mouse_buttons(&buttons);
    SB_CHECK_EQ(buttons.state[0], 1);

    /* 停靠点 10：松开，状态 2，并产生一次单击 */
    stop_here();
    sb_wait_frames(40);
    sb_mouse_buttons(&buttons);
    SB_CHECK(buttons.state[0] == 2 && buttons.click[0] == 1);

    /* 双击间隔：设置后保存，重设范围不改它 */
    sb_mouse_set_double_click(0x55);
    SB_CHECK_EQ(SB_BYTE_AT(SB_MOUSE_PARAM), 0x55);
    sb_mouse_set_limits(0, 0, 250, 200);
    SB_CHECK_EQ(SB_BYTE_AT(SB_MOUSE_PARAM), 0x55);

    /* 按 ESC 中断：保存恢复点后打开，停靠点 11 脚本按 ESC，程序在等待中被带到落点 */
    sb_context_save(program_page, (unsigned int)esc_landing, sb_stack_pointer());
    sb_break_enable();
    SB_CHECK_EQ(SB_BYTE_AT(SB_ESC_BREAK), 0xFF);
    stop_here();
    sb_wait_frames(200);

    /* 不会执行到这里 */
    SB_CHECK(0);
    sb_test_done();
    return 0;
}
