/*
 * ------------------------------------------------------------------------
 * 名称：hello.c
 * 说明：NES 最小程序：设置调色板，在屏幕中间显示 HELLO, NES!。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-10
 * 备注：文字用 fcdev 附带的 ASCII 字库；写显存前要先关闭画面，写完再打开。
 * ------------------------------------------------------------------------
 */
#include <fcnes.h>

/* 32 字节调色板：每组 4 色，颜色 0 是背景（黑），颜色 1 是文字（白） */
static const unsigned char palette[32] = {
    0x0F, 0x30, 0x10, 0x00, 0x0F, 0x30, 0x10, 0x00,
    0x0F, 0x30, 0x10, 0x00, 0x0F, 0x30, 0x10, 0x00,
    0x0F, 0x30, 0x10, 0x00, 0x0F, 0x30, 0x10, 0x00,
    0x0F, 0x30, 0x10, 0x00, 0x0F, 0x30, 0x10, 0x00
};

int main(void)
{
    nes_screen_off();
    nes_set_palette(palette);
    nes_print(10, 14, "HELLO, NES!");
    nes_wait_vblank();
    nes_screen_on();

    /* NES 程序没有“返回”，主循环一直运行 */
    for (;;) {
    }
}
