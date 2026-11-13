# vbl-nmi-timing

vblank 标志与 NMI 时序的测试，精确到一个 PPU 时钟：帧长度、vblank 置位与清除的时间、奇偶帧跳点、NMI 抑制与关闭、NMI 的发生时间。作者 Shay Green（blargg）。

## 来源

文件取自 https://github.com/christopherpow/nes-test-roms 的 `vbl_nmi_timing/` 目录（下载地址 `https://raw.githubusercontent.com/christopherpow/nes-test-roms/master/vbl_nmi_timing/文件名`），原样放在这里，没有修改。

| 文件 | 内容 |
| --- | --- |
| `1.frame_basics.nes` | vblank 标志的基本行为与一帧的长度 |
| `2.vbl_timing.nes` | vblank 置位的时间，以及读标志导致本帧不置位的特殊情况 |
| `3.even_odd_frames.nes` | 背景开启时奇数帧跳过一个点 |
| `4.vbl_clear_timing.nes` | vblank 标志清除的时间 |
| `5.nmi_suppression.nes` | 读标志时抑制 NMI 的时间窗口 |
| `6.nmi_disable.nes` | vblank 刚开始时关闭 NMI |
| `7.nmi_timing.nes` | NMI 的发生时间，使能时标志已置位则立即触发 |
| `readme.txt` | 作者的说明，列出各失败码的含义 |

这些测试要求 CPU 与 PPU 处于特定的相位（作者的 NES 有时上电后相位不同而不通过），`fc` 主机型号按它们调定 PPU 相位（`PPU_PHASE_TICKS`）。

## 用途

`core/test/hardware/ppu-rom-test.cpp` 逐个运行这些 ROM。ROM 把结果显示在屏幕上：测试把名称表 0 的图块号当 ASCII 读出，`PASSED` 表示通过，`FAILED #n` 表示第 n 项失败。

## 结果

七个 ROM 全部通过。
