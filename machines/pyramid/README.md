# 金字塔学习机（Pyramid）

金字塔学习机的 BIOS 研究目录。目前只对 PEC-9588 版做了反汇编。

## 目录

| 目录 | 内容 |
| --- | --- |
| [disassembly/](disassembly/README.md) | PEC-9588 的反汇编工程（ca65 源码，构建结果与 `ember/roms/pec9588.bin` 逐字节一致） |
| [docs/LM-DOS命令.md](docs/LM-DOS命令.md) | 磁盘操作系统 LM-DOS 3.31 的 37 个命令：格式、作用、参数规则表、错误信息 |
| [docs/LM-DOS系统接口.md](docs/LM-DOS系统接口.md) | 页 3 向量、文件管理器、读写盘程序（RWTS）、控制台与系统跳转表 |
| [tools/](tools/) | `pyramid-blank-nib.mjs`：直接生成空白的金字塔 `.nib` 原始磁道镜像（35 道 × 每道 6656 半字节 = 232960 字节，`.nib` 的标准尺寸；每道 14 个数据全零的扇区），供 BMOW Floppy Emu 一类按磁道回放的虚拟软驱使用，放入后需 `INIT`；用法 `node machines/pyramid/tools/pyramid-blank-nib.mjs [输出.nib]`，不给输出时写到 `reference/floppy-images/pec9588/blank.nib` |

## 映像

- 反汇编来源：`PEC-9588.NES`，524304 字节，含 16 字节 iNES 文件头。
- 模拟器固件：`ember/roms/pec9588.bin`，524288 字节，不含文件头；FC Dev 构建的产物与它一致。
- 同系列映像 `PEC-586.NES`、`PEC-686F.NES` 以及 `ember/roms/` 下的 `pec586.bin`、`pec686f.bin` 还没有建立工程。
