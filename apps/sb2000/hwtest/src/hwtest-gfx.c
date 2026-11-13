/*
 * ------------------------------------------------------------------------
 * 名称：hwtest-gfx.c
 * 说明：图形测试：$2002 位 6、5 的标志（自动判定），以及一组需要用眼睛看的场景：精灵属性、$2001 各位、背景页、$2008 位 0、
 *       8×16 精灵的图块号、64 项调色板、16 色模式。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：测试图块放在扩展显存 bank 15（$4300=15，图块号 $0C00 起，显存地址 $C000 起），标签文字图块在同一 bank 的 $8000-$BBFF；
 *       精灵图案放在显存 $2400-$24FF（DOS 的精灵图案区 $2000 起，用 $2008 位 3-2 选择，这里用 $2400 起的 64-71 号精灵图块，
 *       用前保存整个 $2400-$27FF，结束时恢复）；背景名称表用到各页的第 24-27 行（用前保存，结束时恢复），$2008 位 0 的场景
 *       还会用到显存 $2600 起的名称表。调色板只改 BG 第 15 组与精灵第 0 组（V7、V8 会改全部 BG 调色板，需要重启 DOS）。
 *       场景在关 NMI 的状态下显示，按键由 DOS 的键盘中断放进队列，用 sb_key_poll 取；场景里不要按 CapsLock、NumLock 等锁定键。
 *       真机的 DOS 默认用四平面（$2008 位 7 为 1，影子 $062E 为 $88）；场景的图块都是二平面，所以场景期间把影子值与 $2008 的位 7 清 0，
 *       结束时恢复；只有 V8 把位 7 置 1 来看 16 色模式。
 * ------------------------------------------------------------------------
 */
#include "hwtest.h"

/* 扩展显存里的图块：图块号 * 16 = 显存地址 */
#define TILE_SOLID1 0x0C00u
#define TILE_SOLID2 0x0C01u
#define TILE_SOLID3 0x0C02u
#define TILE_STRIPES 0x0C03u
#define TILE_BLANK 0x0C08u
#define TILE_COLOR16 0x0C20u /* 16 色图块，每块 32 字节，图块号间隔 2 */
#define ADDRESS_SOLID 0xC000u
#define ADDRESS_BLANK 0xC080u
#define ADDRESS_COLOR16 0xC200u
#define LABEL_BASE 0x0800u /* 标签文字图块：行号 / 2 * 64 + LABEL_BASE */
#define PALETTE_TEXT 15

/* 精灵图块：号码 64-71，显存地址 $2000 + 号码 * 16 */
#define SPRITE_FIRST 64
#define SPRITE_COUNT 8
#define SPRITE_ADDRESS 0x2400u
#define SPRITE_F 64
#define SPRITE_SOLID 65
#define SPRITE_A 66

/* 要保存与恢复的区域 */
#define SAVE_SPRITE_BYTES 1024
#define SAVE_NAME_BYTES 256
#define SAVE_NAME_OFFSET 0x0600u /* 第 24 行：24 * 64 */

static const unsigned char bg_tiles[4][16] = {
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F }
};

/* 精灵图案，每个 16 字节：前 8 字节是第 0 位平面（颜色 1），后 8 字节是第 1 位平面（全 0） */
static const unsigned char sprite_tiles[SPRITE_COUNT][16] = {
    { 0x7E, 0x60, 0x60, 0x7C, 0x60, 0x60, 0x60, 0x00, 0, 0, 0, 0, 0, 0, 0, 0 }, /* 64 F */
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0, 0, 0, 0, 0, 0, 0, 0 }, /* 65 实心 */
    { 0x3C, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00, 0, 0, 0, 0, 0, 0, 0, 0 }, /* 66 A */
    { 0x7C, 0x66, 0x66, 0x7C, 0x66, 0x66, 0x7C, 0x00, 0, 0, 0, 0, 0, 0, 0, 0 }, /* 67 B */
    { 0x3C, 0x66, 0x60, 0x60, 0x60, 0x66, 0x3C, 0x00, 0, 0, 0, 0, 0, 0, 0, 0 }, /* 68 C */
    { 0x78, 0x6C, 0x66, 0x66, 0x66, 0x6C, 0x78, 0x00, 0, 0, 0, 0, 0, 0, 0, 0 }, /* 69 D */
    { 0x7E, 0x60, 0x60, 0x7C, 0x60, 0x60, 0x7E, 0x00, 0, 0, 0, 0, 0, 0, 0, 0 }, /* 70 E */
    { 0x66, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00, 0, 0, 0, 0, 0, 0, 0, 0 }  /* 71 H */
};

