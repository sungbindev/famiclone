# 步步高学生电脑 BBK

步步高多媒体学生电脑（BBGDOS）的 BIOS 与 DOS 源码工程，目标是把步步高 BIOS 移植到小霸王 SB2000 的硬件上运行。本目录是 famiclone 仓库里的一个机型目录（`machines/bbk/`）。

计划、进度与待确认事项见 [docs/计划与进度.md](docs/计划与进度.md)。

## 环境

- Node.js 22 及以上，脚本没有第三方依赖。
- 原厂资料在仓库根目录的 `reference/` 下，本目录只引用不复制：`reference/docs/BBKDOS源代码/`（GBK 编码）、`reference/rom/bbk_bios10.nes`、`reference/rom/bbk_bios20.nes`。

## 使用

在本目录运行：

| 命令 | 作用 |
| --- | --- |
| `npm run check` | 汇编原厂的 `Bios.asm`、`Command.asm`、`Smcdos.asm`，列出缺失的包含文件、未定义符号、不认识的宏与重复定义，正常时全部为 0 |
| `npm run match` | 把 `Bios.asm` 里位置无关的指令按每 6 条一组，在 BIOS 1.0 ROM 里查找，看两者是否属于同一代码 |
| `npm test` | 汇编器的单元测试，以及对原厂源码的汇编测试 |
| `node tools/gasm-check.mjs <文件>` | 汇编原厂目录里的任意一个源文件 |
| `node tools/compare-rom.mjs <文件> <二进制> <偏移> [起始地址]` | 把汇编结果与二进制逐字节比对 |

## 限制

- 原厂源码缺少符号文件 `tvdef.asm`，本目录的 [src/include/tvdef.asm](src/include/tvdef.asm) 是还原版：取值来自使用手册、源码注释和 ROM 里的功能号分发表；`DosGetDOSVersion` 与 `StackTooDepth` 两个符号没有依据，是推测值。
- 原厂的 `Bios.asm` 与 `bbk_bios10.nes` 不是同一次构建，汇编结果不能与 ROM 逐字节一致；`Command.asm`、`Smcdos.asm` 与同目录的 `.sys` 文件也不是同一版本。详见计划与进度。
- 汇编器只做到能还原符号、核对编码，不生成可烧写的文件。

## 目录

```text
machines/bbk/
  src/include/   还原出的符号文件 tvdef.asm
  tools/         GABBG 方言的 6502 汇编器与检查、比对工具
  test/          汇编器的单元测试
  docs/          计划与进度
  build/         构建输出（不进 Git）
```
