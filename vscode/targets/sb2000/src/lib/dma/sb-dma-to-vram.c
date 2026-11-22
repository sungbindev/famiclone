/*
 * ------------------------------------------------------------------------
 * 名称：sb-dma-to-vram.c
 * 说明：sb_dma_to_vram 的实现：用 DMA 把扩展 RAM 里的数据复制到显存。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：模式 $C0（启动、高速、目的是显存）。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_dma_to_vram(unsigned long source, unsigned int dest, unsigned int length)
{
    return sbi_dma_run(SB_HW_DMA_START | SB_HW_DMA_FAST, source, dest, length);
}
