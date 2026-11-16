# fbasic（SB2000 版）

把任天堂 Family BASIC v3.0（NS-HUBASIC V3.0，NINTENDO/SHARP/HUDSON）原生移植成小霸王学习机 SB2000 的 SB-DOS 程序（`FBASIC.EXE`，6576 模式）。原生移植的意思是：改写依赖 FC 机器的部分（画面、键盘、声音、手柄、程序存储器、退出），直接使用 SB2000 的硬件与 SB-DOS，不做 NES 兼容层，也不是 FC 模式程序。BASIC 解释器本身与数据不改。

计划与决定见 [docs/移植计划.md](docs/移植计划.md)。

## 当前状态

| 阶段 | 内容 | 状态 |
| --- | --- | --- |
| 1 | 把原版还原成可重定位的符号源码 | 完成（见下） |
| 2 | 移植对照表：每处依赖 FC 机器的例程与 SB2000 做法 | 完成，见 [docs/移植对照表.md](docs/移植对照表.md)，仍有 8 项“待查”要在阶段 3、4 里查清 |
| 3 | EXE 骨架：多页、库页切换与跨页调用、程序文本页、启动与退出 | 完成（在 Ember 的学习机里验证过；骨架已被 4.1 的外壳取代，见下） |
| 4 | 原生改写：内存与系统、画面文字、键盘、图形与精灵、声音、手柄、存取 | 进行中：4.1 内存与系统、4.2 文本画面、4.3 键盘与退出完成，见下 |
| 5 | 验证：同一份按键脚本分别在 FC 模式（原版）与学习机（新版）里运行 | 未开始 |

阶段 1 的结果（`node tools/stats.mjs`）：

- [reference/fbasic-nes.s](reference/fbasic-nes.s)：原版 PRG 的可重定位 ca65 源码，8643 个条目、1450 个被引用的地址；汇编后与原 PRG 逐字节一致。
- PRG 32768 字节：代码 18185（8595 条指令，16 处“跳过前缀”），数据 14583（字节表 13559、地址表 572、数值表 144、文字 308），没有未分类的字节。
- 硬件访问点：PPU 80 处（`$2007` 43、`$2006` 17、`$2001` 8、`$2000` 5、`$2005` 4、`$2002` 2、`$2003` 1），声音 21 处，`$4016` 13 处、`$4017` 6 处，程序存储器 `$6000-$7FFF` 21 处。
- 验证：CDL 里执行过的 13994 个字节全部被分类为代码；把源码里 284 块例程与数据随机重排后重新汇编，在 Ember 的 FC 模式里跑同一批脚本，每个脚本结束时的截图与原版逐字节相同（种子 1 跑了前 6 个脚本，种子 2 跑了全部 8 个）。重排验证不到的是没执行过的约 4200 字节代码，它们的地址常量只做了静态复核。
- CHR 另存为 [reference/fbasic-chr.bin](reference/fbasic-chr.bin)（8192 字节）。
- `$00ED` 是 NMI 在 RAM 里的入口：启动时往 `$ED-$EF` 写 `JMP $8971`；原版有 16 处“`$2C`/`$AE`/`$EC` 吞掉后一条指令”的写法，源码里写成 `.byte` 加后一条指令。

阶段 3 的骨架验证过：钩子每帧运行、跨页调用与程序文本页读写无误（NMI 随时打断也不出错）、入口门往返申请并释放 1 页、退出后 DOS 正常、重复运行页号不变。

阶段 4.1 的结果（分页方案与各项决定见 [docs/移植对照表.md](docs/移植对照表.md) 第 3.2、3.4 节）：

- [tools/port.mjs](tools/port.mjs) 按 `analysis/banks.txt` 把原版源码拆成常驻页与各库页，生成 `exe/src/core.s`（解释器，不要手改）与 `exe/src/layout.inc`；转向库页的 `jsr`、`jmp` 与地址表都经 203 个跳板；NMI 例程改成由钩子调用、以 `rts` 返回；`$1E/$1F` 改用 `$38/$39`，`$0600-$06FF` 改到常驻页里的 `fb_page6`，程序文本区 `$6000-$6FFF` 搬到窗口 3 的页（`$B000-$BFFF`）。
- [exe/src/platform.s](exe/src/platform.s) 是手写的外壳：入口、NMI 钩子 5（先保存跳板的临时变量）、库页切换（先写窗口影子再写硬件）、跳板进入例程 `far_enter`、DOS 入口门（DOS 现场与程序现场在一个缓冲库页里互换）。`fcdev.json` 里 `banks` 为 11，文件 14 页（57344 字节）。
- 在 Ember 的学习机里（`node tools/exe-run.mjs`）：原版初始化经 NMI 命令握手走完，进入键盘扫描的空转循环；入口门往返申请并释放 1 页后初始化照常走完（临时加代码验证过一次，已移除，现在没有调用者；4.7 的磁盘存取会用到）。程序文本区还没用 BASIC 程序核对。

