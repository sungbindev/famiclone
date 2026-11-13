# ppu-open-bus

PPU 开放总线测试：读只写寄存器、`$2002` 低 5 位、调色板高 2 位等没有定义的位，读到 PPU 数据总线上的旧值；旧值每一位若约 600 毫秒没有被 1 刷新就会衰减为 0。作者 Shay Green（blargg）。

## 来源

文件取自 https://github.com/christopherpow/nes-test-roms 的 `ppu_open_bus/` 目录（下载地址 `https://raw.githubusercontent.com/christopherpow/nes-test-roms/master/ppu_open_bus/文件名`），原样放在这里，没有修改。

| 文件 | 内容 |
| --- | --- |
| `ppu_open_bus.nes` | 测试程序 |
| `readme.txt` | 作者的说明 |

## 用途

`core/test/hardware/ppu-rom-test.cpp` 运行它。ROM 按 `$6000` 的约定报告结果：`$6000` 为状态（`$80` 表示运行中，`0` 表示通过），`$6001-$6003` 为签名 `DE B0 61`，`$6004` 起是以 0 结尾的文本。测试卡带在 `$6000-$7FFF` 带 8K RAM。

## 结果

**未通过，是已知差异**：失败在第 3 项 `Decay value should become zero by one second`（总线上的值应在约 1 秒后衰减为 0）。`ppu-2c02` 把开放总线简化为“保持最近一次值”，不模拟衰减。测试 `PpuRomKnownDifferences.OpenBusDecayIsNotSimulated` 断言这个失败结果；实现衰减后这个测试会失败，到时把它改成普通的通过测试并更新 `docs/02-硬件模块.md` 第 6.1 节的“已知差异”。
