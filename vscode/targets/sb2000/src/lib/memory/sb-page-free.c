/*
 * ------------------------------------------------------------------------
 * 名称：sb-page-free.c
 * 说明：sb_page_free 的实现：释放连续的扩展 RAM 页。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $7E 子功能 1，起始页号写 $65A7，页数写 $65A6；出错码在 $6300。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_page_free(unsigned char first_page, unsigned char count)
{
    SB_BYTE_AT(SB_ALLOC_START) = first_page;
    SB_BYTE_AT(SB_ALLOC_COUNT) = count;
    sb_syscall(SB_FN_PAGE, SB_SUB_PAGE_FREE);
    return SB_BYTE_AT(SB_DOS_ERROR);
}
