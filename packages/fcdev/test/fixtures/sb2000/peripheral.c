/*
 * ------------------------------------------------------------------------
 * 名称：peripheral.c
 * 说明：测试 sb2000-printer.h 与 sb2000-ime.h：打印机的复位、走纸、定位、页长与文字，输入法的初始化、选择、状态、全半角、按键处理与内码转换。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：打印机是模拟器里的虚拟 ESC/P 打印机，检查的是函数能返回、页长计数与并口初始化标志；输入法的按键由测试脚本在停靠点注入。
 * ------------------------------------------------------------------------
 */
#include <string.h>
#include <sb2000.h>
#include "sb-test.h"

/** 并口初始化标志，初始化后为 1 */
#define PRINTER_PORT_READY 0x038F

int main(void)
{
    sb_ime_config_t config;
    sb_ime_result_t result;
    sb_printer_style_t style;
    unsigned char allow;
    unsigned char flags;
    unsigned int code;
    unsigned int left;
    char text[8];

    /* 打印机：页长计数按纸张类型装入 */
    sb_printer_page_length(1);
    SB_CHECK_EQ(sb_printer_page_left(), 2523);
    sb_printer_page_length(0);
    SB_CHECK_EQ(sb_printer_page_left(), 1900);

    /* 初始化并口、复位、回左端、进纸、定位、发字节、位图模式、回车：都能返回 */
    sb_printer_init_port();
    SB_CHECK_EQ(SB_BYTE_AT(PRINTER_PORT_READY), 1);
    sb_printer_reset();
    sb_printer_home();
    sb_printer_feed(2);
    sb_printer_position(0, 100);
    sb_printer_position(1, 50);
    sb_printer_send_byte(0x41);
    sb_printer_graphics_mode();
    sb_printer_carriage_return();
    SB_CHECK(1);

    /* 打印一行文字：页长计数减少 */
    style.width = 0;
    style.dots = 0;
    style.gap = 0;
    style.feed = 0x18;
    style.transform = 0;
    sb_printer_page_length(1);
    left = sb_printer_page_left();
    sb_printer_text("OK\r\n", &style);
    SB_CHECK(sb_printer_page_left() < left);

    /* 输入法：初始化为英文，选择、状态、全半角、暂停与恢复 */
    config.row = 28;
    config.tile = 0x6F80;
    config.subfunction = 0;
    config.color = 0;
    config.method = SB_IME_ENGLISH;
    config.allow = 0xFF;
    config.flags = 0;
    SB_CHECK_EQ(sb_ime_init(&config), 0);
    SB_CHECK_EQ(sb_ime_get_method(), SB_IME_ENGLISH);
    SB_CHECK_EQ(sb_ime_select(SB_IME_PINYIN, 0), 0);
    SB_CHECK_EQ(sb_ime_get_method(), SB_IME_PINYIN);
    SB_CHECK_EQ(sb_ime_select(10, 0), 1);
    SB_CHECK_EQ(sb_ime_select(7, 0), 1);
    SB_CHECK_EQ(sb_ime_get_method(), SB_IME_PINYIN);
    sb_ime_set_state(0x7F, 0x20);
    sb_ime_get_state(&allow, &flags);
    SB_CHECK(allow == 0x7F && flags == 0x20);
    sb_ime_set_width(1);
    sb_ime_get_state(&allow, &flags);
    SB_CHECK_EQ(flags & 0x02, 0x02);
    sb_ime_set_width(0);
    sb_ime_get_state(&allow, &flags);
    SB_CHECK_EQ(flags & 0x02, 0);
    sb_ime_suspend();
    sb_ime_get_state(&allow, &flags);
    SB_CHECK_EQ(flags & 0x04, 0x04);
    sb_ime_resume();
    sb_ime_get_state(&allow, &flags);
    SB_CHECK_EQ(flags & 0x04, 0);

    /* 内码与十六进制字符、输入字符转内码 */
    sb_ime_hex(0xBABA, text);
    SB_CHECK(strcmp(text, "baba") == 0);
    SB_CHECK(sb_ime_parse("1234", &code) == 0 && code == 0xACC2);
    SB_CHECK(sb_ime_parse("b0a1", &code) == 0 && code == 0xB0A1);
    SB_CHECK_EQ(sb_ime_parse("zzzz", &code), 1);

    /* 按键处理：队列空时没有结果；全拼下输入 a 再按 1 选第一个候选字（阿，内码 $B0A2） */
    SB_CHECK_EQ(sb_ime_select(SB_IME_PINYIN, 0), 0);
    SB_CHECK_EQ(SB_BYTE_AT(0x6900) & 0x0F, SB_IME_PINYIN);
    SB_CHECK_EQ(sb_ime_key(&result), 0);
    sb_wait_frames(30);
    sb_test_sync();
    SB_CHECK_EQ(sb_ime_key_wait(&result), 2);
    SB_CHECK(result.output[0] == 0xB0 && result.output[1] == 0xA2);

    sb_test_done();
    return 0;
}
