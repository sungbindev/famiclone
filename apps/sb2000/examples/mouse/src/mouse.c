/*
 * ------------------------------------------------------------------------
 * 名称：mouse.c
 * 说明：测试鼠标服务的 SB-DOS 程序：复位鼠标、设置坐标范围与初始坐标，等 250 帧后取坐标并显示。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：在模拟器里等程序显示“MOVE THE MOUSE”后，用调试命令“mouse move dx dy”移动鼠标，“mouse btn 掩码”注入按键。
 *       显示格式：X xx Y xx（十六进制），初始坐标是 X=80、Y=40；没有检测到鼠标时显示 NO MOUSE。
 *       第二行 B 后面依次是两个键的状态、双击、单击、拖动共 8 个字节。
 * ------------------------------------------------------------------------
 */
#include <sb2000.h>

int main(void)
{
    unsigned char x;
    unsigned char y;
    unsigned char i;
    sb_mouse_buttons_t buttons;

    if (!sb_mouse_reset()) {
        sb_puts("NO MOUSE");
        sb_getkey();
        return 0;
    }
    /* 鼠标中断要先允许，再设范围与初始坐标 */
    sb_irq_enable(SB_IRQ_SOURCE_MOUSE);
    sb_mouse_set_limits(0, 0, 0xFF, 0xFF);
    sb_mouse_set_position(0x80, 0x40);

    sb_puts("MOVE THE MOUSE");
    sb_wait_frames(250);

    sb_mouse_position(&x, &y);
    sb_mouse_buttons(&buttons);
    sb_print("X ");
    sb_print_hex(x);
    sb_print(" Y ");
    sb_print_hex(y);
    sb_newline();

    sb_print("B");
    for (i = 0; i < 2; ++i) {
        sb_print(" ");
        sb_print_hex(buttons.state[i]);
    }
    for (i = 0; i < 2; ++i) {
        sb_print(" ");
        sb_print_hex(buttons.double_click[i]);
    }
    for (i = 0; i < 2; ++i) {
        sb_print(" ");
        sb_print_hex(buttons.click[i]);
    }
    for (i = 0; i < 2; ++i) {
        sb_print(" ");
        sb_print_hex(buttons.drag[i]);
    }
    sb_newline();

    sb_getkey();
    return 0;
}
