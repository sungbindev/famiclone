# 步步高学生电脑 BBK

步步高多媒体学生电脑（BBGDOS）的 BIOS 与 DOS 原厂源码工程：能汇编这些源码的 6502 汇编器、还原出的符号文件，以及核对源码与 ROM 的分析工具。本目录是 famiclone 仓库里的一个机型目录（`machines/bbk/`）。

把步步高 BIOS 移植到小霸王学习机 SB2000 上运行的部分，是 SB-DOS 下的程序，在 [apps/sb2000/bbk-bios](../../apps/sb2000/bbk-bios/README.md)（BIOS）与 [apps/sb2000/bbk-loader](../../apps/sb2000/bbk-loader/README.md)（加载器）。

计划、原厂源码的完整性与已查明的结论见 [docs/计划与进度.md](docs/计划与进度.md)。

## 环境

- Node.js 22 及以上，脚本没有第三方依赖。
- BIOS 原版源码 [src/bios/bios.asm](src/bios/bios.asm) 在本目录里，原样复制自原厂资料（GBK 编码，不要用编辑器转码保存）；分析工具读这一份。
- 其他原厂资料仍在仓库根目录的 `reference/` 下，只引用不复制：`reference/docs/BBKDOS源代码/`（`Command.asm`、`Smcdos.asm` 与三个 `.sys`）、`reference/floppy-images/bbk/`；BIOS 用 `ember/bios/bbk1_bios.bin`、`ember/bios/bbk98_bios.bin`（不带文件头）。

## 使用

在本目录运行：

| 命令 | 作用 |
| --- | --- |
| `npm run check` | 汇编 `Bios.asm`、`Command.asm`、`Smcdos.asm`，列出缺失的包含文件、未定义符号、不认识的宏与重复定义，正常时全部为 0 |
| `npm run match` | 把 `Bios.asm` 里位置无关的指令按每 6 条一组，在 BIOS 1.0 ROM 里查找，看两者是否属于同一代码 |
| `npm run align` | 把 `Bios.asm` 按指令块与 BIOS 1.0 ROM 最后一页对齐，列出各自独有的区段 |
| `npm run probe` | 运行 `apps/sb2000/fcprobe` 的探测程序，打印 SB2000 在 FC 兼容模式下的内存写入、PPU、软驱、键盘测试结果（先在该目录里 build 与 disk） |
| `npm test` | 汇编器的单元测试，以及对原厂源码的汇编测试 |
| `node tools/gasm-check.mjs <文件>` | 汇编原厂目录里的任意一个源文件 |
| `node tools/compare-rom.mjs <文件> <二进制> <偏移> [起始地址]` | 把汇编结果与二进制逐字节比对 |

## 限制

- 原厂源码缺少符号文件 `tvdef.asm`，本目录的 [src/include/tvdef.asm](src/include/tvdef.asm) 是还原版：取值来自使用手册、源码注释和 ROM 里的功能号分发表；`DosGetDOSVersion` 与 `StackTooDepth` 两个符号没有依据，是推测值。
- 原厂的 `Bios.asm` 与 `bbk1_bios.bin` 不是同一次构建，汇编结果不能与 ROM 逐字节一致；`Command.asm`、`Smcdos.asm` 与同目录的 `.sys` 文件也不是同一版本。
- 汇编器只做到能还原符号、核对编码和生成 ROM 映像；ROM 的前 7 页（字库）取自 `bbk1_bios.bin`，源码里没有。

## 目录

```text
machines/bbk/
  src/bios/      原版 BIOS 源码 bios.asm（从原厂资料复制，GBK 编码）
  src/include/   还原出的符号文件 tvdef.asm
  tools/         GABBG 方言的 6502 汇编器、检查与比对工具、模拟器驱动
  test/          汇编器的测试
  docs/          计划与进度
```
