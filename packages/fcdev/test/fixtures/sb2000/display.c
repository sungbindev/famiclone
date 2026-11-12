/*
 * ------------------------------------------------------------------------
 * 名称：display.c
 * 说明：测试 sb2000-display.h 里的显示函数：屏幕单元的填充与复制、清屏、单元地址、文字、字模地址、窗口保存还原、调色板、显存 DMA、图片解压。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：只用名称表 2、3（DOS 画面不显示它们），结果从屏幕缓冲（扩展 RAM 页 $FB）里读回核对；写进显存的函数只核对能返回与参数换算。
 *       显存里写图块用的图块号都在 $0300 以后，不碰 DOS 文字区。
 * ------------------------------------------------------------------------
 */
#include <string.h>
#include <sb2000.h>
#include "sb-test.h"

/** 读屏幕缓冲里一个单元的值 */
static unsigned int cell_at(unsigned char table, unsigned char col, unsigned char row)
{
    unsigned char page;
    unsigned int offset;
    unsigned char bytes[2];

    sb_cell_address(table, col, row, &page, &offset);
    sb_far_read(page, offset, bytes, 2);
    return (unsigned int)bytes[0] | ((unsigned int)bytes[1] << 8);
}

/** 向屏幕缓冲里一个单元写一个值，不刷新 */
static void set_cell(unsigned char table, unsigned char col, unsigned char row, unsigned int value)
{
    unsigned char page;
    unsigned int offset;
    unsigned char bytes[2];

    bytes[0] = (unsigned char)value;
    bytes[1] = (unsigned char)(value >> 8);
    sb_cell_address(table, col, row, &page, &offset);
    sb_far_write(page, offset, bytes, 2);
}

/** 图案流：16 字节字典（全 0）加指令“2 个原样字节”与结束 */
static const unsigned char tiles[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0xD2, 0xAA, 0x55, 0x00
};
/** 解压到内存用的图案流：3 个原样字节 11 22 33 */
static const unsigned char to_memory[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0xD3, 0x11, 0x22, 0x33, 0x00
};
/** 映射流：填 1 个单元，值 5 */
static const unsigned char map_stream[] = { 0x01, 0x05, 0x00, 0x00 };
/** 一张最小的图片：10 字节头（宽 1、高 1、图案数 1、调色板 1 组、每组 4 色、映射偏移 29、调色板偏移 33），图案流，映射流（填 1 个单元，值 7），调色板 4 字节 */
static const unsigned char image[] = {
    1, 1, 1, 0, 1, 4, 29, 0, 33, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0xD1, 0xFF, 0x00,
    0x01, 0x07, 0x00, 0x00,
    0x0F, 0x16, 0x26, 0x30
};
/** 调色板颜色 */
static const unsigned char colors[] = { 0x0F, 0x01, 0x11, 0x21 };
/** 要复制到屏幕的单元 */
static const unsigned int block[] = { 1, 2, 3, 4, 5, 6 };
/** 直接装入的图案 */
static const unsigned char raw[] = { 1, 2, 3, 4, 5, 6, 7, 8 };

