# oam-stress

OAM 压力测试：随机设置 `$2003`，随机写入或读出若干字节，最后核对 OAM 的全部内容。作者 Shay Green（blargg）。

## 来源

文件取自 https://github.com/christopherpow/nes-test-roms 的 `oam_stress/` 目录（下载地址 `https://raw.githubusercontent.com/christopherpow/nes-test-roms/master/oam_stress/文件名`），原样放在这里，没有修改。

| 文件 | 内容 |
| --- | --- |
| `oam_stress.nes` | 测试程序，约运行 1700 帧 |
| `readme.txt` | 作者的说明（真机上只有四种 CPU-PPU 相位中的一种能通过，`fc` 主机型号用的相位通过） |

## 用途

`core/test/hardware/ppu-rom-test.cpp` 运行它。ROM 按 `$6000` 的约定报告结果：`$6000` 为状态（`$80` 表示运行中，`0` 表示通过），`$6001-$6003` 为签名 `DE B0 61`，`$6004` 起是以 0 结尾的文本。测试卡带在 `$6000-$7FFF` 带 8K RAM。

## 结果

通过。
