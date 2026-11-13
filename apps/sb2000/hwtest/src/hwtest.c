/*
 * ------------------------------------------------------------------------
 * 名称：hwtest.c
 * 说明：在 SB-DOS 下运行：hwtest；在真机上实测 memtest 没有覆盖的硬件行为，自动测试的结果写成当前目录下的 HWTEST.TXT，不在屏幕上显示。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：先在关 NMI、关中断的状态下自动跑完寄存器读回、定时器、DMA 与 $2002 标志的测试，再把结果和看图场景的说明写进 HWTEST.TXT（不在关中断时调 DOS）；
 *       然后可以选择按键翻页，依次看若干屏图形场景（精灵属性、$2001 各位、背景页、$2008 位 0、8×16 精灵、调色板、16 色模式）。
 *       屏幕上只显示简短的提示，每行不超过 31 个 ASCII 字符（屏幕 32 列）。
 * ------------------------------------------------------------------------
 */
#include "hwtest.h"

#define MSG_MEMORY "No memory"
#define BLANK_LINES 14
#define VBLANK_LIMIT 0xFFFF

/* 滚出一屏空白，让接下来的内容从屏幕下部往上排 */
void ui_blank(void)
{
    unsigned char i;

    for (i = 0; i < BLANK_LINES; ++i) {
        sb_newline();
    }
}

/* 把文字复制到 ERAM 的暂存区并返回它的地址，供传给 DOS 服务（常量可能落在窗口 3，DOS 读不到）；最多 63 个字符 */
const char *eram_text(const char *text)
{
    unsigned char i;

    for (i = 0; i < ERAM_TEXT_BYTES - 1 && text[i] != 0; ++i) {
        ERAM_TEXT[i] = text[i];
    }
    ERAM_TEXT[i] = 0;
    return ERAM_TEXT;
}

/* 显示一行文字并换行 */
void ui_puts(const char *text)
{
    sb_puts(eram_text(text));
}

/* 显示文字，不换行 */
void ui_print(const char *text)
{
    sb_print(eram_text(text));
}

/* 显示提示并等一个键，返回它的 ASCII 码 */
unsigned char ui_wait(const char *tail)
{
    ui_print(tail);
    return sb_getkey();
}

/* 轮询 $2002 位 7，等到下一次垂直消隐开始，返回读到的 $2002（含位 6、5）；读 $2002 会清掉位 7。超时返回 0 */
unsigned char wait_vblank_status(void)
{
    unsigned int n;
    unsigned char status = REG(PPU_STATUS);

    for (n = 0; n != VBLANK_LIMIT; ++n) {
        status = REG(PPU_STATUS);
        if (status & 0x80) {
            return status;
        }
    }
    return 0;
}

void wait_vblank(void)
{
    (void)wait_vblank_status();
}

/* 关着 NMI 与中断，依次跑自动测试；测试里不调用 DOS 服务 */
static void run_auto(void)
{
    sb_display_off();
    REG(PPU_CTRL) = 0;
    __asm__("php");
    __asm__("sei");

    reg_run();
    dma_run();
    gfx_flags_run();

    __asm__("plp");
    sb_display_on();
}

/* 把全部结果与看图场景的说明写进报告 */
static void write_report(void)
{
    report_line("HWTEST report: SB2000 real machine hardware test");
    report_line("Please send this file back (or type it in) with the answers to the");
    report_line("visual scenes at the end. $4020/$4022/$4033 and similar values are hex.");
    report_newline();
    reg_report();
    dma_report();
    gfx_flags_report();
    gfx_scenes_report();
}

/* 显示报告文件的保存结果 */
static void show_saved(unsigned char code, unsigned int length)
{
    if (code == 0) {
        ui_puts("Saved HWTEST.TXT");
        ui_print("Size ");
        sb_print_uint(length);
        ui_print(" bytes.");
        sb_newline();
    } else {
        ui_print("Save failed, error ");
        sb_print_hex(code);
        sb_newline();
        ui_puts("Disk full or protected?");
    }
}

int main(void)
{
    unsigned char key;
    unsigned char code;
    unsigned int length;

    ui_blank();
    ui_puts("hwtest: auto tests");
    ui_puts("Do not touch the keys.");
    ui_puts("Please wait about 15 s...");
    if (dma_prepare() == 0) {
        ui_puts(MSG_MEMORY);
        sb_getkey();
        return 0;
    }
    if (report_open() == 0) {
        dma_release();
        ui_puts(MSG_MEMORY);
        sb_getkey();
        return 0;
    }
    run_auto();
    dma_release();

    write_report();
    code = report_close(&length);
    show_saved(code, length);

    ui_puts("Visual tests V1-V8: see");
    ui_puts("the end of HWTEST.TXT.");
    ui_puts("V7, V8 change the palette:");
    ui_puts("reboot DOS after them.");
    key = ui_wait("Enter=start Esc=quit");
    if (key != KEY_ESC) {
        gfx_scenes();
    }
    return 0;
}