int main(void)
{
    unsigned char page;
    unsigned int offset;
    unsigned char shape;
    unsigned char memory_page;
    unsigned char bytes[4];
    unsigned char id;
    unsigned char ok;
    unsigned char col;
    unsigned char row;
    sb_map_t map;

    /* 单元地址：名称表 2 在页 $FB 偏移 0，名称表 3 在页 $FB 偏移 $800；每行 64 字节，每个单元 2 字节 */
    sb_cell_address(3, 5, 7, &page, &offset);
    SB_CHECK(page == 0xFB && offset == 0x9CA);
    sb_cell_address(2, 1, 1, &page, &offset);
    SB_CHECK(page == 0xFB && offset == 0x42);

    /* 线性填充：调色板号替换单元值的高 4 位，两侧的单元不变 */
    set_cell(2, 7, 3, 0x1111);
    sb_cells_fill(2, 2, 3, 5, 0x0123, 5);
    ok = 1;
    for (col = 2; col < 7; ++col) {
        if (cell_at(2, col, 3) != 0x5123) {
            ok = 0;
        }
    }
    SB_CHECK(ok);
    SB_CHECK_EQ(cell_at(2, 7, 3), 0x1111);

    /* 矩形填充 */
    sb_cells_rect(2, 10, 10, 4, 3, 0x0200, 1);
    ok = 1;
    for (row = 10; row < 13; ++row) {
        for (col = 10; col < 14; ++col) {
            if (cell_at(2, col, row) != 0x1200) {
                ok = 0;
            }
        }
    }
    SB_CHECK(ok);
    SB_CHECK(cell_at(2, 14, 10) != 0x1200 && cell_at(2, 10, 13) != 0x1200);

    /* 复制一块单元：3 列 2 行，单元值原样写入 */
    sb_cells_copy(2, 20, 20, 3, 2, block);
    SB_CHECK_EQ(cell_at(2, 20, 20), 1);
    SB_CHECK_EQ(cell_at(2, 22, 20), 3);
    SB_CHECK_EQ(cell_at(2, 20, 21), 4);
    SB_CHECK_EQ(cell_at(2, 22, 21), 6);

    /* 窗口：保存矩形，改写，还原；坐标不合法被拒绝 */
    sb_window_init();
    id = sb_window_save(2, 2, 3, 6, 3);
    SB_CHECK_EQ(id, 1);
    sb_cells_fill(2, 2, 3, 5, 0x0999, 0);
    SB_CHECK_EQ(cell_at(2, 4, 3), 0x0999);
    sb_window_restore(id);
    SB_CHECK_EQ(cell_at(2, 4, 3), 0x5123);
    SB_CHECK_EQ(sb_window_save(2, 5, 5, 4, 4), SB_WINDOW_BAD_RECT);
    sb_window_compact();
    sb_window_init();

    /* 清屏缓冲：名称表 2 全部写成同一个值，名称表 3 不变 */
    set_cell(3, 0, 0, 0x7777);
    sb_screen_clear(0x04, 0x00A0);
    SB_CHECK(cell_at(2, 0, 0) == 0x00A0 && cell_at(2, 31, 29) == 0x00A0);
    SB_CHECK_EQ(cell_at(3, 0, 0), 0x7777);
    sb_screen_flush(2);

    /* 文字：写一行西文，单元值从起始图块号起每列加 2（大字体）；画字符串到显存后图块号被换算成字节地址 */
    sb_text_field_ascii(3, 4, 5, "AB", 0x0300, 0, 15);
    SB_CHECK_EQ(cell_at(3, 4, 5), 0x0300);
    SB_CHECK_EQ(cell_at(3, 5, 5), 0x0302);
    sb_text_field(3, 4, 8, "AB", 0x0310, 0, 15);
    SB_CHECK_EQ(cell_at(3, 4, 8), 0x0310);
    sb_text_draw(0x0320, "OK", 0, 15, 0, 1);
    SB_CHECK_EQ((unsigned int)SB_BYTE_AT(SB_PARAM_064C) | ((unsigned int)SB_BYTE_AT(SB_PARAM_064D) << 8), 0x3200);

    /* 画到内存：一个西文字符二平面 8×8 是 16 字节、大字体是 32 字节；容量不够容纳下一个字符时在它之前停止（容量 10 写 0 字节） */
    SB_CHECK_EQ(sb_page_alloc(2, &memory_page), 0);
    offset = sb_text_to_memory("A", 0, memory_page, 0x8000, 200, 0, 15, 1);
    SB_CHECK(offset == 16 || offset == 32);
    SB_CHECK_EQ(sb_text_to_memory("A", 0, memory_page, 0x8000, 10, 0, 15, 1), 0);
    sb_page_free(memory_page, 2);

    /* 显示模式：高 2 位随之改变，低 6 位不变，最后恢复 */
    shape = SB_BYTE_AT(SB_DISPLAY_MODE);
    sb_display_mode(2);
    SB_CHECK_EQ(SB_BYTE_AT(SB_DISPLAY_MODE), (unsigned char)((shape & 0x3F) | 0x80));
    sb_display_mode(shape >> 6);
    SB_CHECK_EQ(SB_BYTE_AT(SB_DISPLAY_MODE), shape);

    /* 字模地址：汉字“啊”（$B0A1）在 ROM $1C000，即页 $1C 偏移 0；西文字模在页 $50 或 $51；不合法的字符码被拒绝 */
    SB_CHECK_EQ(sb_glyph_address(0xB0A1, &page, &offset, &shape), 0);
    SB_CHECK(page == 0x1C && offset == 0);
    SB_CHECK_EQ(sb_glyph_address(0x0041, &page, &offset, &shape), 0);
    SB_CHECK(page == 0x50 || page == 0x51);
    SB_CHECK_EQ(sb_glyph_address(0xAAA1, &page, &offset, &shape), 0xFF);

    /* 调色板、显存填充与装入、显存 bank：都能返回 */
    sb_palette_write(0, colors, 4);
    sb_vram_fill(0x9000, 16, 0);
    SB_CHECK(sb_page_alloc(1, &memory_page) == 0);
    sb_vram_load(memory_page, 0, 0x9000, 16);
    sb_page_free(memory_page, 1);
    SB_CHECK(1);

    /* 图片：直接装入图案、解压图案流到显存、解压映射流到名称表 3 的 (0,0)、按效果铺满矩形 */
    sb_image_load_raw(raw, sizeof(raw), 0x0330);
    sb_image_unpack_tiles(tiles, 0x0334);
    map.table = 3;
    map.col = 0;
    map.row = 20;
    map.width = 1;
    map.height = 1;
    map.effect = 0;
    map.speed = 0;
    map.palette = 3;
    map.base = 0x0100;
    sb_image_unpack_map(map_stream, &map);
    SB_CHECK_EQ(cell_at(3, 0, 20), 0x3105);
    map.col = 1;
    map.row = 25;
    map.width = 2;
    map.height = 2;
    map.base = 0x0150;
    sb_image_fill_rect(&map);
    SB_CHECK(cell_at(3, 1, 25) == 0x0150 && cell_at(3, 2, 26) == 0x0150);

    /* 压缩图片：显示到 (3,3)，映射值 7 加图块号；整屏显示把映射流解到名称表缓冲开头 */
    map.col = 3;
    map.row = 3;
    map.width = 1;
    map.height = 1;
    map.base = 0x0340;
    map.palette = 0;
    sb_image_show(image, &map, 0);
    SB_CHECK_EQ(cell_at(3, 3, 3), 0x0347);
    sb_image_show_full(image, 0x0350, 3);
    SB_CHECK_EQ(cell_at(3, 0, 0), 0x0357);

    /* 解压到内存：3 个字节解到申请来的页的开头，返回字节数 */
    SB_CHECK_EQ(sb_page_alloc(2, &memory_page), 0);
    SB_CHECK_EQ(sb_image_unpack_to_memory(to_memory, memory_page, 0x8000), 3);
    sb_far_read(memory_page, 0, bytes, 3);
    SB_CHECK(bytes[0] == 0x11 && bytes[1] == 0x22 && bytes[2] == 0x33);
    sb_page_free(memory_page, 2);

    sb_vram_bank(0);
    SB_CHECK(1);

    sb_test_done();
    return 0;
}
