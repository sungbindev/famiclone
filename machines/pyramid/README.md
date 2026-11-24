# 金字塔学习机（Pyramid）

金字塔学习机的 BIOS 研究目录。目前只对 PEC-9588 版做了反汇编。

## 目录

| 目录 | 内容 |
| --- | --- |
| [disassembly/](disassembly/README.md) | PEC-9588 的反汇编工程（ca65 源码，构建结果与 `ember/roms/pec9588.bin` 逐字节一致） |

## 映像

- 反汇编来源：`reference/study_roms/金字塔/PEC-9588.NES`，524304 字节，含 16 字节 iNES 文件头。
- 模拟器固件：`ember/roms/pec9588.bin`，524288 字节，不含文件头；FC Dev 构建的产物与它一致。
- 同系列映像 `PEC-586.NES`、`PEC-686F.NES`（在 `reference/study_roms/金字塔/` 中）以及 `ember/roms/` 下的 `pec586.bin`、`pec686f.bin` 还没有建立工程。
