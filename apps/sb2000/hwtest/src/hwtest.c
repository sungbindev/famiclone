/*
 * ------------------------------------------------------------------------
 * 名称：hwtest.c
 * 说明：在 SB-DOS 下运行：hwtest；在真机上实测 memtest 没有覆盖的硬件行为，测试结果写成当前目录下的 HWTEST.TXT，不在屏幕上显示。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：在关 NMI、关中断的状态下逐项跑寄存器读回、定时器、DMA 与 $2002 标志的测试，每项之后把调色板里的文字色换成红、绿、蓝之一并等按键，
 *       屏幕变灰说明刚才那一项让颜色输出丢了；全部跑完后把结果和看图场景的说明写进 HWTEST.TXT（不在关中断时调 DOS）；
 *       然后可以选择按键翻页，依次看若干屏图形场景（精灵属性、$2001 各位、背景页、$2008 位 0、8×16 精灵、调色板、16 色模式）。
 *       屏幕上只显示简短的提示，每行不超过 31 个 ASCII 字符（屏幕 32 列）。
 * ------------------------------------------------------------------------
 */
#include "hwtest.h"

#define MSG_MEMORY "No memory"
#define BLANK_LINES 14
#define VBLANK_LIMIT 0xFFFF
#define PALETTE_ENTRIES 64
#define KEY_SETTLE 40000U
#define STEP_TOTAL (REG_STEPS + DMA_STEPS + 1)

/* 探测色：红、绿、蓝；每一项之后按顺序用它们换掉所有文字的前景色 */
static const unsigned char probe_colors[3] = { 0x36, 0x3A, 0x31 };

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

/* 记下测前的 64 项调色板 */
static void palette_save(void)
{
    unsigned char i;

    sb_display_off();
    for (i = 0; i < PALETTE_ENTRIES; ++i) {
        ERAM_PALETTE[i] = REG(PALETTE + i) & 0x3F;
    }
    sb_display_on();
}

static void palette_restore(void)
{
    unsigned char i;

    sb_display_off();
    for (i = 0; i < PALETTE_ENTRIES; ++i) {
        REG(PALETTE + i) = ERAM_PALETTE[i];
    }
    sb_display_on();
}

/* \u6309\u952e\u540e\u5148\u7b49\u4e00\u4f1a\u513f\uff08\u4e2d\u65ad\u5f00\u7740\uff09\uff0c\u8ba9 DOS \u628a\u6309\u952e\u7684\u91ca\u653e\u7801\u6536\u5b8c\uff0c\u518d\u5173\u4e2d\u65ad\u505a\u6d4b\u8bd5 */
static void settle_keys(void)
{
    unsigned int n;

    for (n = 0; n != KEY_SETTLE; ++n) {
        __asm__("nop");
    }
}

static const char *step_name(unsigned char index)
{
    if (index < REG_STEPS) {
        return reg_step_name(index);
    }
    if (index < REG_STEPS + DMA_STEPS) {
        return dma_step_name(index - REG_STEPS);
    }
    return "$2002 flags";
}

/* 关着 NMI 与中断跑第 index 项（测试里不调用 DOS 服务），再把每组调色板的后 3 项改成探测色 */
static void run_step(unsigned char index)
{
    unsigned char i;

    sb_display_off();
    REG(PPU_CTRL) = 0;
    __asm__("php");
    __asm__("sei");

    if (index < REG_STEPS) {
        reg_step(index);
    } else if (index < REG_STEPS + DMA_STEPS) {
        dma_step(index - REG_STEPS);
    } else {
        gfx_flags_run();
    }

    __asm__("plp");
    for (i = 0; i < PALETTE_ENTRIES; ++i) {
        if ((i & 3) != 0) {
            REG(PALETTE + i) = probe_colors[index % 3];
        }
    }
    sb_display_on();
}

/* 把全部结果与看图场景的说明写进报告；done 是已经跑完的项数 */
static void write_report(unsigned char done)
{
    report_line("HWTEST report (values are hex), please send it back");
    if (done < STEP_TOTAL) {
        report_text("STOPPED, steps run: ");
        report_uint(done);
        report_newline();
    }
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
    }
}

int main(void)
{
    unsigned char key;
    unsigned char code;
    unsigned char step;
    unsigned int length;

    ui_blank();
    ui_puts("hwtest: step by step");
    ui_puts("Text turns R, G, B per step");
    ui_puts("Enter=run Space=skip Esc=quit");
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
    palette_save();
    for (step = 0; step < STEP_TOTAL; ++step) {
        ui_print("Step ");
        sb_print_uint(step + 1);
        ui_print(": ");
        ui_print(step_name(step));
        key = sb_getkey();
        if (key == KEY_ESC) {
            sb_newline();
            break;
        }
        if (key == ' ') {
            ui_print(" -");
            sb_newline();
            continue;
        }
        ui_print(" ...");
        settle_keys();
        run_step(step);
        sb_newline();
    }
    palette_restore();
    dma_release();

    write_report(step);
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
