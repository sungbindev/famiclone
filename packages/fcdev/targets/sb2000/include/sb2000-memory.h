/*
 * ------------------------------------------------------------------------
 * 名称：sb2000-memory.h
 * 说明：SB-DOS 程序的内存服务：扩展 RAM 页的分配、跨页读写、窗口映射、窗口地址换算。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：由 sb2000.h 引入，程序不必单独包含。
 *       程序装在 $8000-$BFFF 的 4 个窗口里（每窗口 4K，对应扩展 RAM 的一页）；扩展 RAM 的页号从 $80 起，
 *       读文件、放图片、放大块数据都在扩展 RAM 页里，CPU 不能直接访问，用 sb_far_read、sb_far_write 读写。
 *       本文件的函数都假定调用者的数据在程序窗口 $8000-$BFFF 或 $6000-$7FFF 内；
 *       窗口 3（$B000-$BFFF）里有程序的软件栈，不要把别的页映射给它。
 * ------------------------------------------------------------------------
 */
#ifndef SB2000_MEMORY_H
#define SB2000_MEMORY_H

/**
 * 分配连续的扩展 RAM 页。页数为 0 时不分配，只返回第一个空闲页的页号。
 *
 * @syscall $7E/$00
 * @param count 页数，每页 4K。
 * @param first_page 返回分配到的第一个页号；失败时为 0。
 * @return 0 成功；SB_ERR_NO_MEMORY 没有这么多连续的空闲页。
 */
unsigned char sb_page_alloc(unsigned char count, unsigned char *first_page);

/**
 * 释放连续的扩展 RAM 页。
 *
 * @syscall $7E/$01
 * @param first_page 第一个页号，必须不小于 $80 且小于 $F0。
 * @param count 页数；这个服务不检查页号加页数是否越过 $F0。
 * @return 0 成功；SB_ERR_BAD_ADDRESS 起始页号不合法，没有释放任何页。
 */
unsigned char sb_page_free(unsigned char first_page, unsigned char count);

/**
 * 求最长的连续空闲页数。
 *
 * @syscall $7E/$02
 * @return 页数；计数是 8 位，256 页全部空闲时回绕成 0。
 */
unsigned char sb_page_largest_free(void);

/**
 * 读扩展 RAM 页里的一个字节。
 *
 * @param page 页号。
 * @param offset 页内偏移，超过 $0FFF 时折算到后面的页。
 * @return 读到的字节。
 */
unsigned char sb_far_read_byte(unsigned char page, unsigned int offset);

/**
 * 写扩展 RAM 页里的一个字节。
 *
 * @param page 页号。
 * @param offset 页内偏移，超过 $0FFF 时折算到后面的页。
 * @param value 要写的字节。
 */
void sb_far_write_byte(unsigned char page, unsigned int offset, unsigned char value);

/**
 * 把扩展 RAM 里的一段数据读到内存，可以跨页。数据经 $0700 缓冲中转，调用期间会改写 $0700-$07FF。
 *
 * @param page 起始页号。
 * @param offset 页内偏移，超过 $0FFF 时折算到后面的页。
 * @param dest 目的地，必须在程序窗口的前 3 个窗口（$8000-$AFFF）、$6000-$7FFF 或内核 RAM 里，不能在窗口 3。
 * @param length 字节数。
 */
void sb_far_read(unsigned char page, unsigned int offset, void *dest, unsigned int length);

/**
 * 把内存里的一段数据写到扩展 RAM，可以跨页。数据经 $0700 缓冲中转，调用期间会改写 $0700-$07FF。
 *
 * @param page 起始页号。
 * @param offset 页内偏移，超过 $0FFF 时折算到后面的页。
 * @param src 数据来源，位置的要求同 sb_far_read 的 dest。
 * @param length 字节数。
 */
void sb_far_write(unsigned char page, unsigned int offset, const void *src, unsigned int length);

/**
 * 把窗口地址换算成远指针：页号 = 地址高 4 位 - 8 + 页组首页号（按 8 位回绕），页内偏移是地址的低 12 位。
 *
 * @syscall $2C
 * @param group 页组的首页号。
 * @param address 窗口地址，$8000-$FFFF。
 * @param page 返回页号。
 * @param offset 返回页内偏移。
 */
void sb_address_to_far(unsigned char group, unsigned int address, unsigned char *page, unsigned int *offset);

/**
 * 把一个扩展 RAM 页映射到一个窗口。调用返回后映射保持，直到再次映射。
 * 窗口 0-2 可以用来临时访问别的页；程序自己占用的窗口被换走后，里面的代码与数据在恢复前看不到。
 * 窗口 3 里有程序的软件栈，不要映射。
 *
 * @syscall $29
 * @param window 窗口号 0-3，0 是 $8000，1 是 $9000，2 是 $A000，3 是 $B000。
 * @param page 页号。
 */
void sb_window_map(unsigned char window, unsigned char page);

/**
 * 把页组的 4 个连续页映射到 4 个窗口（$8000-$BFFF），调用返回后映射保持。
 * 调用者本身的代码所在的窗口也会换掉，所以新页组里同一地址要有同样的代码。
 *
 * @syscall $27
 * @param group 页组的首页号。
 */
void sb_window_map_group(unsigned char group);

#endif
