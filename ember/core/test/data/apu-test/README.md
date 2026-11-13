# apu-test

2010 年版的 APU 测试（作者 Shay Green，即 blargg），每项单独一个 ROM（原仓库的 `rom_singles/`）：长度计数器、长度表、帧 IRQ 标志、时钟抖动、长度计数器的时序、帧 IRQ 标志的时序、DMC 的基本操作与 16 个速率。

## 来源

文件取自 https://github.com/christopherpow/nes-test-roms 的 `apu_test/rom_singles/` 目录（下载地址 `https://raw.githubusercontent.com/christopherpow/nes-test-roms/master/apu_test/rom_singles/文件名`），`readme.txt` 取自 `apu_test/` 目录，原样放在这里，没有修改。

| 文件 | 内容 |
| --- | --- |
| `1-len_ctr.nes` | 四个通道的长度计数器：载入、`$4015` 禁用、写 `$4017` 立即时钟、暂停位 |
| `2-len_table.nes` | 长度表的 32 个值 |
| `3-irq_flag.nes` | 帧 IRQ 标志：各模式下是否置位、读清除、写 `$4017` 的影响 |
| `4-jitter.nes` | 写 `$4017` 的奇偶抖动 |
| `5-len_timing.nes` | 两种模式下前三次长度计数器时钟的时序 |
| `6-irq_flag_timing.nes` | 写 `$4017` 为 `$00` 之后 29831 个周期，帧 IRQ 标志连续三次置位 |
| `7-dmc_basics.nes` | DMC 的启动、重启、停止、长度、IRQ、循环与单字节缓冲 |
| `8-dmc_rates.nes` | DMC 的 16 个速率（NTSC 周期表） |
| `readme.txt` | 作者的说明，含各项失败代码的含义 |

## 用途

`core/test/hardware/apu-rom-test.cpp` 逐个运行这些 ROM，按 `$6000` 的约定读结果：`$6000` 为状态（`$80` 表示运行中，`0` 表示通过），`$6001-$6003` 为签名 `DE B0 61`，`$6004` 起是以 0 结尾的文本；文本同时显示在屏幕上。测试卡带在 `$6000-$7FFF` 带 8K RAM。

## 结果

8 个 ROM 全部通过。
