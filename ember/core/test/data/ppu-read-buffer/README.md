# ppu-read-buffer

PPU 内存读写与读缓冲的综合测试：基本的 PPU 内存读写、精灵 0 命中标志、`$4014` DMA 与 RAM 镜像的组合、读缓冲与调色板镜像，最后有一段较长的耗时测试。作者 Shay Green（blargg）。

## 来源

文件取自 https://github.com/christopherpow/nes-test-roms 的 `ppu_read_buffer/` 目录（下载地址 `https://raw.githubusercontent.com/christopherpow/nes-test-roms/master/ppu_read_buffer/文件名`），原样放在这里，没有修改。

| 文件 | 内容 |
| --- | --- |
| `test_ppu_read_buffer.nes` | 测试程序，mapper 3（CNROM），4 个 8K 的 CHR 页，约运行 1300 帧 |
| `readme.txt` | 作者的说明 |

## 用途

`core/test/hardware/ppu-rom-test.cpp` 运行它。ROM 按 `$6000` 的约定报告结果：`$6000` 为状态（`$80` 表示运行中，`0` 表示通过），`$6001-$6003` 为签名 `DE B0 61`，`$6004` 起是以 0 结尾的文本。

这个 ROM 是 mapper 3（CNROM），官方插件里没有这个 mapper，所以测试用 `core/test/data/test-plugins/cnrom/` 里的测试插件 `test-cnrom`：写 `$8000-$FFFF` 选择 8K 的 CHR 页，不模拟总线冲突；其他 ROM 都是 mapper 0，用官方的 nrom 插件。核心程序和测试代码里都没有 mapper 的替身实现。

## 结果

通过。
