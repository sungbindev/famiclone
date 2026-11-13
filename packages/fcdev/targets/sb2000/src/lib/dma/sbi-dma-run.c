/*
 * ------------------------------------------------------------------------
 * 名称：sbi-dma-run.c
 * 说明：sbi_dma_run 的实现：启动一次通用 DMA 并等它完成。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：库内部使用；等待按 890KB/s 的速率估算，每个字节循环一次，足够 DMA 完成。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sbi_dma_run(unsigned char mode, unsigned long source, unsigned int dest, unsigned int length)
{
    unsigned int last;
    unsigned int wait;

    if (length == 0 || length > 0x8000u) {
        return SB_ERR_BAD_PARAMETER;
    }
    last = length - 1;
    SB_BYTE_AT(SB_HW_DMA_SOURCE_BANK) = SB_HW_DMA_SOURCE_RAM | (unsigned char)((source >> 15) & 0x0F);
    SB_BYTE_AT(SB_HW_DMA_SOURCE_LOW) = (unsigned char)source;
    SB_BYTE_AT(SB_HW_DMA_SOURCE_HIGH) = ((unsigned char)(source >> 8) & 0x7F) | SB_HW_DMA_SOURCE_A15;
    SB_BYTE_AT(SB_HW_DMA_DEST_LOW) = (unsigned char)dest;
    SB_BYTE_AT(SB_HW_DMA_DEST_HIGH) = (unsigned char)(dest >> 8);
    SB_BYTE_AT(SB_HW_DMA_LENGTH_LOW) = (unsigned char)last;
    SB_BYTE_AT(SB_HW_DMA_LENGTH_HIGH) = (unsigned char)(last >> 8);
    SB_BYTE_AT(SB_HW_DMA_MODE) = mode;
    for (wait = length; wait != 0; --wait) {
        __asm__("nop");
    }
    return 0;
}
