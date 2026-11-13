/*
 * ------------------------------------------------------------------------
 * 名称：hwtest-reg.c
 * 说明：寄存器读回、外设探测与定时器测试：图形端口、OAM、调色板的读回，写保护寄存器与未映射地址的读值，
 *       $4015、$4022、$4023、$4026、实时时钟 $4080/$4081 的读回，定时器一次计时与连续计时的行为。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：reg_step 逐项运行，必须在关 NMI、关中断、显示已关的状态下调用；会改动的单元都在测完后恢复：
 *       $2008 恢复成 DOS 的影子值，定时器与 $4032 恢复成 DOS 的值，$4023 与 $4022 恢复成测前的值；
 *       OAM 与 BG 调色板第 15 组（$207C-$207F）不恢复，DOS 的下一次 NMI 与重绘会覆盖它们。
 * ------------------------------------------------------------------------
 */
#include "hwtest.h"

#define OAM_TEST_INDEX 240 /* 精灵 60 的 Y 字节地址，DOS 的光标是精灵 0 */
#define PALETTE_TEST (PALETTE + 60) /* BG 调色板第 15 组 */
#define FRAME_COUNT 120
#define POLL_LIMIT 60000U
#define DELAY_SHORT 4000U
#define DELAY_LONG 20000U

/* 写入值：$2008、OAM 与调色板各写 4 个值再读回 */
static const unsigned char ext_values[4] = { 0x00, 0xFF, 0xA5, 0x5A };
static const unsigned char oam_values[4] = { 0x12, 0xFF, 0xFF, 0x34 };
static const unsigned char pal_values[4] = { 0x3F, 0xFF, 0xC0, 0x55 };
static const unsigned char sound_values[4] = { 0x00, 0x0F, 0x1F, 0xFF };
static const unsigned int unmapped_addresses[8] = { 0x2009, 0x203F, 0x2080, 0x3FFF, 0x4018, 0x4019, 0x401F, 0x4037 };

static unsigned char ext_back[4];
static unsigned char oam_back[4];
static unsigned char pal_back[4];
static unsigned char write_only[5]; /* $2000 $2001 $2003 $2005 $2006 */
static unsigned char unmapped[8];
static unsigned char sound_back[4];
static unsigned char key_back[4]; /* $4022：测前、写 $80、$84、$C0 之后 */
static unsigned char mouse_back[3]; /* $4023：测前、写 1、写 0 之后 */
static unsigned char misc_back[5]; /* $4026 $4016 $4016 $4017 $4017 */
static unsigned char irq_back[5]; /* $4032 $4033 $4034 $4035 $4036 */
static unsigned char rtc_back[9]; /* 不设地址读一次，再设地址 0-7 各读一次 */

static unsigned char one_current_after_write;
static unsigned char one_pending;
static unsigned char one_current;
static unsigned char one_current_later;
static unsigned char one_pending_cleared;
static unsigned char one_pending_again;
static unsigned int tick_bios;
static unsigned int tick_cpu;
static unsigned int tick_line;

/* 简单延时 */
static void delay(unsigned int count)
{
    while (count-- != 0) {
        __asm__("nop");
    }
}

/* 清掉定时器中断标志并忽略它；$4032 的其余位保持 DOS 的值，不能写 1 去清键盘等中断，否则会丢掉已收到的键盘数据 */
static void timer_clear(void)
{
    REG(IRQ_MASK) = REG(SB_IRQ_MASK_SHADOW) | 0x80;
}

/* 允许定时器中断标志置位 */
static void timer_allow(void)
{
    REG(IRQ_MASK) = REG(SB_IRQ_MASK_SHADOW) & 0x7F;
}

/* 清掉并重新允许定时器中断标志 */
static void timer_rearm(void)
{
    timer_clear();
    timer_allow();
}

static void test_ext(void)
{
    unsigned char i;
    unsigned char ext_saved = REG(SB_PPU_EXT_CTRL_SHADOW);

    for (i = 0; i < 4; ++i) {
        REG(PPU_EXT) = ext_values[i];
        ext_back[i] = REG(PPU_EXT);
    }
    REG(PPU_EXT) = ext_saved;
}

static void test_oam(void)
{
    unsigned char i;

    REG(OAM_ADDRESS) = OAM_TEST_INDEX;
    for (i = 0; i < 4; ++i) {
        REG(OAM_DATA) = oam_values[i];
    }
    for (i = 0; i < 4; ++i) {
        REG(OAM_ADDRESS) = OAM_TEST_INDEX + i;
        oam_back[i] = REG(OAM_DATA);
    }
}

