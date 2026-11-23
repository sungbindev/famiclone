# 步步高 DOS 反汇编与开发库计划

## 目标

1. 以 `reference/floppy-images/bbg/BBGDOS.IMG`（BBGDOS 3.0 系统盘）里的启动扇区、`BBGDOS.SYS`、`COMMAND.CMD` 为准，用 fcdev 的反汇编工程还原成可重新汇编、逐字节一致的 ca65 源码，按功能分模块，符号有含义、例程有注释。
2. 从反汇编结果取得全部系统 API（原厂源码只作参考），包括 BIOS 调用、外设调用、文件调用、DOS 调用，以及它们的子功能、出错码、数据结构，写成带详细注释的符号定义文件（`.inc`，供智能感知使用）和接口文档。
3. 在符号文件之上做汇编开发库，供编写 BBGDOS 下的 `.CMD`、`.EXC` 程序使用；C 语言函数库与头文件在汇编库完成后另行启动，不在本计划范围内。

## 输入资料

| 对象 | 位置 | 大小 | 装入与入口 | 状态 |
| --- | --- | --- | --- | --- |
| 启动扇区 | `BBGDOS.IMG` 第 0 扇区 | 512 字节 | BIOS 读到 `$0400`，校验偏移 3 的 `SMDOS1.0`，从 `$0400 + 偏移 1 的字节 + 2`（即偏移 `$3E`）执行 | 只有二进制 |
| `BBGDOS.SYS` | `BBGDOS.IMG` 根目录，1998-01-11 | 14822 字节 | 文件头无装入地址，`Bios.asm` 的 `SMDOS_Begin` 是 `$7C00`，首条指令 `JMP $7C96` | 只有二进制，与 `reference/docs/BBGDOS源代码/Bbgdos.sys` 逐字节相同 |
| `COMMAND.CMD` | `BBGDOS.IMG` 根目录，1998-01-13 | 15277 字节 | 前 2 字节是装入地址 `$8000`，其余 15275 字节装入 `$8000-$BAAA`，入口 `JMP $BA3D` | 只有二进制，与 `Command.sys`、`Command.asm` 都不是同一版本 |
| 原厂 BIOS 源码 | `reference/docs/BBGDOS源代码/Bios.asm`，已移植为 `machines/bbg/src/bios/bios.s` | 7412 行 | `$C000-$FFFF` | 只作参考：工程版，与发布版 ROM 不是同一次构建，内容不一定准确 |
| 发布版 BIOS | `ember/roms/bbg1_bios.bin`（128KB）、`bbg98_bios.bin`（2MB） | 固定页 16KB 是代码，其余是字库 | `$C000-$FFFF` | 只有二进制，BIOS 接口以它的反汇编为准；`bbg98_bios.bin` 只提取功能号分发表 |
| 使用手册 | `reference/docs/步步高多媒体学生电脑 BIOS系统 DOS系统使用手册（可搜索）.pdf` | | | 系统篇第二至六章是 BIOS、外设、文件、DOS 调用与严重错误接口，数据定义技术手册有 CMD 与 EXC 文件格式 |
| 已还原的符号 | `machines/bbg/src/include/tvdef.inc` | 56 个 BIOS 功能号、24 个文件功能号、17 个 DOS 功能号 | | `DosGetDOSVersion`、`StackTooDepth` 是推测值，三个 BIOS 功能号手册未列 |

原厂源码（`Bios.asm`、`Command.asm`、`Smcdos.asm`）只用来提示名称与用途，与二进制不一致时一律以反汇编的结果为准；命名与注释的依据是代码行为、手册与运行观察。

## 可行性结论

可行。理由与风险如下。

**可行的依据**

