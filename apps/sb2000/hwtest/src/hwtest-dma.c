/*
 * ------------------------------------------------------------------------
 * 名称：hwtest-dma.c
 * 说明：DMA 测试：$4020 位 7 对 $4048 位 6、5 的门控，目的是工作 RAM 还是显存，长度寄存器的含义（奇数长度、位 15）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：dma_run 必须在关 NMI、关中断、显示已关的状态下调用。源数据放在 dma_prepare 申请的扩展 RAM 页里（$404B 位 7 固定为 1）。
 *       观察点：ERAM $7C00 起 32 字节与显存 $2780 起 32 字节，测前都填 $55，测后看有多少字节变了；两处在测后恢复。
 *       位 15 的测试把长度写成 $8010，目的是扩展显存 bank 14 的 $8000：长度被屏蔽成 $0010 时只写 16 字节，
 *       否则会写 32K 并回绕，会改动扩展显存 bank 14 的内容（DOS 不用）与显存 $0000 起的几个字节（DOS 会重绘）。
 * ------------------------------------------------------------------------
 */
#include "hwtest.h"

#define SOURCE_PAGES 2
#define SOURCE_FIRST_BANK 0x80
#define PATTERN_BYTES 64
#define FILL 0x55
#define ERAM_CELL 0x7C00
#define VRAM_CELL_ADDRESS 0x2780
#define CELL_BYTES 32
#define WAIT_COUNT 20000U
#define BANK_TEST 14
#define GATE_ROWS 3
#define GATE_MODES 4
#define LENGTH_CASES 7

static const unsigned char gate_values[GATE_ROWS] = { 0x00, 0x80, 0xA0 };
static const unsigned char gate_modes[GATE_MODES] = { 0xA0, 0xE0, 0x80, 0xC0 };
static const unsigned int length_values[LENGTH_CASES] = { 1, 2, 3, 4, 5, 16, 17 };

static unsigned char source_page;
static unsigned char *const pattern = ERAM_PATTERN;
static unsigned char gate_result[GATE_ROWS][GATE_MODES][2]; /* [$4020][模式][目的 0 ERAM $7C00、1 显存 $2780]，值：位 0 ERAM 变了，位 1 显存变了 */
static unsigned char length_result[LENGTH_CASES]; /* 目的 ERAM，写入各长度后变了几个字节 */
static unsigned char bit15_changed; /* 长度 $8010 之后，扩展显存 $8100 起的 8 个采样字节里变了几个 */
static unsigned char bit15_head; /* 同上，$8000 起 16 字节里变了几个 */

/* 申请源数据页并写入样板，成功返回 1 */
unsigned char dma_prepare(void)
{
    unsigned char i;

    if (sb_page_alloc(SOURCE_PAGES, &source_page) != 0) {
        return 0;
    }
    for (i = 0; i < PATTERN_BYTES; ++i) {
        pattern[i] = 0x31 + i;
    }
    sb_far_write(source_page, 0, pattern, PATTERN_BYTES);
    return 1;
}

void dma_release(void)
{
    sb_page_free(source_page, SOURCE_PAGES);
}

static void settle(void)
{
    unsigned int wait;

    for (wait = 0; wait != WAIT_COUNT; ++wait) {
        __asm__("nop");
    }
}

/* 从源数据页开头复制 length 字节：$404B 位 7 固定为 1，源是扩展 RAM */
static void start_dma(unsigned char mode, unsigned int dest, unsigned int length)
{
    unsigned long source = (unsigned long)(unsigned char)(source_page - SOURCE_FIRST_BANK) << 12;

    REG(DMA_SOURCE_BANK) = 0x10 | (unsigned char)((source >> 15) & 0x0F);
    REG(DMA_SOURCE_LOW) = (unsigned char)source;
    REG(DMA_SOURCE_HIGH) = ((unsigned char)(source >> 8) & 0x7F) | 0x80;
    REG(DMA_DEST_LOW) = (unsigned char)dest;
    REG(DMA_DEST_HIGH) = (unsigned char)(dest >> 8);
    REG(DMA_LENGTH_LOW) = (unsigned char)length;
    REG(DMA_LENGTH_HIGH) = (unsigned char)(length >> 8);
    REG(DMA_MODE) = mode;
    settle();
}

/* 把两个观察点填成 FILL */
static void fill_cells(void)
{
    memset((void *)ERAM_CELL, FILL, CELL_BYTES);
    vram_fill(VRAM_CELL_ADDRESS, FILL, CELL_BYTES);
}

/* 数 ERAM 观察点里变了几个字节 */
static unsigned char eram_changed(void)
{
    unsigned char *cell = (unsigned char *)ERAM_CELL;
    unsigned char i;
    unsigned char total = 0;

    for (i = 0; i < CELL_BYTES; ++i) {
        if (cell[i] != FILL) {
            ++total;
        }
    }
    return total;
}

