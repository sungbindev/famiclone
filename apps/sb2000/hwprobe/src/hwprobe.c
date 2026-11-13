/*
 * ------------------------------------------------------------------------
 * 名称：hwprobe.c
 * 说明：在 SB-DOS 下运行：hwprobe；在真机上找出让 SB2000 丢失颜色输出的操作（丢失后软复位不恢复，要断电重开）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：hwtest 的第 12 项（定时器按 BIOS 值连续运行 120 帧）跑完后颜色丢失，这里把那一项拆成几个更小的实验，逐项运行；
 *       每项之后把文字前景色依次换成红、绿、蓝，文字变灰就是刚运行的那一项丢了颜色。前一项没问题才会运行下一项，
 *       所以变灰之后要断电重开，再用 Space 跳过已经确认没问题的项，继续往后试。
 *       屏幕上只显示简短的提示，每行不超过 31 个 ASCII 字符（屏幕 32 列）。
 * ------------------------------------------------------------------------
 */
#include <string.h>
#include <sb2000.h>

#define REG(address) SB_BYTE_AT(address)

#define PPU_CTRL 0x2000
#define PPU_STATUS 0x2002
#define PALETTE 0x2040
#define IRQ_MASK 0x4032
#define IRQ_PENDING 0x4033
#define TIMER_CONTROL 0x4034
#define TIMER_PRESET 0x4035

#define KEY_ESC 0x1B
#define KEY_SPACE 0x20

/* ERAM 里的暂存区（BIOS 不用）：传给 DOS 服务的文字必须在 $8000-$AFFF 或 ERAM，不能在窗口 3（$B000 以上） */
#define ERAM_TEXT ((char *)0x7400)
#define ERAM_PALETTE ((unsigned char *)0x74C0)
#define ERAM_TEXT_BYTES 64

#define PALETTE_ENTRIES 64
#define STEP_TOTAL 8
#define STALL_ROUNDS 5
#define KEY_SETTLE 40000U
#define FRAME_COUNT 120
#define GUARD_LIMIT 250000UL

/* 探测色：红、绿、蓝 */
static const unsigned char probe_colors[3] = { 0x36, 0x3A, 0x31 };

static const char *const step_names[STEP_TOTAL] = {
    "stall, display ON",
    "stall, display OFF",
    "stall, IRQ on",
    "poll $2002 x120",
    "timer, no reads",
    "timer + poll",
    "timer CPU clock",
    "timer line clock"
};

/* 把文字复制到 ERAM 的暂存区并返回它的地址，最多 63 个字符 */
static const char *eram_text(const char *text)
{
    unsigned char i;

    for (i = 0; i < ERAM_TEXT_BYTES - 1 && text[i] != 0; ++i) {
        ERAM_TEXT[i] = text[i];
    }
    ERAM_TEXT[i] = 0;
    return ERAM_TEXT;
}

static void ui_puts(const char *text)
{
    sb_puts(eram_text(text));
}

static void ui_print(const char *text)
{
    sb_print(eram_text(text));
}

/* 空等约 3 秒，不访问任何硬件寄存器 */
static void stall(void)
{
    unsigned char round;
    unsigned int n;

    for (round = 0; round < STALL_ROUNDS; ++round) {
        for (n = 0; n != 0xFFFFU; ++n) {
            __asm__("nop");
        }
    }
}

/* 按键后先等一会儿（中断开着），让 DOS 把按键的释放码收完 */
static void settle_keys(void)
{
    unsigned int n;

    for (n = 0; n != KEY_SETTLE; ++n) {
        __asm__("nop");
    }
}

/* 只写 $4032 的位 7，其余位保持 DOS 的值，避免丢掉键盘等中断 */
static void timer_clear(void)
{
    REG(IRQ_MASK) = REG(SB_IRQ_MASK_SHADOW) | 0x80;
}

static void timer_allow(void)
{
    REG(IRQ_MASK) = REG(SB_IRQ_MASK_SHADOW) & 0x7F;
}

static void timer_restore(void)
{
    REG(TIMER_CONTROL) = 0xEF;
    REG(TIMER_PRESET) = 0x3C;
    REG(IRQ_MASK) = REG(SB_IRQ_MASK_SHADOW);
}

