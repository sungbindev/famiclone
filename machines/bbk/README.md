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
| `npm run build` | 汇编 `Bios.asm`，与 `bbk_bios10.nes` 的前 7 页（字库）拼成 `build/bbk_src.nes` |
| `npm run boot` | 在调试版模拟器里加载 `build/bbk_src.nes` 与 DOS 启动盘，回车通过日期与时间提示，截图到 `build/boot.png`，画面出现 `A:\>` 即启动成功；`-- --command dir` 可在提示符下再执行一条命令 |
| `npm run match` | 把 `Bios.asm` 里位置无关的指令按每 6 条一组，在 BIOS 1.0 ROM 里查找，看两者是否属于同一代码 |
| `npm run align` | 把 `Bios.asm` 按指令块与 BIOS 1.0 ROM 最后一页对齐，列出各自独有的区段 |
| `npm test` | 汇编器的单元测试，以及对原厂源码的汇编测试 |
| `node tools/gasm-check.mjs <文件>` | 汇编原厂目录里的任意一个源文件 |
| `node tools/compare-rom.mjs <文件> <二进制> <偏移> [起始地址]` | 把汇编结果与二进制逐字节比对 |

## 限制

- 原厂源码缺少符号文件 `tvdef.asm`，本目录的 [src/include/tvdef.asm](src/include/tvdef.asm) 是还原版：取值来自使用手册、源码注释和 ROM 里的功能号分发表；`DosGetDOSVersion` 与 `StackTooDepth` 两个符号没有依据，是推测值。
- 原厂的 `Bios.asm` 与 `bbk_bios10.nes` 不是同一次构建，汇编结果不能与 ROM 逐字节一致；`Command.asm`、`Smcdos.asm` 与同目录的 `.sys` 文件也不是同一版本。详见计划与进度。
- 汇编器只做到能还原符号、核对编码和生成 ROM 映像；ROM 的前 7 页（字库）取自 `bbk_bios10.nes`，源码里没有。
- `npm run boot` 需要先编译调试版模拟器（`node tools/build-emulator.mjs`，或 `virtuanes/build.ps1`），只支持 Windows。
- 用 `Bios.asm` 构建的 ROM 开机画面没有“步步高”徽标，DOS 里显示的默认日期与原版 ROM 不同（2043 年而不是 1996 年），不影响启动；原因没有查。

## 目录

```text
machines/bbk/
  src/include/   还原出的符号文件 tvdef.asm
  tools/         GABBG 方言的 6502 汇编器、检查与比对工具、ROM 构建、模拟器驱动
  test/          汇编器与 ROM 构建的测试
  docs/          计划与进度
  build/         构建输出（不进 Git）
```
