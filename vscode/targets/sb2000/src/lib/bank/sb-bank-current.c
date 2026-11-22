/*
 * ------------------------------------------------------------------------
 * 名称：sb-bank-current.c
 * 说明：sb_bank_current 的实现：取窗口 2 当前的库页号。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：库页号由 sb_bank_select 与调用蹦床维护，见 sb-bank-select.c。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

extern unsigned char sbi_bank_current;

unsigned char sb_bank_current(void)
{
    return sbi_bank_current;
}
