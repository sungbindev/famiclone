/*
 * ------------------------------------------------------------------------
 * 名称：sb-dma-page-address.c
 * 说明：sb_dma_page_address 的实现：扩展 RAM 的页号与偏移换算成 DMA 源地址。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：扩展 RAM 页号从 $80 起，每页 4K。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned long sb_dma_page_address(unsigned char page, unsigned int offset)
{
    return ((unsigned long)(unsigned char)(page - 0x80) << 12) + offset;
}
