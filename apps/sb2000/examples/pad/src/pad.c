/*
 * ------------------------------------------------------------------------
 * 名称：pad.c
 * 说明：测试手柄服务的 SB-DOS 程序：初始化后等待手柄按键，显示键码与两个手柄的状态，再等键盘按键后返回。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：显示格式：KEY xx P1 xx P2 xx，三个值依次是键码、1 号手柄与 2 号手柄的原始状态（位 7 为 A，位 0 为右）。
 *       在模拟器里用调试命令“pad 0 01”按下 1 号手柄 A，松开用“pad 0 clear”。
 *       从键盘启动程序时，回车键在启动后的一小段时间里还按着，模拟器把它当成手柄的开始键，所以先等 120 帧再初始化。
 * ------------------------------------------------------------------------
 */
#include <sb2000.h>

int main(void)
{
    unsigned char key;

    sb_wait_frames(120);
    sb_pad_init();
    sb_puts("PRESS A PAD BUTTON");

    key = sb_pad_wait();
    sb_print("KEY ");
    sb_print_hex(key);
    sb_print(" P1 ");
    sb_print_hex(sb_pad_state(0));
    sb_print(" P2 ");
    sb_print_hex(sb_pad_state(1));
    sb_newline();

    sb_getkey();
    return 0;
}
