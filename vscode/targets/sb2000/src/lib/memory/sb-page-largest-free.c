/*
 * ------------------------------------------------------------------------
 * 名称：sb-page-largest-free.c
 * 说明：sb_page_largest_free 的实现：求最长的连续空闲页数。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $7E 子功能 2，结果在 $65A6。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_page_largest_free(void)
{
    sb_syscall(SB_FN_PAGE, SB_SUB_PAGE_LARGEST);
    return SB_BYTE_AT(SB_ALLOC_COUNT);
}
