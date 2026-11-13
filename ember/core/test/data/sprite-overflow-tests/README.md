# sprite-overflow-tests

精灵溢出标志（`$2002` 位 5）的测试：基本行为、细节、时序、硬件缺陷导致的冷门情形、模拟器容易出错的情形。作者 Shay Green（blargg）。

## 来源

文件取自 https://github.com/christopherpow/nes-test-roms 的 `sprite_overflow_tests/` 目录（下载地址 `https://raw.githubusercontent.com/christopherpow/nes-test-roms/master/sprite_overflow_tests/文件名`），原样放在这里，没有修改。

| 文件 | 内容 |
| --- | --- |
| `1.Basics.nes` | 溢出标志的基本行为：9 个精灵时置位，读 `$2002` 不清除，在预渲染行清除，渲染关闭时不置位 |
| `2.Details.nes` | 细节：左侧遮罩下也置位，Y 为 239 置位而 240、255 不置位，8x16 精灵 |
| `3.Timing.nes` | 标志置位与清除的时间（容许相差一两个 CPU 周期） |
| `4.Obscure.nes` | 冷门情形：已有 8 个精灵后评估逻辑的 n 与 m 同时递增，把后面精灵的其他字节当作 Y 坐标 |
| `5.Emulator.nes` | 按预测方式处理溢出标志的模拟器容易出错的情形 |
| `readme.txt` | 作者的说明 |

## 用途

`core/test/hardware/ppu-rom-test.cpp` 逐个运行这些 ROM。ROM 把结果显示在屏幕上：测试把名称表 0 的图块号当 ASCII 读出，`PASSED` 表示通过，`FAILED #n` 表示第 n 项失败。

## 结果

五个 ROM 全部通过。
