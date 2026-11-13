# blargg-ppu-tests

经典 FC 的 PPU 基础行为测试：调色板 RAM、精灵 RAM 与 `$4014` DMA、VRAM 访问与读缓冲、vblank 清除时间、上电调色板。作者 Shay Green（blargg），2005-09-15 版本。

## 来源

文件取自 https://github.com/christopherpow/nes-test-roms 的 `blargg_ppu_tests_2005.09.15b/` 目录（下载地址 `https://raw.githubusercontent.com/christopherpow/nes-test-roms/master/blargg_ppu_tests_2005.09.15b/文件名`），原样放在这里，没有修改。

| 文件 | 内容 |
| --- | --- |
| `palette_ram.nes` | 调色板的读写与镜像，调色板读取不经缓冲 |
| `power_up_palette.nes` | 上电时调色板的初始值是否与作者的 NES 一致（`ppu-2c02` 的上电调色板取自这张表） |
| `sprite_ram.nes` | 经 `$2003`、`$2004`、`$4014` 读写 OAM，属性字节读出时屏蔽位 2-4，DMA 从 `$2003` 的值起复制并回绕 |
| `vbl_clear_time.nes` | vblank 标志在 NMI 之后约 2270 个 CPU 周期被清除 |
| `vram_access.nes` | VRAM 读写与读缓冲的行为 |
| `readme.txt` | 作者的说明，列出各 ROM 的结果码含义 |

## 用途

`core/test/hardware/ppu-rom-test.cpp` 逐个运行这些 ROM。ROM 把结果显示在屏幕上（同时用蜂鸣声报告）：测试把名称表 0 的图块号当 ASCII 读出，出现 `$01` 表示全部通过，其他结果码的含义见 `readme.txt`。

## 结果

五个 ROM 全部通过。
