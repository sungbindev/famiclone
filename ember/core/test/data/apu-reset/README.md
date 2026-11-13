# apu-reset

上电与复位之后的 APU 状态测试（作者 Shay Green，即 blargg）：`$4015` 被清除、帧计数器的起点与上次写入的 `$4017`、帧 IRQ 标志被清除、长度计数器启用、上电与复位后寄存器立即可用。

## 来源

文件取自 https://github.com/christopherpow/nes-test-roms 的 `apu_reset/` 目录（下载地址 `https://raw.githubusercontent.com/christopherpow/nes-test-roms/master/apu_reset/文件名`），`readme.txt` 原样放在这里，没有修改。

| 文件 | 内容 |
| --- | --- |
| `4015_cleared.nes` | 上电与复位时 `$4015` 被清除 |
| `4017_timing.nes` | 上电时如同在第一条指令前 9 到 12 个周期写入 `$4017` 为 `$00`；复位时重写上次写入的值 |
| `4017_written.nes` | 上电时 `$4017` 为 `$00`；复位时模式不变 |
| `irq_flag_cleared.nes` | 上电与复位时帧 IRQ 标志被清除 |
| `len_ctrs_enabled.nes` | 上电与复位时长度计数器启用，三角波不受复位影响 |
| `works_immediately.nes` | 上电与复位后 `$4017`、`$4015` 与长度计数器立即可用 |
| `readme.txt` | 作者的说明 |

## 用途

`core/test/hardware/apu-rom-test.cpp` 逐个运行这些 ROM，按 `$6000` 的约定读结果（见 `apu-test/README.md`）。其中要求复位的项，测试先出现状态 `$81`（需要按复位键，且至少延迟 100 毫秒），测试辅助代码（`rom-test-support.hpp`）等待约 8 帧后调用软复位，ROM 在复位后继续运行；用于记录复位次数的变量放在卡带的 RAM 里，软复位后保持。

## 结果

6 个 ROM 全部通过。
