# blargg-apu-tests

2005 年版的 APU 帧计数器与长度计数器测试（作者 Shay Green，即 blargg），共 11 个 ROM：长度计数器的基本操作与长度表、帧 IRQ 标志、时钟抖动、两种模式下长度计数器的时钟时序、帧 IRQ 的置位与响应时序、上电与复位后帧计数器的起点、暂停位与载入的时序。

## 来源

文件取自 https://github.com/christopherpow/nes-test-roms 的 `blargg_apu_2005.07.30/` 目录（下载地址 `https://raw.githubusercontent.com/christopherpow/nes-test-roms/master/blargg_apu_2005.07.30/文件名`），`readme.txt` 与 `tests.txt` 原样放在这里，没有修改。

| 文件 | 内容 |
| --- | --- |
| `01.len_ctr.nes` | 长度计数器的基本操作 |
| `02.len_table.nes` | 长度表的 32 个值 |
| `03.irq_flag.nes` | 帧 IRQ 标志的基本操作 |
| `04.clock_jitter.nes` | 写 `$4017` 的 3 或 4 周期延迟（时钟抖动），以及帧 IRQ 的基本时序 |
| `05.len_timing_mode0.nes` | 4 步模式下长度计数器时钟的时序 |
| `06.len_timing_mode1.nes` | 5 步模式下长度计数器时钟的时序 |
| `07.irq_flag_timing.nes` | 写 `$4017` 为 `$00` 之后 29831 个周期，帧 IRQ 标志连续三次置位 |
| `08.irq_timing.nes` | 帧 IRQ 处理程序最早在写 `$4017` 之后 29833 个周期被调用 |
| `09.reset_timing.nes` | 上电与复位后，帧计数器如同在第一条指令前 9 到 12 个周期写入过 `$00` |
| `10.len_halt_timing.nes` | 暂停位的写入延迟一个周期生效 |
| `11.len_reload_timing.nes` | 长度计数器载入与时钟在同一周期时的结果 |
| `readme.txt`、`tests.txt` | 作者的说明，含帧计数器各步的精确时序与各项失败代码的含义 |

## 用途

`core/test/hardware/apu-rom-test.cpp` 逐个运行这些 ROM。结果显示在屏幕上（名称表 0 的图块号就是 ASCII 字符），`$01` 表示全部通过，其他数字是失败的项（含义见 `tests.txt`）。卡带用官方的 nrom 插件装配。

## 结果

11 个 ROM 全部通过。