static unsigned char vram_changed(void)
{
    unsigned char buffer[CELL_BYTES];
    unsigned char i;
    unsigned char total = 0;

    vram_read_block(VRAM_CELL_ADDRESS, buffer, CELL_BYTES);
    for (i = 0; i < CELL_BYTES; ++i) {
        if (buffer[i] != FILL) {
            ++total;
        }
    }
    return total;
}

static void test_gating(void)
{
    unsigned char row;
    unsigned char mode;
    unsigned char target;
    unsigned int dest;
    unsigned char value;

    for (row = 0; row < GATE_ROWS; ++row) {
        REG(KEY_DATA) = gate_values[row];
        for (mode = 0; mode < GATE_MODES; ++mode) {
            for (target = 0; target < 2; ++target) {
                dest = target ? VRAM_CELL_ADDRESS : ERAM_CELL;
                fill_cells();
                start_dma(gate_modes[mode], dest, 16);
                value = 0;
                if (eram_changed() != 0) {
                    value |= 1;
                }
                if (vram_changed() != 0) {
                    value |= 2;
                }
                gate_result[row][mode][target] = value;
            }
        }
    }
    REG(KEY_DATA) = 0x80;
}

static void test_length(void)
{
    unsigned char i;

    REG(KEY_DATA) = 0x80;
    for (i = 0; i < LENGTH_CASES; ++i) {
        fill_cells();
        start_dma(0xA0, ERAM_CELL, length_values[i]);
        length_result[i] = eram_changed();
    }
}

/* 长度 $8010：看位 15 是被屏蔽（只写 16 字节）还是当作 32K 长度 */
static void test_bit15(void)
{
    unsigned char buffer[16];
    unsigned char i;
    unsigned char sample[8];

    REG(KEY_DATA) = 0x80;
    REG(VRAM_BANK) = BANK_TEST;
    vram_fill(0x8000, FILL, 16);
    vram_fill(0x8100, FILL, 8);
    start_dma(0x80, 0x8000, 0x8010);
    vram_read_block(0x8000, buffer, 16);
    vram_read_block(0x8100, sample, 8);
    bit15_head = 0;
    bit15_changed = 0;
    for (i = 0; i < 16; ++i) {
        if (buffer[i] != FILL) {
            ++bit15_head;
        }
    }
    for (i = 0; i < 8; ++i) {
        if (sample[i] != FILL) {
            ++bit15_changed;
        }
    }
    REG(VRAM_BANK) = 0;
}

void dma_run(void)
{
    unsigned char eram_saved[CELL_BYTES];
    unsigned char vram_saved[CELL_BYTES];

    memcpy(eram_saved, (void *)ERAM_CELL, CELL_BYTES);
    vram_read_block(VRAM_CELL_ADDRESS, vram_saved, CELL_BYTES);

    test_gating();
    test_length();
    test_bit15();

    memcpy((void *)ERAM_CELL, eram_saved, CELL_BYTES);
    vram_write_block(VRAM_CELL_ADDRESS, vram_saved, CELL_BYTES);
}

/* D1：$4020 门控。每种 $4020 值、每种模式各一行：目的是 ERAM $7C00 和显存 $2780 时各有哪个观察点变了 */
static void report_gating(void)
{
    unsigned char row;
    unsigned char mode;

    report_line("==== D1 DMA vs $4020 (length 16, source bank ok)");
    report_line("value: 0 nothing changed, 1 ERAM $7C00 changed,");
    report_line("2 VRAM $2780 changed, 3 both changed");
    for (row = 0; row < GATE_ROWS; ++row) {
        for (mode = 0; mode < GATE_MODES; ++mode) {
            report_text("$4020=");
            report_hex(gate_values[row]);
            report_text(" mode=");
            report_hex(gate_modes[mode]);
            report_text(" dest ERAM: ");
            report_uint(gate_result[row][mode][0]);
            report_text(" dest VRAM: ");
            report_uint(gate_result[row][mode][1]);
            report_newline();
        }
    }
    report_newline();
}

/* D2：长度寄存器 */
static void report_length(void)
{
    unsigned char i;

    report_line("==== D2 DMA length (mode A0, dest ERAM $7C00)");
    report_line("bytes changed after writing length L:");
    for (i = 0; i < LENGTH_CASES; ++i) {
        report_text("L=");
        report_uint(length_values[i]);
        report_text(" -> ");
        report_uint(length_result[i]);
        report_newline();
    }
    report_line("L=$8010, mode 80, dest extended VRAM bank 14 $8000:");
    report_text("  bytes changed in first 16: ");
    report_uint(bit15_head);
    report_text("  in the 8 sample bytes at $8100: ");
    report_uint(bit15_changed);
    report_newline();
    report_newline();
}

void dma_report(void)
{
    report_gating();
    report_length();
}
