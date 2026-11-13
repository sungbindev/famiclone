# dmc-dma-during-read4

DMC 取样 DMA 发生在 CPU 读 `$2007`、`$4016` 等寄存器的时候的测试（作者 Shay Green，即 blargg）：真机上，被 DMA 暂停的 CPU 会在暂停的前几个周期里重复读它正要读的地址，所以这些读有副作用的寄存器会被多读几次。

## 来源

文件取自 https://github.com/christopherpow/nes-test-roms 的 `dmc_dma_during_read4/` 目录（下载地址 `https://raw.githubusercontent.com/christopherpow/nes-test-roms/master/dmc_dma_during_read4/文件名`），`readme.txt` 取自该目录的 `source/readme.txt`（作者对测试框架的说明），原样放在这里，没有修改。

| 文件 | 内容 |
| --- | --- |
| `dma_2007_read.nes` | DMC DMA 与 `$2007` 的读 |
| `dma_2007_write.nes` | DMC DMA 与 `$2007` 的写 |
| `dma_4016_read.nes` | DMC DMA 与 `$4016` 的读 |
| `double_2007_read.nes` | DMC DMA 与连续两次 `$2007` 的读 |
| `read_write_2007.nes` | DMC DMA 与 `$2007` 的读与写交替 |
| `readme.txt` | 测试框架的说明 |

## 用途

这些 ROM 用旧框架，不写 `$6000`：把输出的表格显示在屏幕上，输出的校验和与预期一致时在最后显示测试名与 `Passed`，不一致时显示校验和（8 位十六进制数），`dma_4016_read` 显示 `Failed`。`core/test/hardware/apu-rom-test.cpp` 运行 2 个能通过的 ROM；另外 3 个依赖“暂停时重复读 CPU 的地址”，登记在 `docs/02-硬件模块.md` 第 12 节“已知差异”，测试 `ApuRomKnownDifferences.DmcDmaDoesNotRepeatTheCpuRead` 断言当前的失败结果。

## 结果

`dma_2007_write`、`read_write_2007` 通过；`dma_2007_read`、`double_2007_read`、`dma_4016_read` 失败（已知差异）。
