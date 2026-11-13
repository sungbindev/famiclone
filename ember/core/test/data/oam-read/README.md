# oam-read

OAM 读取测试：从 0 到 `$FF` 逐字节检查 `$2004` 读出的是 `$2003` 当前地址处的字节，打印 16 行、每行 16 个字符，`-` 表示读对，`*` 表示读错。作者 Shay Green（blargg）。

## 来源

文件取自 https://github.com/christopherpow/nes-test-roms 的 `oam_read/` 目录（下载地址 `https://raw.githubusercontent.com/christopherpow/nes-test-roms/master/oam_read/文件名`），原样放在这里，没有修改。

| 文件 | 内容 |
| --- | --- |
| `oam_read.nes` | 测试程序 |
| `readme.txt` | 作者的说明 |

## 用途

`core/test/hardware/ppu-rom-test.cpp` 运行它。ROM 按 `$6000` 的约定报告结果：`$6000` 为状态（`$80` 表示运行中，`0` 表示通过），`$6001-$6003` 为签名 `DE B0 61`，`$6004` 起是以 0 结尾的文本。测试卡带在 `$6000-$7FFF` 带 8K RAM。

## 结果

通过。
