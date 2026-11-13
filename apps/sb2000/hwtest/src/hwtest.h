/*
 * ------------------------------------------------------------------------
 * 名称：hwtest.h
 * 说明：hwtest 各源文件共用的寄存器定义、结果存放与函数声明。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：寄存器含义见 packages/fcdev/docs/sb2000/03-硬件说明.md。
 * ------------------------------------------------------------------------
 */
#ifndef HWTEST_H
#define HWTEST_H

#include <string.h>
#include <sb2000.h>

#define REG(address) SB_BYTE_AT(address)

/* 图形端口 */
#define PPU_CTRL 0x2000
#define PPU_MASK 0x2001
#define PPU_STATUS 0x2002
#define OAM_ADDRESS 0x2003
#define OAM_DATA 0x2004
#define PPU_SCROLL 0x2005
#define PPU_ADDRESS 0x2006
#define PPU_DATA 0x2007
#define PPU_EXT 0x2008
#define PALETTE 0x2040

/* 声音、键盘、鼠标与外设 */
#define SOUND_ENABLE 0x4015
#define PAD_1 0x4016
#define PAD_2 0x4017
#define KEY_DATA 0x4020
#define KEY_CONTROL 0x4022
#define MOUSE_SELECT 0x4023
#define EXT_IO 0x4026
#define IRQ_MASK 0x4032
#define IRQ_PENDING 0x4033
#define TIMER_CONTROL 0x4034
#define TIMER_PRESET 0x4035
#define TIMER_CURRENT 0x4036
#define RTC_ADDRESS 0x4080
#define RTC_DATA 0x4081

/* DMA */
#define DMA_MODE 0x4048
#define DMA_SOURCE_BANK 0x4049
#define DMA_SOURCE_LOW 0x404A
#define DMA_SOURCE_HIGH 0x404B
#define DMA_DEST_LOW 0x404C
#define DMA_DEST_HIGH 0x404D
#define DMA_LENGTH_LOW 0x404E
#define DMA_LENGTH_HIGH 0x404F
#define VRAM_BANK 0x4300

/* 菜单与按键 */
#define KEY_ESC 0x1B

/* 扩展显存里放测试图块的 bank（DOS 不用），图块号 = 显存地址 / 16 */
#define TEST_BANK 15

/* ERAM 里的暂存区（BIOS 不用）。传给 DOS 服务的字符串与数据必须在 $8000-$AFFF 或 ERAM，不能在窗口 3（$B000 以上），
   而程序的常量与变量可能落在那里，所以这类内容都先复制到下面的区域再传出去 */
#define ERAM_SAVE_SPRITE ((unsigned char *)0x6C00) /* 1024 字节 */
#define ERAM_SAVE_NAMES ((unsigned char *)0x7000) /* 4 x 256 字节 */
#define ERAM_TEXT ((char *)0x7400) /* ERAM_TEXT_BYTES 字节：要显示的文字与文件名 */
#define ERAM_LINE ((char *)0x7440) /* 报告的行缓冲 */
#define ERAM_PATTERN ((unsigned char *)0x7480) /* DMA 源数据样板 */
#define ERAM_TEXT_BYTES 64

/* hwtest.c：界面与垂直消隐 */
const char *eram_text(const char *text);
void ui_blank(void);
void ui_puts(const char *text);
void ui_print(const char *text);
unsigned char ui_wait(const char *tail);
void wait_vblank(void);
unsigned char wait_vblank_status(void);

/* hwtest-report.c：报告文件。文字先写进申请到的扩展 RAM 页，report_close 时整个写成 HWTEST.TXT */
unsigned char report_open(void);
void report_text(const char *text);
void report_newline(void);
void report_line(const char *text);
void report_hex(unsigned char value);
void report_uint(unsigned int value);
void report_bytes(const char *label, const unsigned char *bytes, unsigned char count);
unsigned char report_close(unsigned int *length);

/* hwtest-reg.c：寄存器读回、外设探测、定时器 */
void reg_run(void);
void reg_report(void);

/* hwtest-dma.c：DMA */
unsigned char dma_prepare(void);
void dma_release(void);
void dma_run(void);
void dma_report(void);

/* hwtest-gfx.c：显存读写（需要关着显示，$2000 的地址增量位为 0） */
void vram_seek(unsigned int address);
void vram_fill(unsigned int address, unsigned char value, unsigned int count);
void vram_read_block(unsigned int address, unsigned char *dest, unsigned int count);
void vram_write_block(unsigned int address, const unsigned char *source, unsigned int count);
unsigned char vram_peek(unsigned int address);

/* hwtest-gfx.c：图形 */
void gfx_open(void);
void gfx_close(void);
void gfx_flags_run(void);
void gfx_flags_report(void);
void gfx_scenes_report(void);
void gfx_scenes(void);

#endif
