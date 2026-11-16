# gbasic（SB2000 版）

把步步高学习机的 BASIC 开发程序 `GBASIC.CMD`（Family BASIC 的步步高版）原生移植成小霸王学习机 SB2000 的 SB-DOS 程序（`.EXE`）。原生移植的意思是：改写 GBASIC 里依赖步步高机器的部分（画面、键盘、声音、文件、内存分页、退出），让它直接使用 SB2000 的硬件与 SB-DOS 的系统调用，不做 NES、BBGDOS 的兼容层。BASIC 解释器本身与数据不改。

原版的反汇编与运行记录在 [apps/bbk/gbasic](../../bbk/gbasic)，只读取不修改。

## 当前状态

| 阶段 | 内容 | 状态 |
| --- | --- | --- |
| 1 | 把原版还原成可重定位的符号源码 | 完成（见下），未执行过的路径的地址常量只做了静态复核 |
| 2 | 移植对照表：每个平台例程的作用与 SB2000 做法、内存使用图 | 完成，见 [docs/移植对照表.md](docs/%E7%A7%BB%E6%A4%8D%E5%AF%B9%E7%85%A7%E8%A1%A8.md)，其中仍标“待查”的事项在对应的阶段 4 子步骤里查清 |
| 3 | SB-DOS 的 EXE 外壳：整机接管（窗口 0-7 全部换成映像自己的页）、DOS 入口门、退出 | 完成（[image/stub.s](image/stub.s)、[exe/src/loader.s](exe/src/loader.s)），在 Ember 里验证 |
| 4 | 原生改写：内存与系统、画面文字、键盘、图形与精灵、声音、文件、手柄 | 进行中：画面、键盘、图形编辑器的精灵与调色板组已完成；声音、文件、手柄未做 |
| 5 | 验证：同一脚本跑原版与新版，比较截图与存取盘结果 | 未开始 |

**方案（2026-11-16 定案，替代早先的“钩子 5 加保留内核”）**：像步步高原版一样整机接管。启动后窗口 0-7 都映射成映像自己的 RAM 页（`$8000-$FFFF` 共 32K），NMI 向量在映像自己的第 7 页里，不用内核的 NMI；PPU 寄存器直接写；IRQ 只开键盘，由映像自己的 IRQ 处理程序读码。调用 SB-DOS 服务时经 ERAM 里的“入口门”（[image/stub.s](image/stub.s)）：把零页与 `$0100-$07FF` 就地换成 DOS 现场、窗口 4-7 换回 ROM 页、调用、再换回。文本区仍是 `$C410` 起（上限 `$F6FF`），与原版一致。

阶段 4 已做的原生改写（[image/gbasic.s](image/gbasic.s) 由原版源码切掉步步高存根后手工维护，平台代码在 [image/platform-code.inc](image/platform-code.inc)，符号与宏在 [image/platform.inc](image/platform.inc)）：

- 画面：名称表在 CRAM，每个图块 2 字节（低字节图案号，高字节高 4 位调色板组、低 4 位图案表位）；背景图案放在扩展显存 `$8000` 起，精灵图案放在字符 RAM `$2000`（表 0）、`$3000`（表 1），`$2008` 位 3-2 选表。`put_tile` 只写图案号并跳过高字节，所以改写文字不改变这个位置的调色板组。调色板组按 2x2 块读写（NMI 命令 16、17、18），CGEN 改背景图案表时重写名称表 0、1 的图案表位。
- 键盘：AT 键盘扫描码集 1，由 IRQ 驱动，维护修饰键与当前按着的键，翻译成原版内部码；按键重复与光标闪烁以帧计时。
- 图形编辑器的名称表存取（FILE）：文件格式仍是原版的 1K（960 个图块号加 64 个属性字节），写入读出时与块调色板组互转；文件层（4.6）做完后才能端到端验证。

**还没做**：声音（PLAY、BEEP 的 `$4017`、`$4010` 写入要删）、文件（BASIC 的 SAVE、LOAD 与图形编辑器的 FILE 现在是占位，调用一律失败；原版的磁带例程要删）、手柄（`STICK`、`STRIG`）、残留的步步高等式与 `L7D04` 一带的清理。

阶段 1 的产物与验证：

- [reference/gbasic-bbk.s](reference/gbasic-bbk.s)：原版的可重定位 ca65 源码（7725 个条目，1293 个被引用的地址）。汇编后与原映像逐字节一致；整体平移加载地址（`$7B40`、`$7B80`、`$7C00`）重新汇编后在步步高模拟器里运行，BASIC 阶段 75 行输入与图形编辑阶段（63 个固定按键加 300 个固定种子的随机按键）的截图与原版逐字节相同。
- [analysis/](analysis)：分析结果，生成源码的唯一输入，不手改源码。
- 没有覆盖到的路径（如 SAVE、LOAD、文件出错、打印机）只做了静态复核：代码里 `lda/ldx/ldy #立即数` 拼成映像地址的写法逐条看过，数据里的地址表按布局文件标成地址。移植版测试时要补这些路径。

## 已查明的结论