static const unsigned char bg_colors[4] = { 0x0E, 0x30, 0x36, 0x3A };
static const unsigned char sprite_colors[4] = { 0x0E, 0x38, 0x31, 0x30 };
static const unsigned char mask_edges[4] = { 0x1E, 0x18, 0x1A, 0x1C };
static const unsigned char mask_colors[5] = { 0x1E, 0x1F, 0x3E, 0x5E, 0x9E };
static const unsigned char page_tiles[4] = { TILE_SOLID1 & 0xFF, TILE_SOLID2 & 0xFF, TILE_SOLID3 & 0xFF, TILE_STRIPES & 0xFF };

/* 保存区放在 ERAM（$6C00 起 2K，BIOS 不用），省下程序自己的内存 */
#define SAVE_SPRITE ERAM_SAVE_SPRITE
#define SAVE_NAMES(page) (ERAM_SAVE_NAMES + (unsigned int)(page) * SAVE_NAME_BYTES)
static unsigned char ext_dos; /* DOS 的 $2008 影子值（真机默认 $88：四平面） */
static unsigned char ext_default; /* 场景用的 $2008：DOS 的值去掉位 7，改用二平面 */
static unsigned char stopped;
static char text_buffer[34];

/* $2002 标志测试结果：4 种情形各取 3 帧，再在第 3 帧之后立即读一次 */
static unsigned char flag_status[4][3];
static unsigned char flag_after[4];

/* ---- 显存读写 ---- */

void vram_seek(unsigned int address)
{
    unsigned char sink = REG(PPU_STATUS);

    (void)sink;
    REG(PPU_ADDRESS) = (unsigned char)(address >> 8);
    REG(PPU_ADDRESS) = (unsigned char)address;
}

void vram_fill(unsigned int address, unsigned char value, unsigned int count)
{
    vram_seek(address);
    while (count-- != 0) {
        REG(PPU_DATA) = value;
    }
}

void vram_write_block(unsigned int address, const unsigned char *source, unsigned int count)
{
    unsigned int i;

    vram_seek(address);
    for (i = 0; i < count; ++i) {
        REG(PPU_DATA) = source[i];
    }
}

/* 读一个字节：设地址后第一次读只是刷新缓冲，重设地址后第二次读才是内容（与 memtest 相同） */
unsigned char vram_peek(unsigned int address)
{
    unsigned char sink;

    vram_seek(address);
    sink = REG(PPU_DATA);
    (void)sink;
    vram_seek(address);
    return REG(PPU_DATA);
}

void vram_read_block(unsigned int address, unsigned char *dest, unsigned int count)
{
    unsigned int i;

    for (i = 0; i < count; ++i) {
        dest[i] = vram_peek(address + i);
    }
}

/* ---- 图形准备与恢复（要求显示已关） ---- */

/* 保存会被改动的显存区域，并记下 $2008 的值 */
void gfx_open(void)
{
    unsigned char page;

    REG(VRAM_BANK) = 0;
    ext_dos = REG(SB_PPU_EXT_CTRL_SHADOW);
    ext_default = ext_dos & 0x7F;
    vram_read_block(SPRITE_ADDRESS, SAVE_SPRITE, SAVE_SPRITE_BYTES);
    for (page = 0; page < 4; ++page) {
        vram_read_block((unsigned int)page * 0x800u + SAVE_NAME_OFFSET, SAVE_NAMES(page), SAVE_NAME_BYTES);
    }
}

