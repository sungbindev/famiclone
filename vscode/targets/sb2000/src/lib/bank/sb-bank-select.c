/*
 * ------------------------------------------------------------------------
 * 名称：sb-bank-select.c
 * 说明：sb_bank_select 的实现：把一个库页换进窗口 2。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：库页 1 是程序文件的第 2 页，第 3 页是栈页，库页 n（n 不小于 2）是第 n+2 页；窗口 2 的页号 = 程序起始页 + 文件里的页序号。
 *       sbi_bank_current 记着窗口 2 当前的库页，程序启动时窗口 2 里就是库页 1。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

/** 窗口 2 当前的库页号，调用蹦床与 sb_bank_current 也读它 */
unsigned char sbi_bank_current = 1;

void sb_bank_select(unsigned char bank)
{
    sbi_bank_current = bank;
    sb_window_map(2, SB_BYTE_AT(SB_PROGRAM_START_PAGE) + (bank == 1 ? 2 : bank + 2));
}