- 代码量小：三个对象的代码与数据合计约 30KB，SB2000 的 BIOS 是 512KB。
- 两个系统文件都是单块连续映像：`BBGDOS.SYS` 起始 `$7C00`，`COMMAND.CMD` 起始 `$8000`，没有换页，布局简单。
- 对外接口有官方手册：BIOS、外设、文件、DOS 四套调用的功能号、参数与出错码已有文字依据，反汇编的任务是核对、补全与取得内部实现，不是从零推测。
- BIOS 有参考源码：`Bios.asm` 的例程名与注释可提示用途，但不作为依据，BIOS 调用的约定从发布版 ROM 的反汇编得到。
- 有实际调用样本：对 `COMMAND.CMD` 的静态扫描（统计 `JSR` 与 `JMP` 到 `$58xx` 入口的指令，功能号取前 12 字节内最近的 `LDA #立即数`，是粗略值）找到 31 处 DOS 调用（12 个不同的功能号：0、1、2、3、4、6、7、8、9、11、12、16）、9 处文件调用、25 处 BIOS 调用；`BBGDOS.SYS` 有 63 处 BIOS 调用，用到 29 个不同的功能号。
- Ember 支持 `bbg1` 机型，能载入 BIOS 与软盘镜像开机到 `A:\>`，可以读内存、设断点、记录运行数据。
- 工程实现手段齐备：`vscode/targets/bbg` 已有 `.CMD` 的打包（头 `0080`）、`crt0.s` 与 `asminc/bbg.inc`，`COMMAND.CMD` 可以直接用它回读校验；`apps/bbg/game` 已验证过文件调用与 BIOS 调用的实际用法。

**风险与处理**

| 风险 | 说明 | 处理 |
| --- | --- | --- |
| `BBGDOS.SYS` 的装入布局未核实 | `$7C00` 加 14822 字节会越过 `$8000`，进入换页窗口；`$5800-$5FFF` 的入口表和部分代码很可能由初始化代码搬到别处，文件里的字节顺序不等于运行时地址 | 阶段 0 用模拟器在 `A:\>` 读出整个 64KB 与各页，和文件逐段比对，确定每一段的运行地址，再用 `segments` 分段 |
| fcdev 没有内置的 BBG 目标 | `disasm_open` 对这个机型只能用 `format: "flat"`，符号集和硬件寄存器要自备 | 以 `tvdef.inc` 为起点，经反汇编核对后整理 `.inc`，用 `symbolSets` 导入 |
| 运行记录对 `bbg1` 是否可用未验证 | 技能文档只说明了 NES 与 SB2000 的记录闭环 | 阶段 0 实测 `cdl`、`instruction`、`callLog` 三种记录对 RAM 中的代码是否生效；不可用时用断点与内存读取代替，在报告中写明 |
| 分发表在 RAM 中 | DOS 调用 `$580C`、文件调用 `$5803` 的分发表可能是初始化时才写入的 | 静态追踪找不到的目标用 `callLog` 或断点补全，剩余项在注释中写明原因 |
| BIOS 接口只能来自发布版 ROM | 原厂 `Bios.asm` 与 ROM 不是同一次构建，功能号集合与寄存器约定可能不同 | 阶段 1 反汇编 `bbg1_bios.bin` 的固定页，接口以它为准；`bbg98_bios.bin` 提取分发表，每个功能号标注存在于哪个版本 |
| 手册与实现不一致 | 手册编号有错位（如 `DosGetSetInitScrn`），OCR 文本有错字；`apps/bbg/game` 的目录枚举（`FileFirst`、`FileNext`）参数约定至今没有查清 | 以反汇编结果与模拟器实测为准，差异记入“手册差异表”；目录枚举作为文件调用阶段的第一个验证用例 |

## 工程布局

反汇编工程放在 `machines/bbg/disasm/`，一个对象一个工程，共享符号放在 `machines/bbg/src/include/`：

```text
machines/bbg/
  src/bios/                 原厂 BIOS 源码的移植（已有，只作参考）
  src/include/              共享符号定义（供四个工程与开发库使用）
    tvdef.inc               已还原的符号（阶段 1 后由下面的分节文件取代）
    bios-calls.inc          BIOS 与外设调用：入口、功能号、子功能号、参数单元
    dos-calls.inc           文件调用与 DOS 调用：功能号、通道结构、出错码
    system-memory.inc       零页、$0100-$07FF、$5800-$7FFF 的系统变量与固定地址
    hardware.inc            $FF00 起的系统寄存器与 PPU、APU、软驱等 I/O
  disasm/
    bios-rom/               发布版 BIOS 固定页（bbg1_bios.bin 的 $C000-$FFFF）工程
    boot-sector/            启动扇区工程
    bbgdos/                 BBGDOS.SYS 工程
    command/                COMMAND.CMD 工程
  docs/
    DOS反汇编与开发库计划.md  本文件
    api/                    接口文档（阶段 5 生成）
```

