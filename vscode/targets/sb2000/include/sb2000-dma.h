/*
 * ------------------------------------------------------------------------
 * 名称：sb2000-dma.h
 * 说明：通用 DMA（$4048-$404F）：把扩展 RAM 里的数据复制到 CPU 地址或显存。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：由 sb2000.h 引入，程序不必单独包含。按真机实测封装：源只能是扩展 RAM，目的是 CPU 地址时只能写 ERAM（$8000 以上的程序窗口写不进去）；
 *       长度寄存器写“字节数 - 1”，源地址高字节的位 7 必须为 1，这些都由函数处理。函数不检查显示状态，复制到显存时调用者要先关显示或在垂直消隐期间调用。
 *       函数返回前会等一段足够 DMA 完成的时间，复制到 CPU 地址时返回后就能直接读。硬件细节见 docs/sb2000/03-硬件说明.md。
 * ------------------------------------------------------------------------
 */
#ifndef SB2000_DMA_H
#define SB2000_DMA_H

/**
 * 把扩展 RAM 的页号与页内偏移换算成 DMA 的源地址。
 *
 * @param page 扩展 RAM 页号，0x80 到 0xFF。
 * @param offset 页内偏移，可以超过一页。
 * @return 源地址，传给 sb_dma_to_cpu、sb_dma_to_vram。
 */
unsigned long sb_dma_page_address(unsigned char page, unsigned int offset);

/**
 * 用 DMA 把扩展 RAM 里的数据复制到 CPU 地址。只有目的在 ERAM（$6000-$7FFF）时有效，目的范围不能越过 $8000。
 *
 * @param source 源地址，用 sb_dma_page_address 换算。
 * @param dest 目的 CPU 地址。
 * @param length 字节数，1 到 $8000。
 * @return 0 成功；SB_ERR_BAD_PARAMETER 长度为 0 或超过 $8000，或目的范围越过 $8000。
 */
unsigned char sb_dma_to_cpu(unsigned long source, unsigned int dest, unsigned int length);

/**
 * 用 DMA 把扩展 RAM 里的数据复制到显存。目的地址不小于 $8000 时是扩展显存，再加 $4300 选的 32K bank，否则是内置字符 RAM。
 * 调用时要关着显示，或在垂直消隐期间。
 *
 * @param source 源地址，用 sb_dma_page_address 换算。
 * @param dest 目的显存地址。
 * @param length 字节数，1 到 $8000。
 * @return 0 成功；SB_ERR_BAD_PARAMETER 长度为 0 或超过 $8000。
 */
unsigned char sb_dma_to_vram(unsigned long source, unsigned int dest, unsigned int length);

#endif
