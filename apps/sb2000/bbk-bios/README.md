# bbk-bios

移植到小霸王学习机 SB2000（6576 模式）上的步步高 BIOS，构建成 `BIOS.BBK`，由 [bbk-loader](../bbk-loader/README.md) 装入后换掉小霸王的 BIOS，再从软盘启动步步高的 BBGDOS。目前能启动英文版 BBGDOS 并在提示符下使用键盘和软驱，没有在真机上验证。

计划、设计、进度与未完成事项见 [docs/计划与进度.md](docs/计划与进度.md)。原厂源码、汇编器与分析工具在 [machines/bbk](../../../machines/bbk/README.md)。

## 环境

- Node.js 22 及以上，脚本没有第三方依赖。
- 汇编器、符号文件 `tvdef.asm` 与字库 ROM 直接用 `machines/bbk` 里的，不复制。
- 加载器 `START.EXE` 要先在 `apps/sb2000/bbk-loader` 里构建（`npm run build`）。
- 验证要用调试版模拟器（用 `virtuanes/build.ps1` 编译），只支持 Windows。
- BIOS 源码 [src/bios.asm](src/bios.asm) 是 SB2000 版，GBK 编码，不要用编辑器转码保存；它由原版（`machines/bbk/src/bios/bios.asm`）经 [tools/port-bios.mjs](tools/port-bios.mjs) 移植而来，之后可以直接修改这一份。

## 使用

在本目录运行：

| 命令 | 作用 |
| --- | --- |
| `npm run build` | 汇编 `src/bios.asm`，与 `bbk_bios10.nes` 的前 7 页（字库）拼成 `build/bin/BIOS.BBK` |
| `npm run verify` | 在调试版模拟器里验证：生成 `build/work/dosboot.img`（步步高 DOS 启动盘加 `START.EXE`、`BIOS.BBK`），SB-DOS 里运行 `start`，进入 BBGDOS，回车跳过日期与时间，再执行 `dir`，截图到 `build/work/shots/dos.png`；画面出现文件列表与 `A:\>` 即成功。先构建 `BIOS.BBK` 与加载器 |
| `node tools/run-bbk.mjs [帧数] [截图文件] [dos] [命令]` | 同上，可改帧数与截图位置；不带 `dos` 时软盘只含加载器与 `BIOS.BBK`，只能看到步步高的开机检测画面 |
| `npm run port` | 按规则从原版 `Bios.asm` 重新生成 `src/bios.asm`，会覆盖对它的手工修改，所以与现有文件不同时要加 `-- --force` |
| `npm test` | 移植规则、`BIOS.BBK` 格式与软盘追加文件的测试 |

## 限制

- 目前只能启动英文版 BBGDOS。并口打印机、微机通讯卡、语音、鼠标、中文显示（`Smdos.sys`、`Smcdos.asm` 直接写 PPU）、扫描线分屏都没有移植，详见计划与进度。
- 内存按 256K（16 页）报告，显存按 32K 报告；步步高 `$4400-$57FF` 的低端内存在 SB2000 上不存在。
- 没有在真机上验证，结论都来自调试版模拟器。

## 目录

```text
apps/sb2000/bbk-bios/
  src/           SB2000 版 BIOS：bios.asm（GBK）与硬件适配层 hal-defs.asm、hal-resident.asm、hal-ppu.asm
  tools/         移植与构建脚本、模拟器验证脚本、软盘追加文件
  test/          移植规则、BIOS.BBK 格式的测试
  docs/          计划、设计与进度
  build/bin/     构建产物 BIOS.BBK（不进 Git）
  build/work/    验证用的临时文件：合并启动盘、截图（不进 Git，可随时删除）
```