- 映像 `$7B00-$E40F` 共 26896 字节：`$7B00-$7FFF` 是常驻在公共 RAM 里的部分（文件参数块、BBGDOS 调用存根、页切换与启动代码、十六进制监视器），`$8000-$C127` 是 BASIC 解释器与图形编辑代码，`$C128-$C3FF` 是关键字表、语句与函数的处理例程地址表、开机文字与菜单记录、调色板，`$C400` 起是图案与字库数据。其中 `$C410-$C414` 实际是 5 个单元的变量（程序运行时往里写），原样放在图案数据的开头。
- 原版用“跳过前缀”技巧：`lda #n` 后面放一个 `$AE`（`ldx 绝对地址`）把下一条双字节指令吞掉，另一个入口直接跳到被吞的指令，共 13 处（[analysis/skip-prefixes.txt](analysis/skip-prefixes.txt)）。源码里写成一个 `.byte $AE` 加后面的指令，不是 `ldx`。
- 步步高的 `$BFFA/$BFFB`（NMI 向量）落在映像的地址范围内，其实是 BIOS 的 SetPage($FF) 让 `$8000-$BFFF` 与 `$C000-$FFFF` 同页后对 `$FFFA/$FFFB` 的写，源码里按外部符号处理（[analysis/symbols.txt](analysis/symbols.txt) 里以 `!` 标记）。
- `$9477` 的 `jmp $9469` 落在 `beq` 的操作数上，是原版代码的疑点（多半是原版的缺陷，运行记录里没执行过），源码里照原样保留。
- 运行记录按 CPU 地址记，分页内存里别的页（BIOS、BBGDOS）的指令也会落在 `$8000-$BFFF`，所以要核对操作码与操作数才能当作 GBASIC 的指令。
- `FRE(0)` 的结果随映像大小变化，比较两个版本时不能用。

阶段 3 的外壳：[exe/src/loader.s](exe/src/loader.s) 把 32K 映像放进程序文件的 8 页，启动时把 ERAM 小段（`$7E00` 起）复制好、填窗口页号表后进入映像；映像退出时还原 DOS 现场并回到 DOS。在 Ember 里核对过：NMI 每帧运行、入口门往返申请与释放页正常、退出后 DOS 提示符与 `dir` 正常。策略依据见 [docs/移植对照表.md](docs/%E7%A7%BB%E6%A4%8D%E5%AF%B9%E7%85%A7%E8%A1%A8.md)（第 3 节写的是早先的钩子 5 方案，已被上面的整机接管取代）。

## 命令

在本目录运行：

| 命令 | 作用 |
| --- | --- |
| `npm test` | 分类覆盖、跳过前缀、立即数地址、汇编回读的测试 |
| `node tools/lift.mjs` | 由 `analysis/` 生成 `reference/gbasic-bbk.s` 并汇编核对；`--base <地址>` 整体平移加载地址，`--shift <字节> [--split <地址>]` 把高半部分后移，`--why <地址>` 查引用来源，`--writes` 列出写映像内地址的指令 |
| `node tools/extract-seeds.mjs` | 从步步高运行轨迹提取代码种子 `analysis/trace-entries.txt` |
| `node tools/discover.mjs [--list] [--write-entries]` | 试探没执行过的代码，列出候选与剩余区段 |
| `node tools/scan-pointer-pairs.mjs [--write]` | 找“立即数加零页指针”的地址对 |
| `node tools/scan-immediates.mjs [--window N] [--context]` | 找立即数里拆开的地址候选，要逐条人工看 |
| `node tools/inventory.mjs [--ram]` | 按例程列出全部平台依赖点（硬件访问与服务调用），`--ram` 另列 RAM 使用统计 |
| `node tools/run-bbk.mjs --name <标签> [--bin <二进制> --load <地址>]` | 在步步高模拟器里按固定脚本运行并截图（需要调试版模拟器与 BIOS ROM） |
| `node tools/compare-shots.mjs <标签甲> <标签乙>` | 比较两次运行的截图 |
| `node tools/compare-dumps.mjs`、`node tools/compare-traces.mjs` | 比较两次运行的内存转存或指令轨迹 |

在 `exe/` 目录运行（需要 Ember 打包版，环境变量 `FCDEV_EMULATOR` 指向 `ember.exe`）：

| 命令 | 作用 |
| --- | --- |
| `node ../../../../packages/fcdev/bin/fcdev.mjs build src` | 生成 `build/sb2000/GBASIC.EXE`（要先在上级目录运行 `node tools/build-image.mjs`） |

在本目录运行的调试脚本（开机进 SB-DOS 的顺序用 fcdev 公共版的 `enterProgram`）：

| 命令 | 作用 |
| --- | --- |
| `node tools/build-exe.mjs` | 一键构建：映像与 `GBASIC.EXE` |
| `node tools/shoot.mjs <标签> <步骤>...` | 在 Ember 里运行 GBASIC.EXE，按步骤等待帧数（`w300`）、按键（`kDigit1`）、输入文字（`tPRINT_12\n`，`_` 是空格）、截图（`s`，存到 `build/session/`）、读内存（`mFC,1`） |
| `node tools/debug-state.mjs`、`node tools/debug-sprite.mjs` | 进入图形编辑器后读零页与精灵相关状态的一次性调试脚本 |

## 目录

```text
apps/sb2000/gbasic/
  analysis/    分析结果：轨迹种子、代码入口、跳过前缀、数据区段与布局、符号、立即数地址
  reference/   生成的原版可重定位源码（gbasic-bbk.s）
  tools/       分析、生成与运行验证工具（lib/ 是共用模块）
  docs/        移植对照表
  image/       移植版的 32K 映像源码：gbasic.s（解释器，手工维护）、platform-code.inc 与 platform.inc（平台层）、stub.s（ERAM 小段与入口门）、layout.s 与 image.cfg（布局）
  exe/         SB-DOS 程序外壳（fcdev 项目，把映像放进程序文件并启动）
  test/        阶段 1 的测试
  build/       中间文件与截图（不进 Git）
```
