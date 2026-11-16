# fbasic（SB2000 版）

把任天堂 Family BASIC v3.0（NS-HUBASIC V3.0，NINTENDO/SHARP/HUDSON）原生移植成小霸王学习机 SB2000 的 SB-DOS 程序（`FBASIC.EXE`，6576 模式）。原生移植的意思是：改写依赖 FC 机器的部分（画面、键盘、声音、手柄、程序存储器、退出），直接使用 SB2000 的硬件与 SB-DOS，不做 NES 兼容层，也不是 FC 模式程序。BASIC 解释器本身与数据不改。

计划与决定见 [docs/移植计划.md](docs/移植计划.md)，每处依赖 FC 机器的地方的做法见 [docs/移植对照表.md](docs/移植对照表.md)。

## 当前状态

| 阶段 | 内容 | 状态 |
| --- | --- | --- |
| 1 | 把原版还原成可重定位的符号源码 | 完成（见下） |
| 2 | 移植对照表 | 已按整机接管方案重写，仍有若干“待查”事项 |
| 3 | 外壳：加载器、ERAM 小段、DOS 入口门、退出 | 完成：原版 PRG 原样放进窗口 0-7 能跑起来，入口门、退出回 DOS 已在 Ember 里验证 |
| 4 | 原生改写：启动与退出、画面、键盘、图形与精灵、声音、手柄、存取 | 进行中：开机画面、文字输出、SB2000 键盘（码集 1，IRQ）已在 Ember 里跑通，能输入并执行 BASIC 语句，F12 退回 DOS；GAME 0-3 的数据已搬到扩展 RAM 页，DMA 搬运已验证；BG 画面、属性调色板、精灵、声音、手柄、存取还没改 |
| 5 | 验证：同一份按键脚本分别在 FC 模式（原版）与学习机（新版）里运行 | 未开始 |

## 阶段 1 的结果

- [reference/fbasic-nes.s](reference/fbasic-nes.s)：原版 PRG 的可重定位 ca65 源码，8643 个条目、1450 个被引用的地址；汇编后与原 PRG 逐字节一致，`$8000` 起 32768 字节。
- PRG 32768 字节：代码 18185（8595 条指令，16 处“跳过前缀”），数据 14583（字节表 13559、地址表 572、数值表 144、文字 308），没有未分类的字节。
- 硬件访问点：PPU 80 处（`$2007` 43、`$2006` 17、`$2001` 8、`$2000` 5、`$2005` 4、`$2002` 2、`$2003` 1），声音 21 处，`$4016` 13 处、`$4017` 6 处，程序存储器 `$6000-$7FFF` 21 处。
- CHR 另存为 [reference/fbasic-chr.bin](reference/fbasic-chr.bin)（8192 字节）。
- `$00ED` 是 NMI 在 RAM 里的入口：启动时往 `$ED-$EF` 写 `JMP $8971`；原版有 16 处“`$2C`/`$AE`/`$EC` 吞掉后一条指令”的写法，源码里写成 `.byte` 加后一条指令。
- 验证：CDL 里执行过的 13994 个字节全部被分类为代码；把源码里 284 块例程与数据随机重排后重新汇编，在 Ember 的 FC 模式里跑同一批脚本，截图与原版逐字节相同。

## 命令

在本目录运行；FC 模式的运行需要 Ember 打包版（环境变量 `FCDEV_EMULATOR` 指向 `ember.exe`，默认 `../../../ember/out/Ember-win32-x64/ember.exe`）。

| 命令 | 作用 |
| --- | --- |
| `npm test` | 分类覆盖、跳过前缀、立即数地址、CDL 一致、汇编回读与重排汇编的测试 |
| `node tools/lift.mjs [--permute <种子>]` | 由 `analysis/` 生成 `reference/fbasic-nes.s` 并汇编核对；`--permute` 打乱块顺序，生成 `build/fbasic-permuted.nes` |
| `node tools/record-coverage.mjs [脚本名片段...]` | 在 FC 模式里让原版按 `analysis/coverage/` 里的脚本运行，记录 CDL 并合并到 `analysis/prg.cdl`，截图在 `build/shots/`；`--rom <nes 文件> --shots <目录>` 换一个 ROM 重跑，只留截图 |
| `node tools/fc-shot.mjs <png> [帧数] [--no-keyboard]` | 在 FC 模式里运行原版并截图，和学习机里新版的画面对照 |
| `node tools/diff-png.mjs <甲> <乙>` | 逐像素比较两张 PNG（只在 Windows 上可用） |
| `node tools/stats.mjs` | 统计分类结果、硬件访问点与 RAM 使用 |
| `node tools/inventory.mjs [--callers]` | 按例程列出依赖 FC 硬件的访问点与调用者数 |
| `node tools/show.mjs asm\|hex <地址> [数量]` | 查看 PRG 里的一段内容 |
| `node tools/scan-text-area.mjs [--abs] [--imm]` | 列出可能与程序文本区 `$6000-$7FFF` 有关的指令及上下文 |
| `node tools/extract-chr.mjs` | 把 CHR 另存为 `reference/fbasic-chr.bin` |
| `node tools/build-exe.mjs` | 构建 `exe/build/sb2000/FBASIC.EXE`：先汇编链接 ERAM 小段与 32K 映像，再用 fcdev 构建加载器 |
| `node tools/run-exe.mjs <步骤>...` | 在 Ember 的学习机里运行 FBASIC.EXE（开机进 DOS 用 fcdev 公共版的 `enterProgram`），步骤：`w120` 等帧、`mED,3` 读内存、`kEnter` 按键、`dShiftLeft` 按下不松、`uShiftLeft` 松开、`r` 打印 CPU 与 PPU 状态、`sboot.png` 截图（存到 `build/run/`） |

## 目录

```text
apps/sb2000/fbasic/
  analysis/    分析结果：CDL、代码入口、跳过前缀、数据布局、立即数地址、覆盖脚本（coverage/）
  reference/   生成的原版可重定位源码（fbasic-nes.s）与 CHR
  tools/       分析与 FC 模式验证工具，以及构建与运行学习机版的脚本（lib/ 是共用模块）
  image/       映像与外壳的源码：fbasic.s（手工维护的移植源码）、platform.inc、platform-code.inc（画面初始化、PPU 写入、GAME 数据的 DMA）、keyboard.inc（SB2000 键盘）、game.s（GAME 0-3 的数据）、stub.s（ERAM 小段：进入、入口门、退出）、shell.inc（共用常量）
  exe/         SB-DOS 程序外壳（fcdev 项目）：loader.s 把 32K 映像与小段放进程序文件并启动
  docs/        移植计划、移植对照表
  test/        分析的测试
  build/       中间文件、截图与重排 ROM（不进 Git）
```
