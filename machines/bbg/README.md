# 步步高学生电脑 BBG（BIOS 源码）

步步高多媒体学生电脑（BBGDOS）的 BIOS 源码工程，用 ca65 汇编链接。源码由原版 `bios.asm` 转换而来：伪指令与运算符改为 ca65 写法，原版的 `org` 改为段，段地址在 `link.cfg` 中与原版一致。

## 构建

工具链在 fcdev 扩展的 `toolchain/bin` 下。在本目录运行：

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
| `link.cfg` | 链接配置：8 个段（`S0` 至 `S7`）对应原版的 8 个 `org` |
| `docs/计划与进度.md` | 计划、原厂源码的完整性与已查明的结论 |

## 限制

- 原厂源码缺少符号文件 `tvdef.asm`，还原版中 `DosGetDOSVersion` 与 `StackTooDepth` 两个符号没有依据，是推测值。
- 段名 `S0` 至 `S7` 按原版 `org` 的顺序编号，还没有改成按功能命名。
- 原版的 `org $ff00+1` 这类写法已拆成独立段，段的长度取到下一个 `org` 为止。
