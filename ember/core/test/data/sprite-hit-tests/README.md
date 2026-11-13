# sprite-hit-tests

精灵 0 命中标志的测试：基本行为、对齐、四个角、翻转、左侧遮罩、右边缘、屏幕底部、8x16 精灵、命中的时序与顺序、边缘时序。作者 Shay Green（blargg），2005-10-05 版本。

## 来源

文件取自 https://github.com/christopherpow/nes-test-roms 的 `sprite_hit_tests_2005.10.05/` 目录（下载地址 `https://raw.githubusercontent.com/christopherpow/nes-test-roms/master/sprite_hit_tests_2005.10.05/文件名`），原样放在这里，没有修改。

| 文件 | 内容 |
| --- | --- |
| `01.basics.nes` | 命中标志的基本行为 |
| `02.alignment.nes` | 精灵与背景像素对齐 |
| `03.corners.nes` | 重叠区域四个角上的命中 |
| `04.flip.nes` | 精灵翻转时的命中 |
| `05.left_clip.nes` | 左 8 列遮罩 |
| `06.right_edge.nes` | 屏幕右边缘（第 255 列不命中） |
| `07.screen_bottom.nes` | 屏幕底部 |
| `08.double_height.nes` | 8x16 精灵 |
| `09.timing_basics.nes` | 命中发生的时间点 |
| `10.timing_order.nes` | 多个命中的先后顺序 |
| `11.edge_timing.nes` | 边缘处的命中时序 |
| `readme.txt` | 作者的说明 |

## 用途

`core/test/hardware/ppu-rom-test.cpp` 逐个运行这些 ROM。ROM 把结果显示在屏幕上：测试把名称表 0 的图块号当 ASCII 读出，`PASSED` 表示通过，`FAILED #n` 表示第 n 项失败。

## 结果

十一个 ROM 全部通过。