阶段 4.2 的结果（做法见 [docs/移植对照表.md](docs/移植对照表.md) 第 2.2.1 节）：

- 原版对 NES 的 PPU 寄存器 `$2000`、`$2001`、`$2002`、`$2005`、`$2006`、`$2007` 的全部 82 处访问，由生成器换成对 [exe/src/platform.s](exe/src/platform.s) 里 PPU 转换层（`ppu_*`）的调用，原版例程不改。转换层把 NES 名称表 0、1 写成 6576 的 2 字节名称表项（每个单元同时写在行 r 与行 r+30，以匹配 NES 的 30 行回绕），属性表改成每个单元高 4 位的调色板组，调色板直接写 `$2040` 起。背景字库（CHR 的 `$1000-$1FFF`）放在专用库页，启动时写进扩展显存 `$8000` 起。
- 常驻页装不下转换层，分页重新调成常驻代码 2000 字节（`plan-banks --resident 2000 --bank 3500`）。
- 在学习机里运行，开机显示“键盘未连接”提示（还没有键盘），与 FC 模式里原版（不接键盘）逐像素比较（`node tools/fc-shot.mjs`、`node tools/diff-png.mjs`）：文字位置完全一致，只有字形像素的颜色不同（Ember 两种 PPU 的 RGB 表不同，白色是 `FFFFFF` 与 `ECEEEC`）。名称表、属性表重写、行 r+30 的镜像、读回与调色板读写还用临时自检代码写进去再读显存核对过（已移除）。精灵图案表、BG 画面与属性表的实际使用要等键盘之后用 BASIC 命令核对。没有键盘和退出路径，所以运行后要重启 Ember 才能回 DOS。

阶段 4.3 的结果（做法见 [docs/移植对照表.md](docs/移植对照表.md) 第 2.3 节）：

- 键盘按“字符”映射：`tools/gen-keymap.mjs` 把 PC 键盘（美式键位）的扫描码译成原版键盘矩阵里的键与是否按 SHIFT，生成 `exe/src/keymap.s`；键盘服务 `exe/src/keyboard.s` 在库页 12，每个按键排队，每个键至少让原版扫到 6 次再松开、松开后空闲 6 次再送下一个。
- F12 退出到 DOS（恢复 DOS 的显示状态），End 对应原版的 STOP 键。
- 跳板在目标就在当前窗口 2 的库页里时直接跳转，不替换返回地址：FOR/GOSUB 把调用者的返回地址存进栈帧，之后由 NEXT/RETURN 返回；跨页时返回地址换成 `far_return`，软栈里的条目与它对不上，所以这类例程不能经换页的跳板。
- 原版 `$0300` 页的行缓冲改到 `fb_linebuf`（页对齐，避开内核 NMI 每帧改写的 `$0300-$0306`），`$0600-$07FF` 的 `fb_page6` 也页对齐，字符串临时区的指针高字节同步改成它的第二页。
- 在学习机里键入 `10 FOR I=1 TO 3` / `20 PRINT I*2;` / `30 NEXT` / `RUN` 输出 ` 2 4 6`，`GOSUB`/`RETURN`、`PRINT "AB"`、字符串变量、`ASC`、`LEN` 结果正确（看截图核对）；还没有与 FC 模式原版逐命令对照，也没有核对全部关键字。

## 命令

在本目录运行；记录与比较需要 Ember 打包版（环境变量 `FCDEV_EMULATOR` 指向 `ember.exe`，默认 `../../../ember/out/Ember-win32-x64/ember.exe`）。

