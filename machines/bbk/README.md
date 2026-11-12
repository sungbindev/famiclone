# 步步高学生电脑 BBK

步步高多媒体学生电脑（BBGDOS）的 BIOS 与 DOS 源码工程，把步步高 BIOS 移植到小霸王学习机 SB2000 的硬件上运行：在 SB-DOS 下输入 `startbbk`，机器换成步步高的 BIOS，再从软盘启动 BBGDOS。本目录是 famiclone 仓库里的一个机型目录（`machines/bbk/`）。

计划、设计、进度与未完成事项见 [docs/计划与进度.md](docs/计划与进度.md)。

## 环境

- Node.js 22 及以上，脚本没有第三方依赖。
- 在 SB-DOS 下运行的装载器 [apps/sb2000/startbbk](../../apps/sb2000/startbbk/README.md) 用 fcdev 构建（`npm run build`）。
- 验证要用调试版模拟器（`node tools/build-emulator.mjs`，或 `virtuanes/build.ps1`），只支持 Windows。
- BIOS 源码 [src/bios/Bios.asm](src/bios/Bios.asm) 是 SB2000 版，GBK 编码，不要用编辑器转码保存；它由原厂源码经 [tools/port-bios.mjs](tools/port-bios.mjs) 移植而来，之后可以直接修改这一份。
- 原厂资料在仓库根目录的 `reference/` 下，只引用不复制：`reference/docs/BBKDOS源代码/`（原厂 `Bios.asm`、`Command.asm`、`Smcdos.asm` 与三个 `.sys`）、`reference/rom/bbk_bios10.nes`、`reference/rom/bbk_bios20.nes`、`reference/floppy-images/bbk/`。

## 使用

在本目录运行：

| 命令 | 作用 |
| --- | --- |
| `npm run build` | 汇编 `src/bios/Bios.asm`，与 `bbk_bios10.nes` 的前 7 页（字库）拼成 `build/bbk.bios` |
| `npm run verify` | 在调试版模拟器里验证：生成 `build/dosboot.img`（步步高 DOS 启动盘加 `STARTBBK.EXE`、`BBK.BIO`），SB-DOS 里运行 `startbbk`，进入 BBGDOS，回车跳过日期与时间，再执行 `dir`，截图到 `build/shots/dos.png`；画面出现文件列表与 `A:\>` 即成功。先构建 `bbk.bios` 与装载器 |
| `node tools/run-bbk.mjs [帧数] [截图文件] [dos] [命令]` | 同上，可改帧数与截图位置；不带 `dos` 时软盘只含装载器与 `bbk.bios`，只能看到步步高的开机检测画面 |
| `npm run port` | 按规则从原厂 `Bios.asm` 重新生成 `src/bios/Bios.asm`，会覆盖对它的手工修改，所以与现有文件不同时要加 `-- --force` |
| `npm run check` | 汇编原厂的 `Bios.asm`、`Command.asm`、`Smcdos.asm`，列出缺失的包含文件、未定义符号、不认识的宏与重复定义，正常时全部为 0 |
| `npm run match` | 把原厂 `Bios.asm` 里位置无关的指令按每 6 条一组，在 BIOS 1.0 ROM 里查找，看两者是否属于同一代码 |
| `npm run align` | 把原厂 `Bios.asm` 按指令块与 BIOS 1.0 ROM 最后一页对齐，列出各自独有的区段 |
| `npm run probe` | 运行 `apps/sb2000/fcprobe` 的探测程序，打印 SB2000 在 FC 兼容模式下的内存写入、PPU、软驱、键盘测试结果（先在该目录里 build 与 disk） |
| `npm test` | 汇编器、移植规则、`bbk.bios` 格式与软盘追加文件的测试 |
| `node tools/gasm-check.mjs <文件>` | 汇编原厂目录里的任意一个源文件 |
| `node tools/compare-rom.mjs <文件> <二进制> <偏移> [起始地址]` | 把汇编结果与二进制逐字节比对 |

## 限制

- 目前只能启动英文版 BBGDOS 并在提示符下使用键盘和软驱。并口打印机、微机通讯卡、语音、鼠标、中文显示（`Smdos.sys`、`Smcdos.asm` 直接写 PPU）、扫描线分屏都没有移植，详见计划与进度。
- 内存按 256K（16 页）报告，显存按 32K 报告；步步高 `$4400-$57FF` 的低端内存在 SB2000 上不存在。
- 原厂源码缺少符号文件 `tvdef.asm`，本目录的 [src/include/tvdef.asm](src/include/tvdef.asm) 是还原版：取值来自使用手册、源码注释和 ROM 里的功能号分发表；`DosGetDOSVersion` 与 `StackTooDepth` 两个符号没有依据，是推测值。
- 原厂的 `Bios.asm` 与 `bbk_bios10.nes` 不是同一次构建，汇编结果不能与 ROM 逐字节一致；`Command.asm`、`Smcdos.asm` 与同目录的 `.sys` 文件也不是同一版本。
- 字库（ROM 前 7 页）取自 `bbk_bios10.nes`，源码里没有。
- 没有在真机上验证，结论都来自调试版模拟器。

## 目录

```text
machines/bbk/
  src/bios/      SB2000 版 BIOS：Bios.asm（GBK）与硬件适配层 hal-defs.asm、hal-resident.asm、hal-ppu.asm
  src/include/   还原出的符号文件 tvdef.asm
  tools/         GABBG 方言的 6502 汇编器、移植与构建脚本、模拟器验证脚本、分析工具
  test/          汇编器、移植规则、bbk.bios 格式的测试
  docs/          计划、设计与进度
  build/         构建输出（不进 Git）
```
