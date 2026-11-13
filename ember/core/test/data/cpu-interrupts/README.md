# cpu-interrupts

2010 年版的 CPU 中断测试（作者 Shay Green，即 blargg），每项单独一个 ROM（原仓库的 `rom_singles/`）：CLI 延迟、NMI 与 BRK 的相互作用、NMI 与 IRQ 的相互作用、IRQ 与 DMA 的相互作用、分支指令对 IRQ 的延迟。其中 IRQ 的来源是 APU 的帧计数器。

## 来源

文件取自 https://github.com/christopherpow/nes-test-roms 的 `cpu_interrupts_v2/rom_singles/` 目录（下载地址 `https://raw.githubusercontent.com/christopherpow/nes-test-roms/master/cpu_interrupts_v2/rom_singles/文件名`），`readme.txt` 取自 `cpu_interrupts_v2/` 目录，原样放在这里，没有修改。

| 文件 | 内容 |
| --- | --- |
| `1-cli_latency.nes` | `CLI`、`SEI`、`PLP` 对 IRQ 响应的延迟 |
| `2-nmi_and_brk.nes` | NMI 与 `BRK` 同时发生时的向量选择 |
| `3-nmi_and_irq.nes` | NMI 与 IRQ 同时发生时的向量选择 |
| `4-irq_and_dma.nes` | IRQ 与精灵 DMA 同时发生 |
| `5-branch_delays_irq.nes` | 分支指令对 IRQ 响应的延迟 |
| `readme.txt` | 作者的说明 |

## 用途

`core/test/hardware/apu-rom-test.cpp` 运行这些 ROM，按 `$6000` 的约定读结果（见 `apu-test/README.md`）。

## 结果

第 1、4、5 项通过。第 2、3 项失败：CPU 核对“NMI 劫持 `BRK` 与 IRQ 的向量”的时序与真机不同，与 APU 无关，登记在 `docs/02-硬件模块.md` 第 12 节“已知差异”，测试 `ApuRomKnownDifferences.NmiHijackingOfBrkAndIrq` 断言当前的失败结果。
