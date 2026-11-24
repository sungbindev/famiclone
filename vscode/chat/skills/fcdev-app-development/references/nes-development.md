# NES 程序开发

开发 NES（红白机）程序时使用。FC Dev 的 `nes` 目标复用 cc65 自带的 `nes` 目标，在它之上加了辅助库 `fcnes` 与 ASCII 字库。目标文件位于 `targets/nes/`（`include/fcnes.h`、`asminc/fcnes.inc`、`chr/`、`src/`）。

## 目标定义

| 项 | 值 |
| --- | --- |
| 链接配置 | cc65 的 `nes.cfg`（NROM） |
| 库 | `fcnes.lib`、cc65 的 `nes.lib` |
| 启动代码 | `nes.lib` 自带；目标强制引入 `__STARTUP__`，纯汇编程序也会链接启动代码 |
| 产物 | `.nes`（iNES） |
| 磁盘镜像 | 不支持（构建不生成镜像） |
| 运行 | 运行任务或 `fcdev_run` 直接加载 `.nes`，不需要固件 |

## 内存与映射器

- 只支持 `nes.cfg` 的 NROM：16K PRG（从 `$8000` 起）加 8K CHR，中断向量在 `$FFFA` 起。其他映射器不支持，也不要改链接配置绕过。
- cc65 参数栈 3 页，在 `$0500` 起；`DATA` 装入 ROM，运行时复制到 `$6000` 起的 8K RAM，`BSS` 也在这块 RAM。
- 程序超过 16K PRG 时链接失败，需要精简，不能靠加映射器解决。

## 头文件与函数

C 程序引入 cc65 的 `nes.h`（寄存器与颜色等定义）和 `fcnes.h`；汇编程序 `.include "fcnes.inc"`，它定义 `PPUCTRL`、`PPUMASK`、`PPUSTATUS`、`OAMADDR`、`OAMDATA`、`PPUSCROLL`、`PPUADDR`、`PPUDATA`。

| 函数 | 作用 |
| --- | --- |
| `void nes_screen_off(void)` | 关闭画面，之后才能写显存 |
| `void nes_screen_on(void)` | 打开背景与精灵显示（含最左 8 列），滚动复位到 (0, 0) |
| `void __fastcall__ nes_set_palette(const unsigned char *colors)` | 写 32 字节调色板：前 16 字节背景，后 16 字节精灵 |
| `void __fastcall__ nes_print(unsigned char x, unsigned char y, const char *s)` | 在名称表 (x, y) 写零结尾字符串，x 为 0 到 31，y 为 0 到 29；要先关闭画面 |
| `void nes_wait_vblank(void)` | 等下一次垂直消隐开始 |

- 汇编可直接调用 `_nes_*` 符号。参数用 cc65 的 fastcall：最后一个参数在 A（低字节）与 X（高字节），其余参数在 C 栈上，由被调用者弹出。
- `fcnes` 只提供上面 5 个函数。精灵、手柄、声音、滚动、中断等没有封装，也没有在 FC Dev 里验证过的示例：用到时直接写 PPU/APU 寄存器，或用 cc65 `nes.lib` 提供的接口（名字以工具链 `include/` 下的 `nes.h`、`joystick.h` 等为准，`fcdev_info` 可查工具链目录），写完要在模拟器里实测。

## 字库与图块

- 字库是公有领域的 `font8x8_basic`，ASCII `$20` 到 `$7F` 的 8×8 字模共 1536 字节，放在 `CHARS` 段：前 `$200` 字节留空（对应 `$20` 之前的 32 个图块），之后是字模。
- 文字显示约定：图块号等于字符码，背景图案表为 `$0000`；所以只有 ASCII，不含中文。
- 目标选项 `font` 默认 `true`，链接字库。要自己提供 CHR 数据（图块、精灵）时写 `"targetOptions": { "font": false }`，并自己提供 `CHARS` 段的数据；图块布局由程序自己决定，`nes_print` 此后不再适用。

## 写法要点

- 画面关闭时才能写显存（调色板、名称表、图案表）：先 `nes_screen_off()`，写完 `nes_wait_vblank()` 后再 `nes_screen_on()`。汇编里写 `PPUMASK` 为 `$00` 关闭，`$1E` 显示背景与精灵。
- 写 `PPUADDR` 前先读一次 `PPUSTATUS` 复位写入顺序；调色板在显存 `$3F00`，名称表 (x, y) 的地址是 `$2000 + y*32 + x`；写完显存后把 `PPUSCROLL` 写两次 0 复位滚动。
- NES 程序没有“返回”：`main` 里要有死循环（C 写 `for (;;) {}`，汇编 `jmp` 到自身）。
- 颜色值是 NES 调色板索引（如 `$0F` 黑、`$30` 白、`$10` 灰）；每组 4 色，颜色 0 是背景色。

## 最小程序

C（模板 `nes-c` 的 `src/hello.c`）：

```c
#include <fcnes.h>

static const unsigned char palette[32] = {
    0x0F, 0x30, 0x10, 0x00, 0x0F, 0x30, 0x10, 0x00,
    0x0F, 0x30, 0x10, 0x00, 0x0F, 0x30, 0x10, 0x00,
    0x0F, 0x30, 0x10, 0x00, 0x0F, 0x30, 0x10, 0x00,
    0x0F, 0x30, 0x10, 0x00, 0x0F, 0x30, 0x10, 0x00
};

int main(void)
{
    nes_screen_off();
    nes_set_palette(palette);
    nes_print(10, 14, "HELLO, NES!");
    nes_wait_vblank();
    nes_screen_on();
    for (;;) {
    }
}
```

汇编（模板 `nes-asm`）要点：`.include "fcnes.inc"`，`.export _main`；代码放 `CODE` 段、数据放 `RODATA` 段；先 `lda #$00 / sta PPUMASK` 关画面，再写调色板与名称表，用 `bit PPUSTATUS / bpl` 循环等垂直消隐，最后 `PPUSCROLL`、`PPUCTRL` 写 0、`PPUMASK` 写 `$1E`，以 `jmp` 自身结束。

## 运行与验证

- 运行任务“FC Dev：运行”（或工具 `fcdev_run`）在模拟器里打开 `.nes`，不需要固件；Ember 的位置由用户在设置 `fcdev.emulator.path` 里指定。
- 自动核对：按 [run-and-verify.md](run-and-verify.md) 的“核对清单：NES”做，模板应显示 `HELLO, NES!`。
- 手柄按键用 `emu_input` 的 `kind: "pad"` 注入（1=A、2=B、4=选择、8=开始、16=上、32=下、64=左、128=右），用法见 skill `ember-emulator`。
- 已有 `.nes` 要读懂时用 skill `disassemble-6502`（内置支持 NROM 等常见映射器）。
