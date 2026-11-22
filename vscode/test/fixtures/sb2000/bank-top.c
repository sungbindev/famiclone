/*
 * ------------------------------------------------------------------------
 * 名称：bank-top.c
 * 说明：换页测试程序：库页个数取上限 113，最高的库页 113 里的函数也能调用。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：程序文件 116 页（475 136 字节），DOS 装入后只剩 1 个空闲页。
 * ------------------------------------------------------------------------
 */
#include <sb2000.h>
#include "sb-test.h"

SB_BANK_CALLS_BEGIN(113)
unsigned char bank113_value(void);
SB_BANK_CALLS_END()

SB_BANK_BEGIN(113)
const unsigned char bank113_tag = 113;

unsigned char bank113_value(void)
{
    return bank113_tag + 1;
}
SB_BANK_END()

extern const unsigned char bank113_tag;

int main(void)
{
    SB_CHECK_EQ(SB_BYTE_AT(SB_PROGRAM_PAGE_COUNT), 116);
    SB_CHECK_EQ(sb_bank_current(), 1);
    SB_CHECK_EQ(bank113_value(), 114);
    SB_CHECK_EQ(sb_bank_current(), 1);
    sb_bank_select(113);
    SB_CHECK_EQ(bank113_tag, 113);
    sb_bank_select(1);
    sb_test_done();
    return 0;
}
