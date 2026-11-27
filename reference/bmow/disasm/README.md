# Floppy Emu 固件反汇编工程

BMOW Floppy Emu Model B 统一固件 `femu.bin`（250225A）的反汇编工程。目标是摸清固件结构，为「新增金字塔 40 磁道模拟模式」提供依据。方案见上级目录 `方案文档.md`，笔记见 `NOTES.md`。

## 目录

| 路径 | 内容 |
| --- | --- |
| `femu_full.lst` | 全量反汇编清单（avr-objdump，0x0–0x12F00，AVR 程序与数据段） |
| `verify*.txt`、`mode-sites.txt` | 实施前核实的反汇编摘录（main 全段、菜单循环、模式读取点上下文、size 判定、字符串绘制器等） |
| `../bin/femu.bin` | 官方固件映像（AVR 0x0–0x1FFFF + CPLD XVF 0x20000–0x2D028） |
| `../patch/` | 新增模式 9 的补丁工程（patch.s、build.mjs、README.md），产物在 `../build/femu.bin` |
| `../tools/` | avr-gcc 工具链与检索脚本 |

## 反汇编命令

```
avr-objdump -D -b binary -m avr:51 femu.bin
```

工具链在 `../tools/avr-gcc-16.1.0-x64-windows/bin/`。看某段用 `--start-address=0x… --stop-address=0x…`。

## femu.bin 布局

| 范围 | 内容 |
| --- | --- |
| 0x00000–0x0008B | 中断向量表（复位 `jmp 0x1FDE`） |
| 0x0008C–0x01FD7 | 字符串、字库、扫描表 |
| 0x01FD8–0x12BE3 | AVR 代码 |
| 0x12BE4–0x12DB5 | .data 初值（→ RAM 0x100–0x3D1） |
| 0x12DB6–0x12EFF | 表与字符串 |
| 0x12F00–0x1EFF7 | 全 0 空闲区（新代码放这里） |
| 0x1EFF8–0x1EFFF | `DD DD DD DD 00 01 00 00` 标记（含义待确认） |
| 0x1F000–0x1FFFF | 全 FF |
| 0x20000–0x2D028 | CPLD XVF（不动） |

## 关键函数与变量

| 地址 | 作用 |
| --- | --- |
| 0x3B54 | main |
| 0x307E | 模式菜单构建 |
| 0x317A | 模式选择交互 |
| 0x5C28 | 模式兼容检查（模式号→合法类型） |
| 0x6400 | 磁盘初始化（模式 4/8/7 置 5.25 标志） |
| 0x423C | 按类型设置几何参数 |
| 0x6604 | 磁道数 = 文件大小 ÷ 6656 |
| 0x6A56 | 磁道加载主控（NIB 分支 0x6C2A/0x6F0E） |
| 0x673E | NIB 数据读取 |
| 0x4AB4 | 扇区服务（类型 19→0x4CB0） |
| 0xC842 | 镜像类型识别（0xC912 NIB 大小判定） |
| RAM 0x33FE | 当前模式号（3–8） |
| RAM 0x38B6 | 镜像类型号（19=NIB） |
| flash 0x5ED | 模式名表（步长 23、6 项） |
| flash 0x17A7 | 每磁道 nibble 数表（40 项） |
| flash 0x18F7 | 磁道偏移表（40 项） |
| flash 0x12C80 | 类型名指针表 |
| flash 0x12C96 | 每类型参数表（→ RAM 0x389C） |

## 检索脚本（../tools/）

| 脚本 | 用途 |
| --- | --- |
| `find_mode.ps1` | 枚举 RAM 0x33FE 的读写点 |
| `find_dispatch.ps1` | 枚举类型号 0x38B6 读取与 cpi 类型常量分发点 |
| `read_modenames.ps1` | 读模式名表 |
| `find_ldi.ps1` | 按正确 ldi 编码搜立即数对 |

## 注意

- AVR `ldi` 编码：`1110 KKKK dddd KKKK`，K = (w & 0xF) | (((w>>8) & 0xF) << 4)，Rd = 16 + ((w>>4) & 0xF)。cpi/subi/sbci 同 K 布局。
- ≥64KB 的 flash 数据经 RAMPZ 远指针（蹦床 0x12308）访问，不以 ldi 立即数对出现。
- 生成的清单不手改；改动经方案确认后重汇编拼回。