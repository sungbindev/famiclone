/*
 * ------------------------------------------------------------------------
 * 名称：ppu.c
 * 说明：测试 sb2000-ppu.h 里的图形端口函数：显存写、填充、读、单字节读，调色板、精灵 RAM、等垂直消隐、打开显示。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：由 sb2000-library.test.mjs 在模拟器里运行；显存访问放在扩展显存 bank 15（DOS 不用），测完把 bank 与调色板项还原成测前的值。
 * ------------------------------------------------------------------------
 */
#include <string.h>
#include <sb2000.h>
#include "sb-test.h"

/** 测试数据的字节数 */
#define PATTERN_SIZE 16
/** 测试用的调色板项：精灵调色板的最后一项，DOS 不用 */
#define PALETTE_INDEX 63

static unsigned char pattern[PATTERN_SIZE];
static unsigned char readback[24];

int main(void)
{
    unsigned char old_color;
    unsigned char saved_ext;
    unsigned char status;
    unsigned char i;

    for (i = 0; i < PATTERN_SIZE; ++i) {
        pattern[i] = (unsigned char)(i * 7 + 3);
    }
    saved_ext = SB_BYTE_AT(SB_PPU_EXT_CTRL_SHADOW);

    sb_display_off();
    SB_BYTE_AT(SB_HW_VRAM_BANK) = 15;

    /* 填充与单字节读 */
    sb_ppu_fill(0xC000, 0xAA, 32);
    SB_CHECK(sb_ppu_peek(0xC000) == 0xAA && sb_ppu_peek(0xC01F) == 0xAA);

    /* 写一段数据，连同前后的填充值一起读回 */
    sb_ppu_write(0xC008, pattern, PATTERN_SIZE);
    memset(readback, 0, sizeof(readback));
    sb_ppu_read(0xC000, readback, 24);
    SB_CHECK(memcmp(readback, "\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA", 8) == 0);
    SB_CHECK(memcmp(readback + 8, pattern, PATTERN_SIZE) == 0);

    /* 只设地址、直接写数据端口，再读回 */
    sb_ppu_address(0xC010);
    SB_BYTE_AT(SB_HW_PPU_DATA) = 0x5C;
    SB_CHECK_EQ(sb_ppu_peek(0xC010), 0x5C);

    /* 调色板：读回只有低 6 位 */
    old_color = sb_ppu_palette_get(PALETTE_INDEX);
    sb_ppu_palette_set(PALETTE_INDEX, 0x15);
    SB_CHECK_EQ(sb_ppu_palette_get(PALETTE_INDEX), 0x15);
    sb_ppu_palette_set(PALETTE_INDEX, 0x2A);
    SB_CHECK_EQ(sb_ppu_palette_get(PALETTE_INDEX), 0x2A);
    sb_ppu_palette_set(PALETTE_INDEX, old_color);
    SB_CHECK_EQ(sb_ppu_palette_get(PALETTE_INDEX), old_color);

    /* 精灵 RAM：先全部清成 $F0，再写第 5 个精灵（地址 20 起的 4 个字节） */
    sb_ppu_oam_clear();
    SB_BYTE_AT(SB_HW_OAM_ADDRESS) = 0;
    SB_CHECK_EQ(SB_BYTE_AT(SB_HW_OAM_DATA), 0xF0);
    SB_BYTE_AT(SB_HW_OAM_ADDRESS) = 255;
    SB_CHECK_EQ(SB_BYTE_AT(SB_HW_OAM_DATA), 0xF0);
    sb_ppu_oam_put(5, 0x10, 0x22, 0x01, 0x40);
    for (i = 0; i < 4; ++i) {
        SB_BYTE_AT(SB_HW_OAM_ADDRESS) = (unsigned char)(20 + i);
        readback[i] = SB_BYTE_AT(SB_HW_OAM_DATA);
    }
    SB_CHECK(readback[0] == 0x10 && readback[1] == 0x22 && readback[2] == 0x01 && readback[3] == 0x40);
    sb_ppu_oam_clear();

    /* 设置扩展控制寄存器并打开显示：显示寄存器写 0，显示仍是关的；扩展控制寄存器可以读回 */
    sb_ppu_render(0, 0, 0x01);
    SB_CHECK_EQ(SB_BYTE_AT(SB_HW_PPU_EXT), 0x01);
    SB_BYTE_AT(SB_HW_PPU_EXT) = saved_ext;
    SB_BYTE_AT(SB_HW_VRAM_BANK) = 0;
    sb_display_on();

    /* 显示开着时能等到垂直消隐 */
    status = sb_ppu_wait_vblank();
    SB_CHECK((status & SB_HW_STATUS_VBLANK) != 0);

    sb_test_done();
    return 0;
}
