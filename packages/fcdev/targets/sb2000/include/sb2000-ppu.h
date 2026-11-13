/*
 * ------------------------------------------------------------------------
 * 名称：sb2000-ppu.h
 * 说明：直接操作图形端口（$2000-$2008）：显存读写、调色板、精灵 RAM、等垂直消隐、打开显示。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：由 sb2000.h 引入，程序不必单独包含。显存、调色板、精灵 RAM 只能在关着显示或垂直消隐期间访问，调用者先用 sb_display_off 关显示、
 *       用完用 sb_display_on 恢复；显存地址不小于 $8000 时是扩展显存，再加 $4300 选的 32K bank。$2000 的地址增量位要为 0。
 *       寄存器的位定义见 sb2000-symbols.h 里的 SB_HW_ 常量，行为见 docs/sb2000/03-硬件说明.md。
 * ------------------------------------------------------------------------
 */
#ifndef SB2000_PPU_H
#define SB2000_PPU_H

/**
 * 设置显存地址（$2006），之后读写 $2007 从这里开始。
 *
 * @param address 显存地址。
 */
void sb_ppu_address(unsigned int address);

/**
 * 把一段数据写进显存。
 *
 * @param address 显存起始地址。
 * @param data 数据。
 * @param count 字节数。
 */
void sb_ppu_write(unsigned int address, const unsigned char *data, unsigned int count);

/**
 * 把显存的一段填成同一个值。
 *
 * @param address 显存起始地址。
 * @param value 填充值。
 * @param count 字节数。
 */
void sb_ppu_fill(unsigned int address, unsigned char value, unsigned int count);

/**
 * 读显存的一个字节。设地址后第一次读只刷新缓冲，所以函数设两次地址，取第二次读到的值。
 *
 * @param address 显存地址。
 * @return 该地址的内容。
 */
unsigned char sb_ppu_peek(unsigned int address);

/**
 * 读一段显存。
 *
 * @param address 显存起始地址。
 * @param dest 存放数据的缓冲。
 * @param count 字节数。
 */
void sb_ppu_read(unsigned int address, unsigned char *dest, unsigned int count);

/**
 * 写调色板的一项（$2040 起共 64 项：背景 0-31，精灵 32-63）。
 *
 * @param index 项号，0 到 63。
 * @param color 颜色：低 4 位色相，位 5-4 亮度。
 */
void sb_ppu_palette_set(unsigned char index, unsigned char color);

/**
 * 读调色板的一项，只返回低 6 位（读出的位 6 恒为 1，已去掉）。
 *
 * @param index 项号，0 到 63。
 * @return 颜色。
 */
unsigned char sb_ppu_palette_get(unsigned char index);

/**
 * 把精灵 RAM 的 256 字节全部填成 $F0，使全部 64 个精灵都在屏幕之外。
 */
void sb_ppu_oam_clear(void);

/**
 * 写一个精灵（4 字节）。读回时属性的位 4-2 总是 1（真机实测）。
 *
 * @param index 精灵序号，0 到 63。
 * @param y 垂直位置，实际显示在 y + 1 行。
 * @param tile 图块号。
 * @param attr 属性：位 7 垂直翻转，位 6 水平镜像，位 5 在背景之后，位 1-0 调色板组。
 * @param x 水平位置。
 */
void sb_ppu_oam_put(unsigned char index, unsigned char y, unsigned char tile, unsigned char attr, unsigned char x);

/**
 * 等垂直消隐：轮询 $2002 直到位 7 为 1，最多读 65535 次。
 *
 * @return 读到垂直消隐时 $2002 的值；超时返回 0。
 */
unsigned char sb_ppu_wait_vblank(void);

/**
 * 设置控制寄存器与扩展控制寄存器、滚动回到 (0, 0)，最后写显示寄存器打开显示。
 *
 * @param mask 显示寄存器（$2001）的值，如 SB_HW_MASK_BG | SB_HW_MASK_SPRITE。
 * @param control 控制寄存器（$2000）的值，不要带允许 NMI 的位，除非 NMI 已经准备好。
 * @param ext 扩展控制寄存器（$2008）的值。
 */
void sb_ppu_render(unsigned char mask, unsigned char control, unsigned char ext);

#endif
