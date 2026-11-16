# fbasic（SB2000 版）

把任天堂 Family BASIC v3.0（NS-HUBASIC V3.0，NINTENDO/SHARP/HUDSON）原生移植成小霸王学习机 SB2000 的 SB-DOS 程序（`FBASIC.EXE`，6576 模式）。原生移植的意思是：改写依赖 FC 机器的部分（画面、键盘、声音、手柄、程序存储器、退出），直接使用 SB2000 的硬件与 SB-DOS，不做 NES 兼容层，也不是 FC 模式程序。BASIC 解释器本身与数据不改。

计划与决定见 [docs/移植计划.md](docs/移植计划.md)。

## 当前状态

| 阶段 | 内容 | 状态 |
| --- | --- | --- |
| 1 | 把原版还原成可重定位的符号源码 | 完成（见下） |
| 2 | 移植对照表：每处依赖 FC 机器的例程与 SB2000 做法 | 完成，见 [docs/移植对照表.md](docs/移植对照表.md)，仍有 8 项“待查”要在阶段 3、4 里查清 |
| 3 | EXE 骨架：多页、库页切换与跨页调用、程序文本页、启动与退出 | 完成（[exe/](exe)），在 Ember 的学习机里验证，见下 |
| 4 | 原生改写：内存与系统、画面文字、键盘、图形与精灵、声音、手柄、存取 | 未开始 |
| 5 | 验证：同一份按键脚本分别在 FC 模式（原版）与学习机（新版）里运行 | 未开始 |

阶段 1 的结果（`node tools/stats.mjs`）：

- [reference/fbasic-nes.s](reference/fbasic-nes.s)：原版 PRG 的可重定位 ca65 源码，8643 个条目、1450 个被引用的地址；汇编后与原 PRG 逐字节一致。
- PRG 32768 字节：代码 18185（8595 条指令，16 处“跳过前缀”），数据 14583（字节表 13559、地址表 572、数值表 144、文字 308），没有未分类的字节。
- 硬件访问点：PPU 80 处（`$2007` 43、`$2006` 17、`$2001` 8、`$2000` 5、`$2005` 4、`$2002` 2、`$2003` 1），声音 21 处，`$4016` 13 处、`$4017` 6 处，程序存储器 `$6000-$7FFF` 21 处。
- 验证：CDL 里执行过的 13994 个字节全部被分类为代码；把源码里 284 块例程与数据随机重排后重新汇编，在 Ember 的 FC 模式里跑同一批脚本，每个脚本结束时的截图与原版逐字节相同（种子 1 跑了前 6 个脚本，种子 2 跑了全部 8 个）。重排验证不到的是没执行过的约 4200 字节代码，它们的地址常量只做了静态复核。
- CHR 另存为 [reference/fbasic-chr.bin](reference/fbasic-chr.bin)（8192 字节）。
- `$00ED` 是 NMI 在 RAM 里的入口：启动时往 `$ED-$EF` 写 `JMP $8971`；原版有 16 处“`$2C`/`$AE`/`$EC` 吞掉后一条指令”的写法，源码里写成 `.byte` 加后一条指令。

阶段 3 的骨架（[exe/src/start.s](exe/src/start.s)，`fcdev.json` 里 `banks` 为 2，文件 5 页 20480 字节）：页 0、1 常驻，页 2 是库页 1（窗口 2），页 3 是窗口 3，作程序文本页，页 4 是库页 2。骨架进入时保存零页与 `$0200-$07FF`，内核 NMI 每帧调用钩子 5，键盘自己轮询，库页切换先写窗口影子再写硬件寄存器，跨页调用经 `bank_call`，调用 SB-DOS 服务经“DOS 入口门”换回 DOS 现场，按 `ESC` 还原现场并返回 DOS。在 Ember 的学习机里核对过（`node tools/exe-selftest.mjs`）：钩子每帧运行；自检循环里跨页调用、程序文本页读写、窗口影子都没有错误（NMI 随时打断也不出错）；入口门往返申请并释放 1 页；退出后 DOS 的 `dir` 正常；第二次运行申请到的页号与第一次相同，没有泄漏。

## 命令

在本目录运行；记录与比较需要 Ember 打包版（环境变量 `FCDEV_EMULATOR` 指向 `ember.exe`，默认 `../../../ember/out/Ember-win32-x64/ember.exe`）。

| 命令 | 作用 |
| --- | --- |
| `npm test` | 分类覆盖、跳过前缀、立即数地址、CDL 一致、汇编回读与重排汇编的测试 |
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
| `node tools/exe-selftest.mjs` | 在 Ember 的学习机里运行骨架并自检（截图在 `build/selftest/`） |

## 目录

```text
apps/sb2000/fbasic/
  analysis/    分析结果：CDL、代码入口、跳过前缀、数据布局、立即数地址、覆盖脚本（coverage/）
  reference/   生成的原版可重定位源码（fbasic-nes.s）与 CHR
  tools/       分析、生成与运行验证工具（lib/ 是共用模块）
  docs/        移植计划、移植对照表
  exe/         SB-DOS 程序（fcdev 项目，阶段 3 的骨架，阶段 4 在此改写）
  test/        阶段 1 的测试
  build/       中间文件、截图与重排 ROM（不进 Git）
```
