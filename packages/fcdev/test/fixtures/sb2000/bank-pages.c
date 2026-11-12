/*
 * ------------------------------------------------------------------------
 * 名称：bank-pages.c
 * 说明：换页测试程序的库页：库页 1、2、9 的代码与只读数据。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：库页 1 与 9 各带 3072 字节的数据块（每个字节都是库页号），用来核对整页内容；库页 2 里的函数嵌套调用库页 9 与 1。
 * ------------------------------------------------------------------------
 */
#include "bank-pages.h"

#define R4(x) x, x, x, x
#define R16(x) R4(x), R4(x), R4(x), R4(x)
#define R64(x) R16(x), R16(x), R16(x), R16(x)
#define R256(x) R64(x), R64(x), R64(x), R64(x)
#define R1024(x) R256(x), R256(x), R256(x), R256(x)

/* 常驻部分的函数在 bank.c 里 */

SB_BANK_BEGIN(1)
const unsigned char bank1_tag = 1;
static const unsigned char blob1[3072] = { R1024(1), R1024(1), R1024(1) };

unsigned char bank1_value(void)
{
    return bank1_tag + 0x10;
}

unsigned int bank1_sum(void)
{
    unsigned int index;
    unsigned int sum = 0;
    for (index = 0; index < sizeof(blob1); ++index) {
        sum += blob1[index];
    }
    return sum;
}
SB_BANK_END()

SB_BANK_BEGIN(2)
const unsigned char bank2_tag = 2;

unsigned char bank2_value(void)
{
    return bank2_tag + 0x10;
}

unsigned int bank2_mix(unsigned int a, unsigned char b, unsigned int c)
{
    return a + (unsigned int)b * 100 + c;
}

unsigned int bank2_nested(void)
{
    unsigned int inner = bank9_mix(1000, 2, 30);
    return inner + bank9_value() + bank1_value() + bank2_tag;
}
SB_BANK_END()

SB_BANK_BEGIN(9)
const unsigned char bank9_tag = 9;
static const unsigned char blob9[3072] = { R1024(9), R1024(9), R1024(9) };

unsigned char bank9_value(void)
{
    return bank9_tag + 0x10;
}

unsigned int bank9_sum(void)
{
    unsigned int index;
    unsigned int sum = 0;
    for (index = 0; index < sizeof(blob9); ++index) {
        sum += blob9[index];
    }
    return sum;
}

unsigned int bank9_mix(unsigned int a, unsigned char b, unsigned int c)
{
    return a + (unsigned int)b * 100 + c;
}

unsigned int bank9_call_resident(unsigned char n)
{
    return resident_double(n) + bank9_tag;
}
SB_BANK_END()