| 命令 | 作用 |
| --- | --- |
| `npm test` | 分类覆盖、跳过前缀、立即数地址、CDL 一致、汇编回读与重排汇编，以及移植生成器输出的测试 |
| `node tools/record-coverage.mjs [脚本名片段...]` | 在 FC 模式里让原版按 `analysis/coverage/` 里的脚本运行，记录 CDL 并合并到 `analysis/prg.cdl`，截图在 `build/shots/` |
| `node tools/record-coverage.mjs --rom <nes 文件> --shots <目录>` | 换一个 ROM 重跑同一批脚本，只留截图，用于重排 ROM 的行为比较 |
| `node tools/compare-shots.mjs <目录甲> <目录乙>` | 比较两个目录里同名截图是否逐字节相同 |
| `node tools/extract-seeds.mjs` | 由 CDL 取出指令起点 `analysis/cdl-entries.txt` |
| `node tools/discover.mjs [--list] [--write-entries]` | 试探没执行过的代码，列出候选与剩余区段 |
| `node tools/show-unknown.mjs [起 [止]]` | 列出还没分类的区段 |
| `node tools/show.mjs asm|hex <地址> [数量]` | 查看 PRG 里的一段内容 |
| `node tools/scan-tables.mjs`、`scan-pointer-pairs.mjs`、`scan-immediates.mjs`、`scan-skip-prefixes.mjs`、`scan-uncovered.mjs` | 找地址表、立即数拆开的地址、跳过前缀，以及只在未执行代码里的候选 |
| `node tools/check-cdl.mjs` | 核对 CDL 与分类是否一致 |
| `node tools/lift.mjs [--permute <种子>]` | 由 `analysis/` 生成 `reference/fbasic-nes.s` 并汇编核对；`--permute` 打乱块顺序，生成 `build/fbasic-permuted.nes` |
| `node tools/stats.mjs` | 统计分类结果、硬件访问点与 RAM 使用 |
| `node tools/inventory.mjs [--callers]` | 按例程列出依赖 FC 硬件的访问点与调用者数 |
| `node tools/extract-chr.mjs` | 把 CHR 另存为 `reference/fbasic-chr.bin` |
| `node ../../../packages/fcdev/bin/fcdev.mjs build exe/src` | 构建 `exe/build/sb2000/FBASIC.EXE` |
| `node tools/exe-run.mjs [帧数]` | 在 Ember 的学习机里运行 `FBASIC.EXE`，采样 PC（带标签名）与几个调试变量，截图在 `build/run/` |
| `node tools/exe-trace.mjs [帧数] [条数]` | 同上，并跟踪执行过的指令（每个地址只记第一次），列出第一次跑进 RAM 之前的指令，用来找跑偏的位置 |
| `node tools/fc-shot.mjs <png> [帧数] [--no-keyboard]` | 在 FC 模式里运行原版并截图，用来和学习机里新版的画面对照 |
| `node tools/diff-png.mjs <甲> <乙>` | 逐像素比较两张 PNG（只在 Windows 上可用） |
| `node tools/fc-ppu-dump.mjs [文字]` | 在 FC 模式里让原版开机并键入文字，导出名称表、调色板与滚动变量，摸清原版的画面模型 |
| `node tools/scan-text-area.mjs [--abs] [--imm]` | 列出可能与程序文本区 `$6000-$7FFF` 有关的指令（绝对地址与 `$5F-$71` 的立即数）及上下文 |
| `node tools/callgraph.mjs [--list]` | 统计代码块、块间引用与 NMI 实际要用到的块 |
| `node tools/plan-banks.mjs [--resident <字节>] [--bank <字节>] [--write]` | 把代码块分配到常驻页与各库页，写入 `analysis/banks.txt`（当前用 `--resident 3400 --bank 3500`） |
| `node tools/port.mjs` | 按分配生成 `exe/src/core.s` 与 `exe/src/layout.inc`，并列出各页大小与需要的 `banks` 个数 |

## 目录

```text
apps/sb2000/fbasic/
  analysis/    分析结果：CDL、代码入口、跳过前缀、数据布局、立即数地址、覆盖脚本（coverage/）
  reference/   生成的原版可重定位源码（fbasic-nes.s）与 CHR
  tools/       分析、生成与运行验证工具（lib/ 是共用模块）
  docs/        移植计划、移植对照表
  exe/         SB-DOS 程序（fcdev 项目）：platform.s 手写外壳，core.s 由 tools/port.mjs 生成
  test/        分析与生成器的测试
  build/       中间文件、截图与重排 ROM（不进 Git）
```