- 每个工程的 `disasm.json` 是唯一的修改入口，`src/`、`include/`、`link.cfg`、`verify/` 由 `disasm_generate` 生成，不手改。
- 模块名能否带目录（如 `file-system/fat`）在阶段 0 试验；不能时用连字符前缀（`file-system-fat.s`），仍按功能分文件。
- 命名沿用 SB2000 的约定：例程与全局标签用大写字母加下划线，前缀写所属模块（`FS_OPEN`、`MEM_ALLOC_PAGE`）；内部跳转用局部标签；功能号等常量用 `BIOS_`、`DEV_`、`FILE_`、`DOS_`、`ERR_` 前缀，注释里写手册里的原名（如 `DosReadKeyEcho`）；硬件寄存器用 `IO_`、`PPU_` 前缀。
- 每个 `.inc` 符号的行尾都写含义，格式与 `vscode/targets/sb2000/asminc/sb2000.inc` 一致，使编辑器悬停可显示。

## 总量与统计方法

阶段 0 之后把统计数字写入本节。统计方法固定为：

| 项目 | 方法 |
| --- | --- |
| 例程、变量、区域数量 | 四个工程各自 `disasm_status` 的 `symbols`、`regions`、`unresolved` 字段 |
| API 总量 | 三个调用入口（`$5800`、`$5803`、`$580C`）加外设调用 `$5809` 的分发表项数；BIOS 以反汇编出的分发表为准 |
| API 覆盖 | 接口文档里已写完整条目的功能号数除以分发表项数，要求 100% |
| 手册差异 | 手册表中的功能号与反汇编结果逐项比较，条目写入 `docs/api/manual-diff.md` |

已知起点：手册与 `tvdef.inc` 有 BIOS 56 个、文件调用 24 个、DOS 调用 17 个功能号；反汇编后总量以分发表为准。

## 阶段

按依赖顺序排列，一个阶段做完并达到完成标准再进入下一个。

| 阶段 | 内容 | 完成标准 |
| --- | --- | --- |
| 0 资料基线与探针 | 提取三个对象并记录大小与哈希；Ember 载入 `bbg1` 加 `bbg1_bios.bin` 加 `BBGDOS.IMG` 开机到 `A:\>`，读 64KB 与各页，确定 `BBGDOS.SYS` 的装入与搬运布局；试验 `disasm_open` 的 flat 与 `segments`、模块目录名、三种运行记录的可用性 | 布局表写入本文件；四项探针（flat 打开、模块目录、记录可用性、模拟器核对手段）各有“可用”或“不可用及替代做法”的结论 |
| 1 BIOS 接口基线 | 按八步流程反汇编 `bbg1_bios.bin` 的固定页，分发表与每个 BIOS 功能的实现都来自反汇编；提取 `bbg98_bios.bin` 的分发表对照；整理固定地址变量与硬件寄存器；原厂源码与手册只用来提示名称 | 八步标准达标；`bios-calls.inc`、`system-memory.inc`、`hardware.inc` 完成；每个 BIOS 功能号有名称、入口寄存器、出口、副作用、存在的 BIOS 版本，与手册不符的列入差异表 |
| 2 启动扇区 | 作为试点跑通八步流程：反汇编启动扇区，弄清它如何读 FAT、找到并装入 `BBGDOS.SYS` | 八步完成标准全部达标；`disasm_verify` 为 `ok` |
| 3 `COMMAND.CMD` | 单段 `$8000`，先反汇编它：它是 DOS 调用与文件调用的使用者，31 处 DOS 调用能最快确认调用约定；再用 `bbg` 目标回读构建 | 八步标准达标；用 `bbg` 目标构建出的 `.CMD` 与 `COMMAND.CMD` 逐字节一致 |
| 4 `BBGDOS.SYS` | 核心工作。按子系统分批：初始化与入口表安装、DOS 调用分发、文件调用分发、文件系统（FAT12、目录、通道）、内存页管理、程序装载（CMD 与 EXC）、显示与键盘、严重错误处理、对 BIOS 的扩展功能 | 八步标准达标；用 ca65 与 ld65 按 `$7C00` 构建，与 `BBGDOS.SYS` 逐字节一致 |
| 5 API 提取与文档 | 由分发表导出全部 DOS、文件、外设调用与扩展 BIOS 功能的子功能；整理出错码、文件通道结构、EXC 与 CMD 文件头、程序运行时的低端内存约定；每个功能写入口、出口、进位与出错码、副作用、内核实现位置、手册章节、验证状态；解决目录枚举的参数约定 | `bios-calls.inc` 与 `dos-calls.inc` 补齐；接口文档覆盖分发表 100%；手册差异表完成；目录枚举（`FileFirst`、`FileNext`）在模拟器里实测通过 |
| 6 汇编开发库 | 在符号文件之上写汇编库：一个函数一个文件，参数传递与出错返回统一；扩展 `vscode/targets/bbg` 的 `lib-build.json`、`asminc`；`asminc/bbg.inc` 由符号文件整理而来，只保留面向开发者的符号 | 库覆盖全部可封装的功能号；未封装的功能号在 `api-unwrapped.json` 里写明原因 |
| 7 库验证 | 为库的每个函数写测试程序，在 Ember 的 `bbg1` 上运行并读回内存与镜像核对；`apps/bbg/game` 改用库验证回归 | 每个库函数有测试并通过；接口检查脚本（仿 `check-sb2000-api.mjs`）通过 |

