/*
 * ------------------------------------------------------------------------
 * 名称：hwprobe.c
 * 说明：在 SB-DOS 下运行：hwprobe；在真机上补测 hwtest 还没弄清的三处读回：OAM 属性字节的位 4-2、$4015 各位、$4023 的位 7，
 *       结果写成当前目录下的 HWPROBE.TXT。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：hwtest 的程序窗口已经装满，所以补测单独成一个程序；测试在关 NMI、关中断、显示已关的状态下一次跑完，不调用 DOS，
 *       跑完再写报告文件。$4015 与 $4023 测完都恢复；OAM 的精灵 56-63 不恢复，DOS 的下一次 NMI 与重绘会覆盖它们。
 * ------------------------------------------------------------------------
 */
#include <string.h>
#include <sb2000.h>

#define REG(address) SB_BYTE_AT(address)

#define KEY_ESC 0x1B
#define KEY_SETTLE 40000U

/* 精灵 56 的属性字节地址；精灵 56-63 的属性字节地址（4n+2）的位 4-2 依次为 0-7 */
#define OAM_ATTR_FIRST 0xE2
#define OAM_LATCH_INDEX 0xF2
#define OAM_PASSES 4
#define SOUND_VALUES 18
#define MOUSE_VALUES 8

/* ERAM 里的暂存区（BIOS 不用）：传给 DOS 服务的字符串与数据必须在 $8000-$AFFF 或 ERAM，不能在窗口 3（$B000 以上） */
#define ERAM_TEXT ((char *)0x7400)
#define ERAM_LINE ((char *)0x7440)
#define ERAM_TEXT_BYTES 64
#define LINE_BYTES 48

#define REPORT_FILE "HWPROBE.TXT"
#define REPORT_PAGES 1
#define REPORT_CAPACITY 0x1000u
#define ERROR_OVERFLOW 0xFE
#define ERROR_NO_MEMORY 0xFF

static const unsigned char sound_values[SOUND_VALUES] = {
    0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0xFE, 0xFD, 0xFB, 0xF7, 0xEF, 0xDF, 0xBF, 0x7F, 0xFF
};
static const unsigned char mouse_values[MOUSE_VALUES] = { 0x00, 0x01, 0x80, 0x81, 0xFE, 0xFF, 0x80, 0x00 };
static const unsigned char latch_values[4] = { 0x00, 0x1C, 0xFF, 0x10 };

static unsigned char oam_ff[OAM_PASSES][8];
static unsigned char oam_zero[8];
static unsigned char oam_latch[4];
static unsigned char sound_back[2][SOUND_VALUES];
static unsigned char mouse_before;
static unsigned char mouse_back[MOUSE_VALUES];

static unsigned char report_page;
static unsigned char report_overflow;
static unsigned int report_length;
static char *const line_buffer = ERAM_LINE;
static unsigned char line_length;

/* 把文字复制到 ERAM 的暂存区并返回它的地址，供传给 DOS 服务；最多 63 个字符 */
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

/* 属性字节写 FF 或 00 后读回；$2003 每次都重新设置，读之前最后写的 PPU 端口是 $2003 */
static void probe_oam(void)
{
    unsigned char pass;
    unsigned char i;

    for (pass = 0; pass < OAM_PASSES; ++pass) {
        for (i = 0; i < 8; ++i) {
            REG(SB_HW_OAM_ADDRESS) = OAM_ATTR_FIRST + (i << 2);
            REG(SB_HW_OAM_DATA) = 0xFF;
            REG(SB_HW_OAM_ADDRESS) = OAM_ATTR_FIRST + (i << 2);
            oam_ff[pass][i] = REG(SB_HW_OAM_DATA);
        }
    }
    for (i = 0; i < 8; ++i) {
        REG(SB_HW_OAM_ADDRESS) = OAM_ATTR_FIRST + (i << 2);
        REG(SB_HW_OAM_DATA) = 0x00;
        REG(SB_HW_OAM_ADDRESS) = OAM_ATTR_FIRST + (i << 2);
        oam_zero[i] = REG(SB_HW_OAM_DATA);
    }
    /* 同一个属性字节（地址 F2），读之前最后写的 PPU 端口换成 $2005，看读回的位 4-2 是否跟着它的值走 */
    REG(SB_HW_OAM_ADDRESS) = OAM_LATCH_INDEX;
    REG(SB_HW_OAM_DATA) = 0xFF;
    for (i = 0; i < 4; ++i) {
        REG(SB_HW_OAM_ADDRESS) = OAM_LATCH_INDEX;
        REG(SB_HW_PPU_SCROLL) = latch_values[i];
        oam_latch[i] = REG(SB_HW_OAM_DATA);
    }
    (void)REG(SB_HW_PPU_STATUS);
}

/* 依次写 $4015 的 18 个值再读回，连做两遍，看读回是否稳定 */
static void probe_sound(void)
{
    unsigned char pass;
    unsigned char i;

    for (pass = 0; pass < 2; ++pass) {
        for (i = 0; i < SOUND_VALUES; ++i) {
            REG(SB_HW_SOUND_ENABLE) = sound_values[i];
            sound_back[pass][i] = REG(SB_HW_SOUND_ENABLE);
        }
    }
    REG(SB_HW_SOUND_ENABLE) = 0xFF;
}