void gfx_close(void)
{
    unsigned char page;

    REG(VRAM_BANK) = 0;
    vram_write_block(SPRITE_ADDRESS, SAVE_SPRITE, SAVE_SPRITE_BYTES);
    for (page = 0; page < 4; ++page) {
        vram_write_block((unsigned int)page * 0x800u + SAVE_NAME_OFFSET, SAVE_NAMES(page), SAVE_NAME_BYTES);
    }
    REG(PPU_EXT) = ext_dos;
}

/* 写入测试图块、精灵图案与调色板；之后 $4300 保持为 TEST_BANK，由 gfx_close 恢复成 0 */
static void assets_write(void)
{
    unsigned char i;
    unsigned char plane;

    REG(VRAM_BANK) = TEST_BANK;
    for (i = 0; i < 4; ++i) {
        vram_write_block(ADDRESS_SOLID + (unsigned int)i * 16u, bg_tiles[i], 16);
    }
    vram_fill(ADDRESS_BLANK, 0, 16);
    for (i = 0; i < 16; ++i) {
        for (plane = 0; plane < 4; ++plane) {
            vram_fill(ADDRESS_COLOR16 + (unsigned int)i * 32u + (unsigned int)plane * 8u, (i >> plane) & 1 ? 0xFF : 0x00, 8);
        }
    }
    for (i = 0; i < SPRITE_COUNT; ++i) {
        vram_write_block(SPRITE_ADDRESS + (unsigned int)i * 16u, sprite_tiles[i], 16);
    }
    for (i = 0; i < 4; ++i) {
        REG(PALETTE + 60 + i) = bg_colors[i];
        REG(PALETTE + 16 + i) = sprite_colors[i];
    }
}

/* ---- 场景绘制（要求显示已关） ---- */

static void oam_reset(void)
{
    unsigned int i;

    REG(OAM_ADDRESS) = 0;
    for (i = 0; i < 256; ++i) {
        REG(OAM_DATA) = 0xF0;
    }
}

static void oam_put(unsigned char index, unsigned char y, unsigned char tile, unsigned char attr, unsigned char x)
{
    REG(OAM_ADDRESS) = (unsigned char)(index * 4);
    REG(OAM_DATA) = y;
    REG(OAM_DATA) = tile;
    REG(OAM_DATA) = attr;
    REG(OAM_DATA) = x;
}

/* 在名称表页 base（页号 * $800 或 $2000）的第 row 行第 col 列起填 width x height 个单元；单元 = 图块号低 12 位 + 调色板组 */
static void cells_fill(unsigned int base, unsigned char row, unsigned char col, unsigned char width, unsigned char height, unsigned int tile, unsigned char bank)
{
    unsigned char y;
    unsigned char x;
    unsigned char low = (unsigned char)tile;
    unsigned char high = (unsigned char)((bank << 4) | (unsigned char)(tile >> 8));

    for (y = 0; y < height; ++y) {
        vram_seek(base + (unsigned int)(row + y) * 64u + (unsigned int)col * 2u);
        for (x = 0; x < width; ++x) {
            REG(PPU_DATA) = low;
            REG(PPU_DATA) = high;
        }
    }
}

/* 打开显示：$2000 的 NMI 位固定为 0，滚动回到 (0, 0) */
static void render(unsigned char mask, unsigned char ctrl, unsigned char ext)
{
    REG(PPU_CTRL) = ctrl;
    REG(PPU_EXT) = ext;
    REG(PPU_SCROLL) = 0;
    REG(PPU_SCROLL) = 0;
    REG(PPU_MASK) = mask;
}

/* ---- $2002 位 6、5 标志（关 NMI、关中断状态下调用） ---- */

