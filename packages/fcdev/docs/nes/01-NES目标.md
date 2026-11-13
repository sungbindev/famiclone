# NES 目标

NES（任天堂红白机）目标复用 `cc65` 自带的 `nes` 目标，在它之上加了一个辅助库 `fcnes` 和 ASCII 字库，使 hello 程序保持单文件。目标目录是 [packages/fcdev/targets/nes](../../targets/nes)，字段含义见 [03-目标规范](../03-目标规范.md)。

## 1. 目标定义

| 项 | 值 |
| --- | --- |
| 目标名 | `nes` |
| `cc65Target` | `nes` |
| 链接配置 | `toolchain:cfg/nes.cfg` |
| 启动代码 | 无，用 `nes.lib` 自带的；`forceImports` 为 `__STARTUP__`，保证纯汇编程序也链接启动代码 |
| 库 | `lib/fcnes.lib`、`toolchain:lib/nes.lib` |
| 打包器 | `ines`，产物 `.nes` |
| 程序名规则 | 不限制 |
| 磁盘镜像 | 不支持 |
| 运行方案 | `rom`，不需要固件 |

## 2. 内存与映射器

只支持 `nes.cfg` 的 NROM：16K PRG（`$8000` 起）加 8K CHR，`$FFFA` 起是中断向量。cc65 参数栈 3 页，在 `$0500` 起；`DATA` 装入 ROM，运行时复制到 `$6000` 起的 8K RAM；`BSS` 也在这块 RAM。其他映射器不在首批范围。

## 3. 头文件与函数

- C 程序用 `cc65` 的 `nes.h` 和本目标的 `fcnes.h`，汇编程序用 `fcnes.inc`，它定义 PPU 寄存器地址 `PPUCTRL`、`PPUMASK`、`PPUSTATUS`、`OAMADDR`、`OAMDATA`、`PPUSCROLL`、`PPUADDR`、`PPUDATA`。
- 文字用 ASCII 字库，图块号等于字符码，背景图案表为 `$0000`；画面关闭时才能写显存。

`fcnes.h` 的函数：

| 函数 | 作用 |
| --- | --- |
| `void nes_screen_off(void)` | 关闭画面，之后可以写显存 |
| `void nes_screen_on(void)` | 打开背景与精灵显示（含最左 8 列），并把滚动位置复位到 (0, 0) |
| `void __fastcall__ nes_set_palette(const unsigned char *colors)` | 写入 32 字节调色板：前 16 字节为背景，后 16 字节为精灵 |
| `void __fastcall__ nes_print(unsigned char x, unsigned char y, const char *s)` | 在名称表 (x, y) 处写零结尾字符串，x 为 0 到 31，y 为 0 到 29；要先关闭画面 |
| `void nes_wait_vblank(void)` | 等待下一次垂直消隐开始 |

汇编程序可直接调用对应的 `_nes_*` 符号，参数约定是 cc65 的 `fastcall`：最后一个参数在 A（低字节）与 X（高字节），其余参数在 C 栈上，由被调用者弹出。

## 4. 字库

- 字库数据来自公有领域的 `font8x8_basic`（Daniel Hepper，源自 IBM 公共领域 VGA 字体），原文件保留在 `chr/font8x8_basic.h`，已转成 `chr/ascii-8x8.chr`（ASCII `$20` 到 `$7F` 的 8×8 字模，共 1536 字节）。
- `src/nes-font.s` 把字库放进 `CHARS` 段：前 `$200` 字节留空（对应 `$20` 之前的 32 个图块），之后是字模。
- 目标选项 `font`（布尔，默认 `true`）为 `true` 时链接 `lib/nes-font.o`。在 `fcdev.json` 里写 `"targetOptions": { "font": false }` 可关闭，由程序自己提供 CHR 数据。

## 5. 运行库

`lib-build.json` 把 `src/fcnes.s` 编成 `lib/fcnes.lib`，把 `src/nes-font.s` 编成 `lib/nes-font.o`，`.incbin` 的搜索目录是 `chr`。构建方法见 [03-目标规范](../03-目标规范.md) 第 5 节。

## 6. 运行

`npx fcdev run` 启动模拟器直接加载 `.nes`，不需要固件。验证记录见 [08-测试与验证](../08-测试与验证.md)：NES hello 在模拟器里截图核对，屏幕上显示 `HELLO, NES!`。