/* 依次写 $4023 的几个值再读回；测完恢复成测前的位 0 */
static void probe_mouse(void)
{
    unsigned char i;

    mouse_before = REG(SB_HW_MOUSE_SELECT);
    for (i = 0; i < MOUSE_VALUES; ++i) {
        REG(SB_HW_MOUSE_SELECT) = mouse_values[i];
        mouse_back[i] = REG(SB_HW_MOUSE_SELECT);
    }
    REG(SB_HW_MOUSE_SELECT) = mouse_before & 1;
}

/* 申请存放报告的页，成功返回 1 */
static unsigned char report_open(void)
{
    if (sb_page_alloc(REPORT_PAGES, &report_page) != 0) {
        return 0;
    }
    report_overflow = 0;
    report_length = 0;
    line_length = 0;
    return 1;
}

/* 把行缓冲里的文字写进页；超过容量的部分丢掉并记下 */
static void flush(void)
{
    if (line_length == 0) {
        return;
    }
    if (report_length + line_length > REPORT_CAPACITY) {
        report_overflow = 1;
    } else {
        sb_far_write(report_page, report_length, line_buffer, line_length);
        report_length += line_length;
    }
    line_length = 0;
}

static void put_char(char value)
{
    if (line_length == LINE_BYTES) {
        flush();
    }
    line_buffer[line_length++] = value;
}

static void report_text(const char *text)
{
    while (*text != 0) {
        put_char(*text++);
    }
}

static void report_newline(void)
{
    put_char('\r');
    put_char('\n');
}

static void report_line(const char *text)
{
    report_text(text);
    report_newline();
}

/* 标签后面跟若干字节的十六进制（大写），字节之间空一格，最后换行 */
static void report_bytes(const char *label, const unsigned char *bytes, unsigned char count)
{
    static const char digits[] = "0123456789ABCDEF";
    unsigned char i;

    report_text(label);
    for (i = 0; i < count; ++i) {
        put_char(digits[bytes[i] >> 4]);
        put_char(digits[bytes[i] & 15]);
        if (i + 1 < count) {
            put_char(' ');
        }
    }
    report_newline();
}

static void write_report(void)
{
    unsigned char pass;

    report_line("HWPROBE report (values are hex), please send it back");
    report_newline();

    report_line("==== P1 OAM attribute byte, sprites 56-63");
    report_line("addresses E2 E6 EA EE F2 F6 FA FE (bits 4-2 of the address 0-7)");
    for (pass = 0; pass < OAM_PASSES; ++pass) {
        report_bytes("write FF, read: ", oam_ff[pass], 8);
    }
    report_bytes("write 00, read: ", oam_zero, 8);
    report_bytes("address F2, $2005 written 00 1C FF 10 before the read: ", oam_latch, 4);
    report_newline();

    report_line("==== P2 $4015");
    report_line("written 00 01 02 04 08 10 20 40 80 FE FD FB F7 EF DF BF 7F FF");
    report_bytes("pass 1 read: ", sound_back[0], SOUND_VALUES);
    report_bytes("pass 2 read: ", sound_back[1], SOUND_VALUES);
    report_newline();

    report_line("==== P3 $4023");
    report_bytes("before: ", &mouse_before, 1);
    report_line("written 00 01 80 81 FE FF 80 00");
    report_bytes("read: ", mouse_back, MOUSE_VALUES);
}

/* 把报告写成文件并释放页，返回 DOS 出错码（0 成功）；文字超过容量时返回 ERROR_OVERFLOW（文件仍然写出，内容被截断） */
static unsigned char report_close(unsigned int *length)
{
    unsigned char code;

    flush();
    code = sb_file_save(eram_text(REPORT_FILE), report_page, report_length);
    *length = report_length;
    sb_page_free(report_page, REPORT_PAGES);
    if (code == 0 && report_overflow) {
        return ERROR_OVERFLOW;
    }
    return code;
}

/* 按键后先等一会儿（中断开着），让 DOS 把按键的释放码收完，再关中断做测试 */
static void settle_keys(void)
{
    unsigned int n;

    for (n = 0; n != KEY_SETTLE; ++n) {
        __asm__("nop");
    }
}

int main(void)
{
    unsigned char code;
    unsigned int length;

    ui_puts("hwprobe: OAM, $4015, $4023");
    ui_puts("Enter=run Esc=quit");
    if (sb_getkey() == KEY_ESC) {
        return 0;
    }
    if (report_open() == 0) {
        ui_puts("No memory");
        sb_getkey();
        return 0;
    }
    settle_keys();

    sb_display_off();
    REG(SB_HW_PPU_CTRL) = 0;
    __asm__("php");
    __asm__("sei");
    probe_oam();
    probe_sound();
    probe_mouse();
    __asm__("plp");
    sb_display_on();

    write_report();
    code = report_close(&length);
    if (code == 0) {
        ui_puts("Saved HWPROBE.TXT");
    } else {
        ui_puts("Save failed, error code:");
        sb_print_hex(code);
        sb_newline();
    }
    return 0;
}
