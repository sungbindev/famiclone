/*
 * ------------------------------------------------------------------------
 * 名称：math.c
 * 说明：测试 sb2000-math.h 里的数学函数：伪随机数、16 位除法、时间文字、十进制文字与浮点数、40 位整数、直线步进、3 字节整数运算。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：期望值都由独立的算术得出：浮点数 1.5 是阶码 1、尾数 $C0（0.75）；-2.25 是阶码 2、符号 $FF、尾数 $90（0.5625）。
 * ------------------------------------------------------------------------
 */
#include <string.h>
#include <sb2000.h>
#include "sb-test.h"

/** 文字缓冲 */
static char text[16];
/** 40 位整数缓冲 */
static unsigned char big[6];

int main(void)
{
    unsigned long first;
    unsigned long second;
    unsigned int quotient;
    unsigned char remainder;
    unsigned char length;
    unsigned char sign;
    sb_float_t a;
    sb_float_t b;
    sb_float_t r;
    sb_line_t line;
    sb_int24_t x;
    sb_int24_t y;
    sb_int24_t z;
    sb_int24_t rem;

    /* 伪随机数：同一种子给出同一序列，连续两次不同，种子 0 也能用，预热会改变寄存器 */
    sb_random_seed(1);
    first = sb_random_next();
    second = sb_random_next();
    SB_CHECK(first != second && first != 1);
    sb_random_seed(1);
    SB_CHECK(sb_random_next() == first && sb_random_next() == second);
    sb_random_seed(0);
    SB_CHECK(sb_random_next() != 0);
    sb_random_seed(1);
    sb_random_warm_up();
    SB_CHECK(sb_random_next() != first);

    /* 16 位除以 8 位 */
    SB_CHECK(sb_math_div16(1000, 7, &quotient, &remainder) == 0 && quotient == 142 && remainder == 6);
    SB_CHECK(sb_math_div16(65535, 255, &quotient, &remainder) == 0 && quotient == 257 && remainder == 0);
    SB_CHECK_EQ(sb_math_div16(5, 0, &quotient, &remainder), 0xFF);

    /* 时间文字 */
    sb_math_time_text(13, 5, 9, text);
    SB_CHECK(strcmp(text, "13:05:09") == 0);

    /* 十进制文字与浮点数 */
    sb_float_from_text(" 000000001.50", &a);
    SB_CHECK(a.exponent == 1 && a.sign == 0 && a.mantissa[0] == 0xC0 && a.mantissa[1] == 0 && a.mantissa[4] == 0);
    sb_float_from_text("-000000002.25", &b);
    SB_CHECK(b.exponent == 2 && b.sign == 0xFF && b.mantissa[0] == 0x90 && b.mantissa[1] == 0);
    sb_float_to_text(&a, text);
    SB_CHECK(strcmp(text, " 1.5000000000") == 0);
    sb_float_add(&a, &b, &r);
    sb_float_to_text(&r, text);
    SB_CHECK(strcmp(text, "-0.7500000000") == 0);
    sb_float_sub(&a, &b, &r);
    sb_float_to_text(&r, text);
    SB_CHECK(strcmp(text, " 3.7500000000") == 0);
    sb_float_mul(&a, &b, &r);
    sb_float_to_text(&r, text);
    SB_CHECK(strcmp(text, "-3.3750000000") == 0);
    sb_float_div(&a, &b, &r);
    sb_float_to_text(&r, text);
    SB_CHECK(strcmp(text, "-0.6666666666") == 0);
    /* 结果可以与操作数是同一个变量 */
    sb_float_add(&a, &a, &a);
    sb_float_to_text(&a, text);
    SB_CHECK(strcmp(text, " 3.0000000000") == 0);

    /* 40 位整数 */
    sb_int40_from_text("12345", 5, big);
    SB_CHECK(big[0] == 0 && big[1] == 0 && big[2] == 0 && big[3] == 0x30 && big[4] == 0x39);
    SB_CHECK_EQ(sb_int40_to_text(big, text, &length), 0);
    SB_CHECK(length == 5 && strcmp(text, "1234500000000") == 0);
    big[0] = 0xE8; big[1] = 0xD4; big[2] = 0xA5; big[3] = 0x10; big[4] = 0x00;
    SB_CHECK_EQ(sb_int40_to_text(big, text, &length), 0xFF);

    /* 直线步进：主轴 8，走 4 步到半程，步数不小于主轴长度时落在终点 */
    memset(&line, 0, sizeof(line));
    line.end_x = 8;
    line.end_y = 4;
    sb_line_step(&line, 4);
    SB_CHECK_EQ(line.x, 4);
    SB_CHECK_EQ(line.y, 2);
    memset(&line, 0, sizeof(line));
    line.end_x = 8;
    line.end_y = 4;
    sb_line_step(&line, 20);
    SB_CHECK_EQ(line.x, 8);
    SB_CHECK_EQ(line.y, 4);

    /* 3 字节整数：加（含进位与异号）、减、乘、除 */
    x.sign = 0; x.value[0] = 0x01; x.value[1] = 0x86; x.value[2] = 0xA0;
    y.sign = 0; y.value[0] = 0x00; y.value[1] = 0x5B; y.value[2] = 0xA0;
    SB_CHECK(sb_int24_add(&x, &y, &z) == 0 && z.sign == 0 && z.value[0] == 0x01 && z.value[1] == 0xE2 && z.value[2] == 0x40);
    x.value[0] = 0xFF; x.value[1] = 0xFF; x.value[2] = 0xFF;
    y.value[0] = 0; y.value[1] = 0; y.value[2] = 1;
    SB_CHECK_EQ(sb_int24_add(&x, &y, &z), 1);
    x.value[0] = 0; x.value[1] = 0; x.value[2] = 5;
    y.sign = 0xFF; y.value[0] = 0; y.value[1] = 0; y.value[2] = 7;
    SB_CHECK(sb_int24_add(&x, &y, &z) == 0 && z.sign == 0xFF && z.value[0] == 0 && z.value[1] == 0 && z.value[2] == 2);
    y.sign = 0;
    SB_CHECK(sb_int24_sub(&x, &y, &z) == 0 && z.sign == 0xFF && z.value[2] == 2);
    x.value[0] = 0; x.value[1] = 0x03; x.value[2] = 0xE8;
    y.value[0] = 0; y.value[1] = 0x03; y.value[2] = 0xE8;
    sb_int24_mul(&x, &y, &sign, big);
    SB_CHECK(sign == 0 && big[0] == 0 && big[1] == 0 && big[2] == 0 && big[3] == 0x0F && big[4] == 0x42 && big[5] == 0x40);
    y.sign = 0xFF;
    sb_int24_mul(&x, &y, &sign, big);
    SB_CHECK(sign == 0xFF);
    x.value[0] = 0; x.value[1] = 0; x.value[2] = 100;
    y.sign = 0; y.value[0] = 0; y.value[1] = 0; y.value[2] = 7;
    SB_CHECK(sb_int24_div(&x, &y, &z, &rem) == 0 && z.value[2] == 14 && rem.value[2] == 2);
    y.value[2] = 0;
    SB_CHECK_EQ(sb_int24_div(&x, &y, &z, &rem), 1);

    sb_test_done();
    return 0;
}