static void flags_scene(unsigned char which)
{
    unsigned char i;

    oam_reset();
    cells_fill(0, 10, 4, 2, 2, TILE_SOLID2, TEST_BANK);
    cells_fill(0, 10, 18, 2, 2, TILE_BLANK, TEST_BANK); /* 精灵 0 在这里时下面不能有 DOS 的文字，否则会碰撞 */
    if (which == 0) {
        oam_put(0, 85, SPRITE_SOLID, 0, 36);
    } else if (which == 1) {
        oam_put(0, 85, SPRITE_SOLID, 0, 150);
    } else {
        for (i = 0; i < (which == 2 ? 9 : 8); ++i) {
            oam_put(8 + i, 149, SPRITE_SOLID, 0, 10 + 16 * i);
        }
    }
}

void gfx_flags_run(void)
{
    unsigned char which;
    unsigned char frame;

    gfx_open();
    assets_write();
    for (which = 0; which < 4; ++which) {
        REG(PPU_MASK) = 0;
        flags_scene(which);
        render(0x1E, 0x00, ext_default);
        wait_vblank();
        wait_vblank();
        for (frame = 0; frame < 3; ++frame) {
            flag_status[which][frame] = wait_vblank_status();
        }
        flag_after[which] = REG(PPU_STATUS);
    }
    REG(PPU_MASK) = 0;
    gfx_close();
}

void gfx_flags_report(void)
{
    static const char *const names[4] = { "sprite 0 overlaps BG block:  ", "sprite 0 away from BG block: ", "9 sprites on one line:       ", "8 sprites on one line:       " };
    unsigned char which;
    unsigned char frame;

    report_line("==== G1 $2002 read at each vblank");
    report_line("bit7 vblank, bit6 sprite 0 hit, bit5 more than 8 sprites on a line");
    report_line("three frames, then one more read right after the third");
    for (which = 0; which < 4; ++which) {
        report_text(names[which]);
        for (frame = 0; frame < 3; ++frame) {
            report_hex(flag_status[which][frame]);
            report_text(" ");
        }
        report_text("after: ");
        report_hex(flag_after[which]);
        report_newline();
    }
    report_newline();
}

/* 看图场景的说明：屏幕上只写编号和简短提示，要回答的问题写在报告里 */
void gfx_scenes_report(void)
{
    report_line("==== VISUAL V1-V8: answer each, or send photos. V7, V8 change the");
    report_line("palette: reboot DOS after them. Any key = next screen, Esc = stop.");
    report_line("V1 top: F with attr 00 40 80 C0 = normal/mirrored/flipped/both?");
    report_line("V1 bottom: yellow over red, attr 00 left, 20 right = above/behind/hidden?");
    report_line("V2 $2001=1E 18 1A 1C: red BG block and yellow sprite at x=0:");
    report_line("   leftmost 8 pixels visible? (sprite at x=24 is the reference)");
    report_line("V3 $2001=1E 1F 3E 5E 9E: colors seen (black&white? tints?)");
    report_line("V4 $2000 bits 1-0 = 0..3: bottom band 0 white 1 red 2 green 3 bars?");
    report_line("V5 $2008 bit 0 = 0, 1: band white (names $0000) or red (names $2000)?");
    report_line("V6 8x16 sprites idx 66 67 69: top and bottom letters?");
    report_line("V7 palette (entry = 4*column+row), V8 16 colors (bytes 20-2F): photo");
}

/* ---- 看图场景 ---- */

static unsigned char hold(void)
{
    unsigned int key;

    sb_key_flush();
    do {
        key = sb_key_poll();
    } while (key == 0);
    return (unsigned char)key;
}

/* 在第 row 行（偶数）第 col 列起画一行标签文字（占 2 行）；文字图块按行、按列错开（每行 32 个图块号、每列 2 个），标签之间不互相覆盖 */
static void label(unsigned char row, unsigned char col, const char *text)
{
    sb_text_field(0, col, row, eram_text(text), LABEL_BASE + (unsigned int)row * 32u + (unsigned int)col * 2u, PALETTE_TEXT, 1);
}

