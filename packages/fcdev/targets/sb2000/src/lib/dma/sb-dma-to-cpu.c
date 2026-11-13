/*
 * ------------------------------------------------------------------------
 * 名称：sb-dma-to-cpu.c
 * 说明：sb_dma_to_cpu 的实现：用 DMA 把扩展 RAM 里的数据复制到 CPU 地址。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：真机实测目的在程序窗口（$8000 以上）时写不进去，所以拒绝越过 $8000 的目的范围。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_dma_to_cpu(unsigned long source, unsigned int dest, unsigned int length)
{
    if ((unsigned long)dest + length > 0x8000UL) {
        return SB_ERR_BAD_PARAMETER;
    }
    return sbi_dma_run(SB_HW_DMA_START | SB_HW_DMA_FAST | SB_HW_DMA_TO_CPU, source, dest, length);
}
