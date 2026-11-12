# bbk-loader

在小霸王学习机 SB2000（SB-DOS）的 DOS 下运行：把机器换成步步高的 BIOS，再从软盘启动步步高的 BBGDOS。用 fcdev 开发，C 语言。BIOS 映像 `BIOS.BBK` 由 [bbk-bios](../bbk-bios/README.md) 构建。

## 使用

把 `START.EXE` 与 `BIOS.BBK`放在同一张盘里，在 DOS 提示符下输入：

```text
start
```

读入 `BIOS.BBK` 需要一会儿，之后画面变成步步高的开机检测，并从软驱 A 启动：盘里要有步步高的启动系统（步步高 DOS 启动盘，`BBGDOS.SYS` 必须是根目录第一个文件）。可以把这两个文件追加到步步高 DOS 启动盘里，也可以先启动再换盘。系统起来后不能回到 DOS，只能硬复位。

出错时显示提示，按任意键返回 DOS：

| 提示 | 原因 |
| --- | --- |
| `Can't open BIOS.BBK` | 当前盘里没有 `BIOS.BBK` |
| `Not a BBK BIOS file` | 文件大小或文件头不对 |
| `No memory` | 扩展 RAM 放不下文件 |
| `Read error` | 读盘出错 |
| `Program too big` | 本程序自己超过 8K，没有空闲的窗口可用于复制，改程序时要保持精简 |

## 构建

```bash
npm run build    # 生成 build/sb2000/START.EXE
npm run disk     # 生成只放有 START.EXE 的软盘镜像 build/sb2000/START.img
```

要在模拟器里看到效果，在 `apps/sb2000/bbk-bios` 里运行 `npm run verify`，它会把 `START.EXE`、`BIOS.BBK` 追加到步步高 DOS 启动盘里再运行。

## 做了什么

1. 读入 `BIOS.BBK` 到扩展 RAM 的低页，检查文件头（`BBKB`、版本 1、页数 8、长度）。
2. 关 NMI 与中断，对每个 4K 块用 DMA 复制到 ERAM 缓冲 `$6800-$77FF`，再用 CPU 复制到窗口 2 映射的扩展 RAM 的 4K 页 `$E0-$FF`（8 个 16K 页，共 128K）。真机实测 DMA 只能写 ERAM，且 `$404B` 的位 7 必须置 1；窗口直接写页寄存器 `$4042`。窗口 3 里有 C 软件栈不能换页，所以程序（含数据）必须在 `$A000` 以下。
3. 把跳板复制到 ERAM `$6000` 运行：窗口 4-7（`$C000-$FFFF`）指向映像的第 7 页，窗口 0-3 指向第 0 页，屏蔽 SB2000 的全部中断，`$2000`、`$2001`、`$2008`、`$4300` 清 0，跳到文件头里记的入口。

始终留在 6576 模式，不写 `$4301`。映像格式与移植设计见 [bbk-bios/docs/计划与进度.md](../bbk-bios/docs/计划与进度.md)。

## 文件

| 文件 | 说明 |
| --- | --- |
| `src/start.c` | 程序 |
| `fcdev.json` | fcdev 配置：目标 sb2000，程序名 start，固件指向 `reference/rom/SB2K_BIOS.NES` |
