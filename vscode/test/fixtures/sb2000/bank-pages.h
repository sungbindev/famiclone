/*
 * ------------------------------------------------------------------------
 * 名称：bank-pages.h
 * 说明：换页测试程序的库页函数声明：库页 1、2、9 里的函数与数据。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：声明放在 SB_BANK_CALLS_BEGIN/END 里，调用它们时由编译器经蹦床换页；bankN_tag 是库页里的数据，要先 sb_bank_select(N) 才能读。
 * ------------------------------------------------------------------------
 */
#ifndef BANK_PAGES_H
#define BANK_PAGES_H

#include <sb2000.h>

SB_BANK_CALLS_BEGIN(1)
unsigned char bank1_value(void);
unsigned int bank1_sum(void);
SB_BANK_CALLS_END()

SB_BANK_CALLS_BEGIN(2)
unsigned char bank2_value(void);
unsigned int bank2_mix(unsigned int a, unsigned char b, unsigned int c);
unsigned int bank2_nested(void);
SB_BANK_CALLS_END()

SB_BANK_CALLS_BEGIN(9)
unsigned char bank9_value(void);
unsigned int bank9_sum(void);
unsigned int bank9_mix(unsigned int a, unsigned char b, unsigned int c);
unsigned int bank9_call_resident(unsigned char n);
SB_BANK_CALLS_END()

/** 各库页里的标记数据，值等于库页号 */
extern const unsigned char bank1_tag;
extern const unsigned char bank2_tag;
extern const unsigned char bank9_tag;

/** 常驻部分的函数，库页里的代码要能调用它 */
unsigned char resident_double(unsigned char n);

#endif