C 语言函数库与头文件：阶段 7 完成后另行立项。

## 每个反汇编阶段的做法

阶段 1、2、3、4 都按 skill `disassemble-6502` 的八个步骤执行；每个工程按功能拆成多个模块文件，不把代码放进同一个文件：

1. 识别：`disasm_open` 用 `format: "flat"`，给 `segments`（运行地址与文件偏移的对应）、`entries`（`$7C00`、`$8000`、`$0400+$3E` 与入口表中的各跳转）和 `symbolSets`（共享 `.inc`）。
2. 追踪：`disasm_trace`；分发表项用 `disasm_detect` 找到后标成 `words`；RAM 中的表与间接跳转靠阶段 0 确定的记录手段补全。
3. 区域分类：字符串（`Starting BBGDOS...`、出错提示、命令名）、地址表、数据表、填充。
4. 命名：以被引用最多的例程与变量为先；名字只依据行为、符号集、手册与运行观察，没有依据的保留自动名或注释写“未确认”。
5. 注释：每个例程写作用、入口参数、返回值、副作用；DOS 调用与 BIOS 调用的每个调用点注明功能号名称。
6. 分层：按功能划分模块，大致为 `boot`、`dispatch`（调用入口与分发）、`file-system`、`disk`、`memory`、`loader`、`display`、`keyboard`、`error`、`shell`（`COMMAND.CMD`）、`messages`。
7. 生成与校验：`disasm_generate`、`disasm_verify`，要求 `verification.state` 为 `ok`。
8. 构建：`COMMAND.CMD` 用 `bbg` 目标构建；`BBGDOS.SYS` 与启动扇区用 ca65 与 ld65 的独立链接配置构建，输出与原文件比较。

## 接口文档与符号文件的内容

接口文档放在 `machines/bbg/docs/api/`，一个调用入口一份，格式沿用 `vscode/targets/sb2000/api-reference.md` 的写法：

| 文档 | 内容 |
| --- | --- |
| `bios-calls.md` | `$5800` 全部功能号：名称、参数寄存器与内存、返回、副作用、需要的页、存在的 BIOS 版本 |
| `device-calls.md` | `$5809` 外设调用与 `BiosDevice` 子功能 |
| `file-calls.md` | `$5803` 文件调用：通道块字段、每个功能的字段用法、出错码；目录枚举的完整用法 |
| `dos-calls.md` | `$580C` DOS 调用：每个功能的参数与返回；程序运行约定（`$0100` 命令行、页分配、驻留、返回码） |
| `program-formats.md` | `CMD` 与 `EXC` 文件头、浮动模块、装入过程 |
| `memory-map.md` | 地址空间、页管理、`$5800-$5FFF` 与低端内存的用途 |
| `error-codes.md` | 全部出错码 |
| `manual-diff.md` | 与手册不一致或手册未列的条目及依据 |

## 记录与验证原则

- 没有在模拟器里运行核对的结论，接口文档的“验证状态”一栏写“未验证”，不写成已验证。
- 资料之间不一致时，以模拟器实测为准，在差异表里写明。
- 只关闭自己启动的 Ember；调试 BBG 时用 `emu_load` 的 `machine: "bbg1"`，`firmware` 为 `ember/roms/bbg1_bios.bin`，`diskImage` 为 `BBGDOS.IMG`。

## 已确定的范围

- 其他版本的系统文件：`floppy-images/bbg` 下另有约 10 个 `BBGDOS.SYS`（14822、15371、15375、16240、16516、16553、16647、16660、16668 字节）和约 7 个 `COMMAND.CMD`，含中文版。只反汇编 `BBGDOS.IMG` 的版本；阶段 5 对其他版本只提取功能号分发表，做“版本支持表”，标注每个功能号存在于哪些版本。
- BIOS 以发布版 ROM 的反汇编为准，原厂源码只作参考。
