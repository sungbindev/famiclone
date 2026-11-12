/*
 * ------------------------------------------------------------------------
 * 名称：sb-page-alloc.c
 * 说明：sb_page_alloc 的实现：分配连续的扩展 RAM 页。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $7E 子功能 0，页数写 $65A6，结果在 $65A7；出错码在 $6300。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_page_alloc(unsigned char count, unsigned char *first_page)
{
    SB_BYTE_AT(SB_ALLOC_COUNT) = count;
    sb_syscall(SB_FN_PAGE, SB_SUB_PAGE_ALLOC);
    *first_page = SB_BYTE_AT(SB_ALLOC_START);
    return SB_BYTE_AT(SB_DOS_ERROR);
}
