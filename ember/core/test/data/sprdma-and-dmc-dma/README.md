# sprdma-and-dmc-dma

精灵 DMA 与 DMC DMA 相互影响的测试（作者 Shay Green，即 blargg）：精灵 DMA 进行中发生 DMC 取样时，精灵 DMA 的总时间会被延后；ROM 输出精灵 DMA 总共花的周期数。

## 来源

文件取自 https://github.com/christopherpow/nes-test-roms 的 `sprdma_and_dmc_dma/` 目录（下载地址 `https://raw.githubusercontent.com/christopherpow/nes-test-roms/master/sprdma_and_dmc_dma/文件名`），原样放在这里，没有修改。

| 文件 | 内容 |
| --- | --- |
| `sprdma_and_dmc_dma.nes` | 精灵 DMA 进行中发生 DMC DMA，输出精灵 DMA 总共花的周期数 |
| `sprdma_and_dmc_dma_512.nes` | 同一测试的另一个版本（原仓库没有说明两者的区别） |

## 用途

`core/test/hardware/apu-rom-test.cpp` 的 `ApuRomKnownDifferences.DmcDmaDoesNotDelaySpriteDma` 运行两个 ROM，按 `$6000` 的约定读结果（`$6000` 为 1，文本含 `Failed`）。

## 结果

两个 ROM 都失败，登记在 `docs/02-硬件模块.md` 第 12 节“已知差异”：精灵 DMA 与 DMC DMA 互不知道对方，精灵 DMA 不会被 DMC 取样延后。