/* 轮询 $2002 位 7，等到下一次垂直消隐开始；读 $2002 会清掉位 7 */
static void wait_vblank(void)
{
    unsigned int n;

    for (n = 0; n != 0xFFFFU; ++n) {
        if (REG(PPU_STATUS) & 0x80) {
            return;
        }
    }
}

/* 只轮询 $2002，数 FRAME_COUNT 个垂直消隐 */
static void poll_frames(void)
{
    unsigned int frames = 0;
    unsigned long guard = 0;

    wait_vblank();
    while (frames < FRAME_COUNT && guard < GUARD_LIMIT) {
        ++guard;
        if (REG(PPU_STATUS) & 0x80) {
            ++frames;
        }
    }
}

/* 定时器按 BIOS 的值运行，空等约 3 秒，不读任何寄存器 */
static void timer_stall(void)
{
    REG(TIMER_CONTROL) = 0x00;
    REG(TIMER_PRESET) = 0x3C;
    REG(TIMER_CONTROL) = 0xEF;
    stall();
    timer_restore();
}

/* 与 hwtest 的定时器计数相同：数 FRAME_COUNT 个垂直消隐里定时器下溢的次数 */
static void count_ticks(unsigned char control, unsigned char preset)
{
    unsigned int frames = 0;
    unsigned long guard = 0;

    timer_clear();
    REG(TIMER_CONTROL) = 0x00;
    REG(TIMER_PRESET) = preset;
    REG(TIMER_CONTROL) = control;
    timer_clear();
    timer_allow();
    wait_vblank();
    while (frames < FRAME_COUNT && guard < GUARD_LIMIT) {
        ++guard;
        if (REG(PPU_STATUS) & 0x80) {
            ++frames;
        }
        if (REG(IRQ_PENDING) & 0x80) {
            timer_clear();
            timer_allow();
        }
    }
    REG(TIMER_CONTROL) = 0x00;
    timer_clear();
    timer_restore();
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

/* 运行第 index 项：第 0 项保持显示开着，第 2 项保持中断开着，其余关显示、关中断；NMI 都关掉。
   之后把每组调色板的后 3 项改成探测色，屏幕变灰就说明这一项丢了颜色 */
static void run_step(unsigned char index)
{
    unsigned char i;
    unsigned char control = REG(SB_PPU_CTRL_SHADOW);

    if (index != 0) {
        sb_display_off();
    }
    REG(PPU_CTRL) = 0;
    __asm__("php");
    if (index != 2) {
        __asm__("sei");
    }

    switch (index) {
    case 0:
    case 1:
    case 2:
        stall();
        break;
    case 3:
        poll_frames();
        break;
    case 4:
        timer_stall();
        break;
    case 5:
        count_ticks(0xEF, 0x3C);
        break;
    case 6:
        count_ticks(0xCF, 0xFF);
        break;
    default:
        count_ticks(0xE0, 0xFF);
        break;
    }

    __asm__("plp");
    if (index == 0) {
        REG(PPU_CTRL) = control;
        sb_display_off();
    }
    for (i = 0; i < PALETTE_ENTRIES; ++i) {
        if ((i & 3) != 0) {
            REG(PALETTE + i) = probe_colors[index % 3];
        }
    }
    sb_display_on();
}

int main(void)
{
    unsigned char step;
    unsigned char key;

    ui_puts("hwprobe: what kills color");
    ui_puts("Text turns R, G, B per step");
    ui_puts("Gray = lost: note the step,");
    ui_puts("then power off and on.");
    ui_puts("Enter=run Space=skip Esc=quit");
    palette_save();
    for (step = 0; step < STEP_TOTAL; ++step) {
        ui_print("Step ");
        sb_print_uint(step + 1);
        ui_print(": ");
        ui_print(step_names[step]);
        key = sb_getkey();
        if (key == KEY_ESC) {
            sb_newline();
            break;
        }
        if (key == KEY_SPACE) {
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
    ui_puts("Done. Press a key.");
    sb_getkey();
    return 0;
}