static void test_palette(void)
{
    unsigned char i;

    for (i = 0; i < 4; ++i) {
        REG(PALETTE_TEST + i) = pal_values[i];
    }
    for (i = 0; i < 4; ++i) {
        pal_back[i] = REG(PALETTE_TEST + i);
    }
}

static void test_write_only(void)
{
    write_only[0] = REG(PPU_CTRL);
    write_only[1] = REG(PPU_MASK);
    write_only[2] = REG(OAM_ADDRESS);
    write_only[3] = REG(PPU_SCROLL);
    write_only[4] = REG(PPU_ADDRESS);
}

static void test_unmapped(void)
{
    unsigned char i;

    for (i = 0; i < 8; ++i) {
        unmapped[i] = REG(unmapped_addresses[i]);
    }
}

static void test_sound(void)
{
    unsigned char i;

    for (i = 0; i < 4; ++i) {
        REG(SOUND_ENABLE) = sound_values[i];
        sound_back[i] = REG(SOUND_ENABLE);
    }
    REG(SOUND_ENABLE) = 0xFF;
}

/* 只写 CLK 与中断允许位，不动位 0（方向），避免向键盘发出数据 */
static void test_keyboard(void)
{
    unsigned char key_saved;

    key_saved = REG(KEY_CONTROL);
    key_back[0] = key_saved;
    REG(KEY_CONTROL) = 0x80;
    key_back[1] = REG(KEY_CONTROL);
    REG(KEY_CONTROL) = 0x84;
    key_back[2] = REG(KEY_CONTROL);
    REG(KEY_CONTROL) = 0xC0;
    key_back[3] = REG(KEY_CONTROL);
    REG(KEY_CONTROL) = 0x80;
}

static void test_mouse(void)
{
    unsigned char mouse_saved;

    mouse_saved = REG(MOUSE_SELECT);
    mouse_back[0] = mouse_saved;
    REG(MOUSE_SELECT) = 1;
    mouse_back[1] = REG(MOUSE_SELECT);
    REG(MOUSE_SELECT) = 0;
    mouse_back[2] = REG(MOUSE_SELECT);
    REG(MOUSE_SELECT) = mouse_saved & 1;
}

static void test_misc_reads(void)
{
    misc_back[0] = REG(EXT_IO);
    misc_back[1] = REG(PAD_1);
    misc_back[2] = REG(PAD_1);
    misc_back[3] = REG(PAD_2);
    misc_back[4] = REG(PAD_2);

    irq_back[0] = REG(IRQ_MASK);
    irq_back[1] = REG(IRQ_PENDING);
    irq_back[2] = REG(TIMER_CONTROL);
    irq_back[3] = REG(TIMER_PRESET);
    irq_back[4] = REG(TIMER_CURRENT);
}

/* 实时时钟：只写地址，不写数据 */
static void test_rtc(void)
{
    unsigned char i;

    rtc_back[0] = REG(RTC_DATA);
    for (i = 0; i < 8; ++i) {
        REG(RTC_ADDRESS) = i;
        rtc_back[1 + i] = REG(RTC_DATA);
    }
}

/* 一次计时：不重载，只计一轮；看下溢后的当前值与标志 */
static void test_timer_one_shot(void)
{
    unsigned int n;

    timer_clear();
    REG(TIMER_CONTROL) = 0x00;
    REG(TIMER_PRESET) = 0x40;
    one_current_after_write = REG(TIMER_CURRENT);
    timer_allow();
    REG(TIMER_CONTROL) = 0x8F;
    for (n = 0; n != POLL_LIMIT; ++n) {
        if (REG(IRQ_PENDING) & 0x80) {
            break;
        }
    }
    one_pending = REG(IRQ_PENDING);
    one_current = REG(TIMER_CURRENT);
    delay(DELAY_SHORT);
    one_current_later = REG(TIMER_CURRENT);
    timer_clear();
    one_pending_cleared = REG(IRQ_PENDING);
    timer_allow();
    delay(DELAY_LONG);
    one_pending_again = REG(IRQ_PENDING);
}

/* 连续计时：数 FRAME_COUNT 个垂直消隐里定时器下溢的次数，一次轮询里同时看垂直消隐与定时器标志 */
static unsigned int count_ticks(unsigned char control, unsigned char preset)
{
    unsigned int frames = 0;
    unsigned int ticks = 0;
    unsigned long guard = 0;

    timer_clear();
    REG(TIMER_CONTROL) = 0x00;
    REG(TIMER_PRESET) = preset;
    REG(TIMER_CONTROL) = control;
    timer_rearm();
    wait_vblank();
    while (frames < FRAME_COUNT && guard < 250000UL) {
        ++guard;
        if (REG(PPU_STATUS) & 0x80) {
            ++frames;
        }
        if (REG(IRQ_PENDING) & 0x80) {
            ++ticks;
            timer_rearm();
        }
    }
    REG(TIMER_CONTROL) = 0x00;
    timer_clear();
    return ticks;
}

