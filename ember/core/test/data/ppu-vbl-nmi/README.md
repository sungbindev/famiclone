# ppu-vbl-nmi

2010 年版的 PPU vblank 与 NMI 测试，每项单独一个 ROM（原仓库的 `rom_singles/`），精确到一个 PPU 时钟：vblank 基本行为、置位与清除的时间、NMI 控制、NMI 的时序、读标志的抑制、NMI 开关的时间窗口、奇偶帧跳点及其时序。作者 Shay Green（blargg）。

## 来源

文件取自 https://github.com/christopherpow/nes-test-roms 的 `ppu_vbl_nmi/rom_singles/` 目录（下载地址 `https://raw.githubusercontent.com/christopherpow/nes-test-roms/master/ppu_vbl_nmi/rom_singles/文件名`），`readme.txt` 取自 `ppu_vbl_nmi/` 目录，原样放在这里，没有修改。

| 文件 | 内容 |
| --- | --- |
| `01-vbl_basics.nes` | vblank 的基本行为与时长 |
| `02-vbl_set_time.nes` | vblank 置位的时间，逐点输出 |
| `03-vbl_clear_time.nes` | vblank 清除的时间，逐点输出 |
| `04-nmi_control.nes` | `$2000` 位 7 使能 NMI 的各种情形 |
| `05-nmi_timing.nes` | NMI 发生在第几条指令之后，逐点输出 |
| `06-suppression.nes` | 在 vblank 置位附近读 `$2002` 对标志与 NMI 的影响 |
| `07-nmi_on_timing.nes` | 在 vblank 清除附近使能 NMI |
| `08-nmi_off_timing.nes` | 在 vblank 置位附近关闭 NMI |
| `09-even_odd_frames.nes` | 背景开启时奇数帧跳过一个点 |
| `10-even_odd_timing.nes` | 跳点的时间相对背景开关的位置 |
| `readme.txt` | 作者的说明，含各项的正确输出 |

## 用途

`core/test/hardware/ppu-rom-test.cpp` 逐个运行这些 ROM，并核对输出的表格与 `readme.txt` 里的正确输出一致。ROM 按 `$6000` 的约定报告结果：`$6000` 为状态（`$80` 表示运行中，`0` 表示通过），`$6001-$6003` 为签名 `DE B0 61`，`$6004` 起是以 0 结尾的文本；这些测试同时把文本显示在屏幕上。测试卡带在 `$6000-$7FFF` 带 8K RAM。

## 结果

十个 ROM 全部通过。其中奇偶帧跳点的判断使用前一个点结束时的背景开关（背景开关生效有 1 个点的延迟），是 `10-even_odd_timing.nes` 要求的。
