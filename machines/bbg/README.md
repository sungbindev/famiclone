# 步步高学生电脑 BBG（BIOS 源码）

步步高多媒体学生电脑（BBGDOS）的 BIOS 源码工程，用 ca65 汇编链接。源码由原版 `bios.asm` 转换而来：伪指令与运算符改为 ca65 写法，原版的 `org` 改为段，段地址在 `link.cfg` 中与原版一致。

## 构建与运行

用 fcdev 扩展的 `rom` 目标（说明见扩展使用文档“整机 ROM 工程开发指南”）。配置在 `fcdev.json`：

- `firmware`：完整的 128KB 固件 `ember/roms/bbg1_bios.bin` 当底。BIOS 源码只对应最后 16KB（`$C000-$FFFF`），前 112KB 是字库点阵，取自底固件。
- `targetOptions`：`machine` 为 `bbg1`，`offset` 为 114688（BIOS 在固件里的偏移），`size` 为 131072。
- `diskImage`：运行时放进软驱的系统盘 `DOS.IMG`。

任务“fcdev：构建”（构建路径 `src`）生成 `build/rom/bbg.bin`（131072 字节），任务“fcdev：运行”在 Ember 里以它为 `bbg1` 的固件开机。

手动构建 BIOS 部分（工具链在 fcdev 扩展的 `toolchain/bin` 下，在本目录运行）：

```text
ca65 --cpu 6502 -I src/include -g src/bios/bios.s -o build/bios.o
ld65 -C link.cfg -m build/bios.map -o build/bios.bin build/bios.o
```

汇编与链接都无错误即为通过。结果不与 `bbg1_bios.bin` 逐字节比对，原版源码与 ROM 不是同一次构建。

## 目录

| 路径 | 内容 |
| --- | --- |
| `src/bios/bios.s` | BIOS 源码 |
| `src/include/tvdef.inc` | 符号定义，原厂 `tvdef.asm` 的还原版 |
| `link.cfg` | 链接配置：8 个段（`S0` 至 `S7`）对应原版的 8 个 `org`，内存区都带 `fill`，输出 16384 字节按地址排满 |
| `fcdev.json` | fcdev 项目配置：`rom` 目标、底固件、机型、软盘镜像 |
| `docs/计划与进度.md` | 计划、原厂源码的完整性与已查明的结论 |
| `docs/DOS反汇编与开发库计划.md` | `BBGDOS.SYS`、`COMMAND.CMD`、启动扇区的反汇编，系统 API 的整理与汇编开发库的计划 |
| `docs/api/` | 系统接口说明与符号参考（BIOS、DOS、文件调用） |
| `disasm/` | 反汇编工程：`boot-sector`、`bbgdos`、`command`、`bios-rom`，每个工程的 `disasm.json` 是唯一的修改入口，生成的源码按功能分模块 |
| `src/include/system-entries.inc`、`bios-calls.inc`、`dos-calls.inc` | 系统调用入口、功能号、文件通道块与出错码的符号定义 |
| `tools/` | 提取系统文件（`extract-system-files.mjs`）、读 Ember 内存（`ember-dump.mjs`）、生成符号参考（`gen-symbols-doc.mjs`） |

## 限制

- 原厂源码缺少符号文件 `tvdef.asm`，还原版中 `DosGetDOSVersion` 与 `StackTooDepth` 两个符号没有依据，是推测值。
- 段名 `S0` 至 `S7` 按原版 `org` 的顺序编号，还没有改成按功能命名。
- 原版的 `org $ff00+1` 这类写法已拆成独立段，段的长度取到下一个 `org` 为止。