/* 标签 = 前缀 + 两位十六进制 */
static void label_hex(unsigned char row, const char *prefix, unsigned char value)
{
    static const char digits[] = "0123456789ABCDEF";
    unsigned char n = (unsigned char)strlen(prefix);

    memcpy(text_buffer, prefix, n);
    text_buffer[n] = digits[value >> 4];
    text_buffer[n + 1] = digits[value & 15];
    text_buffer[n + 2] = 0;
    label(row, 0, text_buffer);
}

/* 每屏开始：清名称表缓冲并刷新，之后用 label 写标签（显示开着、NMI 开着时调用） */
static void screen_begin(void)
{
    sb_screen_clear(0x01, 0xF000u | TILE_BLANK);
    sb_screen_flush(0);
}

/* 标签写完后：关显示，清掉 OAM，之后直接写显存与 OAM */
static void screen_enter(void)
{
    label(28, 0, "any key=next, Esc=stop");
    sb_display_off();
    oam_reset();
}

/* 等一个键后恢复 DOS 的显示 */
static void screen_leave(void)
{
    unsigned char key = hold();

    REG(PPU_MASK) = 0;
    REG(PPU_CTRL) = 0;
    sb_display_on();
    if (key == KEY_ESC) {
        stopped = 1;
    }
}

static void scene_attributes(void)
{
    screen_begin();
    label(0, 0, "V1 SPRITE ATTR");
    label(16, 5, "00");
    label(16, 11, "40");
    label(16, 17, "80");
    label(16, 23, "C0");
    label(24, 6, "00");
    label(24, 18, "20");
    screen_enter();
    oam_put(8, 119, SPRITE_F, 0x00, 40);
    oam_put(9, 119, SPRITE_F, 0x40, 88);
    oam_put(10, 119, SPRITE_F, 0x80, 136);
    oam_put(11, 119, SPRITE_F, 0xC0, 184);
    cells_fill(0, 20, 4, 6, 2, TILE_SOLID2, TEST_BANK);
    cells_fill(0, 20, 16, 6, 2, TILE_SOLID2, TEST_BANK);
    oam_put(12, 163, SPRITE_SOLID, 0x00, 48);
    oam_put(13, 163, SPRITE_SOLID, 0x20, 144);
    render(0x1E, 0x00, ext_default);
    screen_leave();
}

static void scene_mask_edges(void)
{
    unsigned char i;

    for (i = 0; i < 4 && !stopped; ++i) {
        screen_begin();
        label_hex(0, "V2 $2001 = ", mask_edges[i]);
        screen_enter();
        cells_fill(0, 22, 0, 2, 3, TILE_SOLID2, TEST_BANK);
        oam_put(8, 213, SPRITE_SOLID, 0x00, 0);
        oam_put(9, 213, SPRITE_SOLID, 0x00, 24);
        render(mask_edges[i], 0x00, ext_default);
        screen_leave();
    }
}

static void scene_mask_colors(void)
{
    unsigned char i;

    for (i = 0; i < 5 && !stopped; ++i) {
        screen_begin();
        label_hex(0, "V3 $2001 = ", mask_colors[i]);
        screen_enter();
        cells_fill(0, 20, 2, 4, 4, TILE_STRIPES, TEST_BANK);
        cells_fill(0, 20, 8, 2, 4, TILE_SOLID1, TEST_BANK);
        cells_fill(0, 20, 11, 2, 4, TILE_SOLID2, TEST_BANK);
        cells_fill(0, 20, 14, 2, 4, TILE_SOLID3, TEST_BANK);
        oam_put(8, 163, SPRITE_SOLID, 0x00, 200);
        render(mask_colors[i], 0x00, ext_default);
        screen_leave();
    }
}

