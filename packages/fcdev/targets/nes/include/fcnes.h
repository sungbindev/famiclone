/*
 * ------------------------------------------------------------------------
 * 名称：fcnes.h
 * 说明：fcdev 的 NES 辅助函数：画面开关、调色板、文字显示、等待垂直消隐。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-10
 * 备注：文字用 ASCII 字库（图块号等于字符码），背景图案表为 $0000；画面关闭时才能写显存。
 * ------------------------------------------------------------------------
 */
#ifndef FCNES_H
#define FCNES_H

/**
 * 关闭画面，之后可以写显存。
 */
void nes_screen_off(void);

/**
 * 打开背景显示，并把滚动位置复位到 (0, 0)。
 */
void nes_screen_on(void);

/**
 * 写入 32 字节调色板：前 16 字节为背景，后 16 字节为精灵。
 *
 * @param colors 32 字节的颜色表。
 */
void __fastcall__ nes_set_palette(const unsigned char *colors);

/**
 * 在名称表的指定位置写零结尾字符串，要先关闭画面。
 *
 * @param x 列，0 到 31。
 * @param y 行，0 到 29。
 * @param s 零结尾的字符串。
 */
void __fastcall__ nes_print(unsigned char x, unsigned char y, const char *s);

/**
 * 等待下一次垂直消隐开始。
 */
void nes_wait_vblank(void);

#endif
