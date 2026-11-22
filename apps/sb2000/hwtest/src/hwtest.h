/*
 * ------------------------------------------------------------------------
 * 名称：hwtest.h
 * 说明：hwtest 各源文件共用的寄存器定义、结果存放与函数声明。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：寄存器含义见 vscode/docs/sb2000/03-硬件说明.md。
 * ------------------------------------------------------------------------
 */
#ifndef HWTEST_H
#define HWTEST_H

#include <string.h>
#include <sb2000.h>

#define REG(address) SB_BYTE_AT(address)

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
#define ERAM_PALETTE ((unsigned char *)0x74C0) /* 测前的 64 项调色板 */
#define ERAM_TEXT_BYTES 64

/* hwtest.c：界面与垂直消隐 */
const char *eram_text(const char *text);
void ui_blank(void);
void ui_puts(const char *text);
void ui_print(const char *text);
unsigned char ui_wait(const char *tail);

/* hwtest-report.c：报告文件。文字先写进申请到的扩展 RAM 页，report_close 时整个写成 HWTEST.TXT */
unsigned char report_open(void);
void report_text(const char *text);
void report_newline(void);
void report_line(const char *text);
void report_hex(unsigned char value);
void report_uint(unsigned int value);
void report_bytes(const char *label, const unsigned char *bytes, unsigned char count);
unsigned char report_close(unsigned int *length);

/* hwtest-reg.c：寄存器读回、外设探测、定时器。逐项运行，序号从 0 起，要在关 NMI、关中断、显示已关时调用 */
#define REG_STEPS 14
void reg_step(unsigned char index);
const char *reg_step_name(unsigned char index);
void reg_report(void);

/* hwtest-dma.c：DMA，用法同上 */
#define DMA_STEPS 3
unsigned char dma_prepare(void);
void dma_release(void);
void dma_step(unsigned char index);
const char *dma_step_name(unsigned char index);
void dma_report(void);

/* hwtest-gfx.c：图形（显存读写等用 sb2000.h 里的 sb_ppu_ 函数） */
void gfx_open(void);
void gfx_close(void);
void gfx_flags_run(void);
void gfx_flags_report(void);
void gfx_scenes_report(void);
void gfx_scenes(void);

#endif
