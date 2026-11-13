/*
 * ------------------------------------------------------------------------
 * 名称：dma.c
 * 说明：测试 sb2000-dma.h 里的 DMA 函数：源地址换算、复制到 ERAM 与显存、长度与页边界、出错参数。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：由 sb2000-library.test.mjs 在模拟器里运行；复制到 ERAM 的结果直接读内存核对，复制到扩展显存的结果用 sb_ppu_read 读回。
 *       长度寄存器写“字节数 - 1”，所以每次都核对“目的的后一个字节没有被改”。测试区用 ERAM $7C00 起（BIOS 不用），测前保存、测完还原。
 * ------------------------------------------------------------------------
 */
#include <string.h>
#include <sb2000.h>
#include "sb-test.h"

/** 复制目的：ERAM $7C00 起 */
#define ERAM_TARGET ((unsigned char *)0x7C00)
/** 测试数据的字节数 */
#define PATTERN_SIZE 300

static unsigned char pattern[PATTERN_SIZE];
static unsigned char readback[PATTERN_SIZE];
static unsigned char saved[PATTERN_SIZE];

int main(void)
{
    unsigned char page;
    unsigned char error;
    unsigned int i;

    for (i = 0; i < PATTERN_SIZE; ++i) {
        pattern[i] = (unsigned char)(i * 7 + 3);
    }
    memcpy(saved, ERAM_TARGET, PATTERN_SIZE);

    /* 源地址换算：页 $80 是扩展 RAM 的开头，每页 4K */
    SB_CHECK(sb_dma_page_address(0x80, 0) == 0UL);
    SB_CHECK(sb_dma_page_address(0x81, 0x10) == 0x1010UL);
    SB_CHECK(sb_dma_page_address(0xFF, 0xFFF) == 0x7FFFFUL);

    error = sb_page_alloc(2, &page);
    SB_CHECK(error == 0);
    sb_far_write(page, 0, pattern, PATTERN_SIZE);

    /* 长度 1：只复制一个字节，后一个字节不动 */
    memset(ERAM_TARGET, 0xEE, 64);
    SB_CHECK_EQ(sb_dma_to_cpu(sb_dma_page_address(page, 5), 0x7C00, 1), 0);
    SB_CHECK(ERAM_TARGET[0] == pattern[5] && ERAM_TARGET[1] == 0xEE);

    /* 长度 16，页内偏移 5 */
    memset(ERAM_TARGET, 0xEE, 64);
    SB_CHECK_EQ(sb_dma_to_cpu(sb_dma_page_address(page, 5), 0x7C00, 16), 0);
    SB_CHECK(memcmp(ERAM_TARGET, pattern + 5, 16) == 0 && ERAM_TARGET[16] == 0xEE);

    /* 整段 300 字节 */
    memset(ERAM_TARGET, 0, PATTERN_SIZE);
    SB_CHECK_EQ(sb_dma_to_cpu(sb_dma_page_address(page, 0), 0x7C00, PATTERN_SIZE), 0);
    SB_CHECK(memcmp(ERAM_TARGET, pattern, PATTERN_SIZE) == 0);

    /* 源跨过页边界：数据从页内偏移 $F00 起，取 $F80 处的 172 字节 */
    sb_far_write(page, 0x0F00, pattern, PATTERN_SIZE);
    memset(ERAM_TARGET, 0, PATTERN_SIZE);
    SB_CHECK_EQ(sb_dma_to_cpu(sb_dma_page_address(page, 0x0F80), 0x7C00, PATTERN_SIZE - 128), 0);
    SB_CHECK(memcmp(ERAM_TARGET, pattern + 128, PATTERN_SIZE - 128) == 0);

    /* 出错参数：长度为 0、超过 $8000，目的越过 $8000 */
    SB_CHECK_EQ(sb_dma_to_cpu(0, 0x7C00, 0), SB_ERR_BAD_PARAMETER);
    SB_CHECK_EQ(sb_dma_to_cpu(0, 0x7C00, 0x8001u), SB_ERR_BAD_PARAMETER);
    SB_CHECK_EQ(sb_dma_to_cpu(0, 0x7FF0, 0x20), SB_ERR_BAD_PARAMETER);
    SB_CHECK_EQ(sb_dma_to_cpu(0, 0x8000, 1), SB_ERR_BAD_PARAMETER);
    SB_CHECK_EQ(sb_dma_to_vram(0, 0xC000, 0), SB_ERR_BAD_PARAMETER);

    /* 复制到扩展显存（bank 15，DOS 不用）：先填 $EE，复制 16 字节，后面的不动 */
    sb_display_off();
    SB_BYTE_AT(SB_HW_VRAM_BANK) = 15;
    sb_ppu_fill(0xC000, 0xEE, 64);
    error = sb_dma_to_vram(sb_dma_page_address(page, 0), 0xC000, 16);
    memset(readback, 0, 64);
    sb_ppu_read(0xC000, readback, 64);
    SB_CHECK(error == 0 && memcmp(readback, pattern, 16) == 0 && readback[16] == 0xEE && readback[63] == 0xEE);

    /* 整段 300 字节 */
    error = sb_dma_to_vram(sb_dma_page_address(page, 0), 0xC000, PATTERN_SIZE);
    memset(readback, 0, PATTERN_SIZE);
    sb_ppu_read(0xC000, readback, PATTERN_SIZE);
    SB_BYTE_AT(SB_HW_VRAM_BANK) = 0;
    sb_display_on();
    SB_CHECK(error == 0 && memcmp(readback, pattern, PATTERN_SIZE) == 0);

    memcpy(ERAM_TARGET, saved, PATTERN_SIZE);
    sb_page_free(page, 2);

    /* 窗口 2 被换过页：程序的代码与数据要全部在 $A000 之前 */
    SB_CHECK((unsigned int)&readback[PATTERN_SIZE] < 0xA000 && (unsigned int)&sb_test_value[SB_TEST_MAX] < 0xA000);
    sb_test_done();
    return 0;
}