static void test_timer_bios(void)
{
    tick_bios = count_ticks(0xEF, 0x3C);
}

static void test_timer_cpu(void)
{
    tick_cpu = count_ticks(0xCF, 0xFF);
}

static void test_timer_line(void)
{
    tick_line = count_ticks(0xE0, 0xFF);
}

/* 把定时器与中断屏蔽恢复成 DOS 的值 */
static void restore_timer(void)
{
    REG(TIMER_CONTROL) = 0xEF;
    REG(TIMER_PRESET) = 0x3C;
    REG(IRQ_MASK) = REG(SB_IRQ_MASK_SHADOW);
}

static const char *const step_names[REG_STEPS] = {
    "$2008 readback", "OAM readback", "palette readback", "write-only ports", "unmapped reads", "$4015 sound",
    "$4022 keyboard", "$4023 mouse", "read $4026 etc.", "RTC address", "timer one-shot", "timer BIOS values",
    "timer CPU clock", "timer line clock"
};

const char *reg_step_name(unsigned char index)
{
    return step_names[index];
}

/* 运行第 index 项；两项定时器测试之后把定时器与 $4032 恢复成 DOS 的值 */
void reg_step(unsigned char index)
{
    switch (index) {
    case 0:
        test_ext();
        break;
    case 1:
        test_oam();
        break;
    case 2:
        test_palette();
        break;
    case 3:
        test_write_only();
        break;
    case 4:
        test_unmapped();
        break;
    case 5:
        test_sound();
        break;
    case 6:
        test_keyboard();
        break;
    case 7:
        test_mouse();
        break;
    case 8:
        test_misc_reads();
        break;
    case 9:
        test_rtc();
        break;
    case 10:
        test_timer_one_shot();
        restore_timer();
        break;
    case 11:
        test_timer_bios();
        restore_timer();
        break;
    case 12:
        test_timer_cpu();
        restore_timer();
        break;
    default:
        test_timer_line();
        restore_timer();
        break;
    }
}

/* R1：图形端口读回 */
static void report_ppu(void)
{
    report_line("==== R1 PPU readback");
    report_bytes("$2008 written 00 FF A5 5A, read: ", ext_back, 4);
    report_bytes("OAM written 12 FF FF 34, read: ", oam_back, 4);
    report_bytes("palette $207C-$207F written 3F FF C0 55, read: ", pal_back, 4);
    report_bytes("read $2000 $2001 $2003 $2005 $2006: ", write_only, 5);
    report_bytes("read $2009 $203F $2080 $3FFF: ", unmapped, 4);
    report_bytes("read $4018 $4019 $401F $4037: ", unmapped + 4, 4);
    report_newline();
}

/* R2：外设读回 */
static void report_peripherals(void)
{
    report_line("==== R2 peripherals");
    report_bytes("$4015 written 00 0F 1F FF, read: ", sound_back, 4);
    report_bytes("$4022 before, then written 80 84 C0, read: ", key_back, 4);
    report_bytes("$4023 before, then written 1 and 0, read: ", mouse_back, 3);
    report_bytes("read $4026 $4016 $4016 $4017 $4017: ", misc_back, 5);
    report_bytes("read $4032 $4033 $4034 $4035 $4036: ", irq_back, 5);
    report_bytes("RTC $4081 without address, then address 0-7: ", rtc_back, 9);
    report_newline();
}

/* R3：定时器 */
static void report_timer(void)
{
    report_line("==== R3 timer");
    report_line("one-shot: $4034=8F, preset $40 (IRQ unmasked)");
    report_text("  $4036 right after preset write: ");
    report_hex(one_current_after_write);
    report_newline();
    report_text("  $4033 after underflow: ");
    report_hex(one_pending);
    report_text("  $4036 then: ");
    report_hex(one_current);
    report_text("  $4036 later: ");
    report_hex(one_current_later);
    report_newline();
    report_text("  $4033 after clearing (bit 7 of $4032): ");
    report_hex(one_pending_cleared);
    report_text("  after unmasking and waiting: ");
    report_hex(one_pending_again);
    report_newline();
    report_line("underflows counted in 120 frames:");
    report_text("  $4034=EF preset $3C (BIOS values): ");
    report_uint(tick_bios);
    report_newline();
    report_text("  $4034=CF preset $FF (CPU clock): ");
    report_uint(tick_cpu);
    report_newline();
    report_text("  $4034=E0 preset $FF (line clock): ");
    report_uint(tick_line);
    report_newline();
    report_newline();
}

void reg_report(void)
{
    report_ppu();
    report_peripherals();
    report_timer();
}