static void scene_pages(void)
{
    unsigned char page;
    unsigned char fill;

    for (page = 0; page < 4 && !stopped; ++page) {
        screen_begin();
        label_hex(0, "V4 BG PAGE ", page);
        screen_enter();
        for (fill = 0; fill < 4; ++fill) {
            cells_fill((unsigned int)fill * 0x800u, 24, 0, 32, 4, 0x0C00u + (unsigned int)page_tiles[fill], TEST_BANK);
        }
        render(0x1E, page, ext_default);
        screen_leave();
    }
}

static void scene_ext_bit0(void)
{
    unsigned char bit0;

    for (bit0 = 0; bit0 < 2 && !stopped; ++bit0) {
        screen_begin();
        label_hex(0, "V5 $2008 = ", (unsigned char)((ext_default & 0xFE) | bit0));
        screen_enter();
        cells_fill(0, 24, 0, 32, 4, TILE_SOLID1, TEST_BANK);
        cells_fill(0x2000u, 24, 0, 32, 4, TILE_SOLID2, TEST_BANK);
        render(0x1E, 0x00, (unsigned char)((ext_default & 0xFE) | bit0));
        screen_leave();
    }
}

static void scene_sprite16(void)
{
    screen_begin();
    label(0, 0, "V6 8x16 SPRITES");
    label(20, 6, "66");
    label(20, 14, "67");
    label(20, 22, "69");
    screen_enter();
    oam_put(8, 135, 66, 0x00, 48);
    oam_put(9, 135, 67, 0x00, 112);
    oam_put(10, 135, 69, 0x00, 176);
    render(0x1E, 0x20, ext_default);
    screen_leave();
}

/* 会改调色板：BG 第 0-14 组的 60 项写成 0-59，第 15 组留给文字 */
static void scene_palette(void)
{
    unsigned char bank;
    unsigned char row;
    unsigned char entry;

    screen_begin();
    label(0, 0, "V7 PALETTE");
    label(26, 0, "0 1 2 3 4 5 6 7 8 9 A B C D E");
    screen_enter();
    for (entry = 0; entry < 60; ++entry) {
        REG(PALETTE + entry) = entry;
    }
    for (bank = 0; bank < 15; ++bank) {
        for (row = 0; row < 4; ++row) {
            cells_fill(0, 16 + 2 * row, (unsigned char)(2 * bank), 2, 2, row == 0 ? TILE_BLANK : TILE_SOLID1 + row - 1, bank);
        }
    }
    render(0x1E, 0x00, ext_default);
    screen_leave();
}

/* 会改调色板：16 色模式下 16 个纯色格，对应调色板第 0-15 项 */
static void scene_color16(void)
{
    unsigned char k;

    screen_begin();
    label(0, 0, "V8 16-COLOR");
    screen_enter();
    for (k = 0; k < 16; ++k) {
        REG(PALETTE + k) = 0x20 + k;
        cells_fill(0, 22, (unsigned char)(2 * k), 2, 4, TILE_COLOR16 + 2u * k, 0);
    }
    render(0x1E, 0x00, (unsigned char)(ext_default | 0x80));
    screen_leave();
}

void gfx_scenes(void)
{
    stopped = 0;
    sb_display_off();
    gfx_open();
    assets_write();
    REG(SB_PPU_EXT_CTRL_SHADOW) = ext_default; /* 让 DOS 的文字服务画二平面字模，NMI 也按二平面显示 */
    sb_display_on();

    scene_attributes();
    if (!stopped) {
        scene_mask_edges();
    }
    if (!stopped) {
        scene_mask_colors();
    }
    if (!stopped) {
        scene_pages();
    }
    if (!stopped) {
        scene_ext_bit0();
    }
    if (!stopped) {
        scene_sprite16();
    }
    if (!stopped) {
        scene_palette();
    }
    if (!stopped) {
        scene_color16();
    }

    sb_display_off();
    gfx_close();
    REG(SB_PPU_EXT_CTRL_SHADOW) = ext_dos;
    sb_display_on();
    ui_puts("hwtest done");
}
