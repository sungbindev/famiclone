---
name: disassemble-6502
description: '把 6502 程序（NES 卡带、学习机 BIOS、SB-DOS 的 .EXE、任意平铺二进制）反汇编成可重新汇编、逐字节一致的 ca65 工程时使用：给出开始前要收集的输入、新建与继续的判断、八个步骤各自的操作顺序与完成标准、区域类型与参数、命名与注释的依据、用模拟器记录补全静态追踪、回读校验与失败时的处理。'
---

# 反汇编 6502 程序

目标：输入一个 6502 映像，输出一个 ca65 工程，满足四点：重新汇编链接后与原映像逐字节一致；符号有含义明确的名字；代码有注释；按功能拆成多个源文件。

对象是工程（`disasm.json` 加映像），源码由工程与映像生成，每次结果确定。所有修改都经 `disasm_*` 工具，不手改生成的 `src/`、`include/`、`link.cfg`。

## 必须先读取的参考文件

| 情形 | 必须先读取 |
| --- | --- |
| 打开工程之前，输入是 NES 的非 NROM 映射器、学习机 BIOS、多页 `.EXE`，或其他机器的映像 | [targets.md](references/targets.md) |
| 第 2 步要用模拟器记录运行数据 | [records.md](references/records.md)，再读 skill `ember-emulator` |

## 开始前

按下表收集输入，缺少的先问用户，一次问全：

| 输入 | 来源 | 缺少时 |
| --- | --- | --- |
| 映像文件路径 | 用户的话 | 问用户 |
| 工程目录 `projectDir` | 用户的话 | 取工作区根目录下的 `disasm/<映像主名，小写>`，并在开始时告诉用户；必须在工作区里 |
| 映像种类 | 第 1 步的识别表 | 识别表判断不了时问用户 |
| 学习机 BIOS 路径 | 用户的话，仅第 2 步要运行 SB-DOS 程序或研究 BIOS 时需要 | 没有就不做依赖它的运行记录，报告里写明 |

新建还是继续：`projectDir` 里已有 `disasm.json` 就是继续，调用 `disasm_open` 只给 `projectDir`，再调用 `disasm_status`，从第一个未达标的步骤做起；没有就新建，从第 1 步做起。

任务开始时用 todo 列出八个步骤，一步做完并达到完成标准再进入下一步。

## 步骤与完成标准

| 步骤 | 做什么 | 完成标准 |
| --- | --- | --- |
| 1 识别 | 确定输入种类与布局，新建工程，登记符号集 | `disasm_layout` 的 `uncovered` 里没有属于程序的字节，向量与入口已登记 |
| 2 追踪 | 静态追踪；需要时运行并导入记录，再追踪 | `disasm_status` 的 `unresolved.indirectJumps` 与 `unresolved.crossBank` 为 0，或每一项都在注释里写明无法确定的原因 |
| 3 区域分类 | 识别并确认地址表、字符串、图形数据、填充 | `regions.unknown.bytes` 为 0，或每处剩余范围都写明原因 |
| 4 命名 | 自动命名的符号换成语义名 | `symbols.auto` 为 0 |
| 5 注释 | 给例程写说明，给关键指令与硬件访问写注释 | `proceduresWithoutComment` 为 0 |
| 6 分层 | 按功能划分模块 | 每个例程属于一个模块 |
| 7 生成与校验 | 生成源码并回读 | `verification.state` 为 `ok`，且之后没有再编辑 |
| 8 构建 | 目标支持时 `disasm_build` | 产物与原映像一致；不支持的目标跳过 |

每做完一批编辑调用一次 `disasm_status`，用它的数字判断是否达标，不凭印象。`image.stale` 为 `true` 表示映像文件变了、工程已过期，停下汇报。

通用规则：

- 一次 `disasm_apply` 带一批相关的编辑（一个例程及它的变量与注释，或 10 到 30 项同类编辑），整批生效或整批不改。
- 列表类工具分页：用 `limit` 与上一页返回的 `nextCursor`，不要一次要全部。
- 工具返回错误时，按指令“6502 工具使用规则”处理：读 `code` 与 `hint`，改正参数后最多重试 1 次。

## 1 识别

先看文件头与大小，按下表选打开方式，调用 `disasm_open`：

| 输入 | 参数 |
| --- | --- |
| NES 卡带（`.nes`，以 `NES` 加 `$1A` 开头） | `image`、`target: "nes"` |
| 需要 Family BASIC 键盘的卡带 | 同上，加 `input: "family-basic"`（反汇编不受影响，运行时会先接入键盘） |
| SB-DOS 程序（`.EXE`，头 `4C ?? ?? AA 55`） | `target: "sb2000"`（默认输入 `program`） |
| 学习机 BIOS 文件 | `target: "sb2000"`、`input: "bios"` |
| 其他 6502 映像 | `format: "flat"`，给 `address`（加载地址）或 `segments`（换页时每段的 `id`、`fileOffset`、`length`、`address`、`bank`）；入口用 `entries`，符号集用 `symbolSets` |

`target` 会自动选映像格式、登记符号集（硬件寄存器的名字）并记下图形数据端口。打开后做三项检查：

1. `disasm_layout`：`uncovered` 里没有应该属于程序的字节；每段的地址、bank 合理。
2. `disasm_list`，`category: "entries"`：向量或文件头的入口都在。
3. 布局只是初始猜测的情形（分页 ROM、MMC3 的 `$A000` 窗口、不在内置表里的映射器、学习机 BIOS）：按 [targets.md](references/targets.md) 用模拟器核实，确认后 `disasm_layout` 给 `segments` 替换。替换会丢掉手工标注的区域，所以要在第 3 步之前做。

## 2 追踪

1. `disasm_trace`：从入口沿控制流标出指令，识别经地址表分发的跳转，并给新发现的目标自动命名（`sub_`、`loc_`、`data_`、`var_` 加地址）。可在 `entries` 里同时加入新入口。
2. `disasm_list`，`category: "unresolved"`。逐项处理，每项最多一轮：
   - `disasm_routine` 看它所在的例程，推断目标表在哪；表在 RAM 里要靠运行记录。
   - 能确定目标地址：`disasm_apply` 的 `addEntry`，或下一次 `disasm_trace` 的 `entries`。
   - 不能确定：在注释里写“未确认：间接跳转的目标取决于……”。
3. 需要补静态追踪看不到的代码，并且有模拟器与所需文件时：按 [records.md](references/records.md) 记录并导入，再 `disasm_trace`，回到本步第 2 项看剩余的 `unresolved`。
4. `unknown` 里较短的范围（几个到几十字节）多半是没有被调用到的库代码：`disasm_hex` 看内容像指令，就把范围起点加为入口再追踪；较长的范围留到第 3 步按数据处理。

## 3 区域分类

1. `disasm_list`，`category: "unknown"`，列出没有识别的字节范围。
2. `disasm_detect` 给候选与依据，不修改工程。`types` 取 `padding`、`words`、`string`、`graphics`；`graphics` 需要 `graphicsPorts`，用 `target` 打开的工程已经带上。
3. 用 `disasm_hex`、`disasm_find`、`disasm_xrefs` 核实：谁读了这段数据、步长多少。确认后用 `disasm_apply` 的 `setRegion` 标注：

| `type` | 用于 | `params` |
| --- | --- | --- |
| `code` | 指令（追踪通常已标好） | 无 |
| `bytes` | 一般数据 | `perLine`（1 到 32） |
| `words` | 16 位地址表，长度必须是偶数 | `perLine`、`adjust`（0 或 1，RTS 表为 1） |
| `string` | 字符串 | `encoding`、`terminator`（`null`、`0` 或 `"high-bit"`） |
| `graphics` | 图形数据（如 CHR 图块） | `format` |
| `padding` | 填充 | `value` |

- `disasm_detect` 的图形数据候选，长度只是上限（取到下一个已标注的字节）。要看循环次数与目标显存地址（例如 `$3F00` 是调色板，32 字节）才能确定实际长度，再把后面的字节分开标注。
- 数据区里夹着带 `$FFFF` 占位的字节，多半是复制到 RAM 再改写的代码模板：标成 `bytes` 并注明“未确认”，不标 `code`。
- 分不清的数据留作 `bytes` 并写注释；整张表被拆成多项时按表项边界切分，不把字表切在奇数偏移上。

## 4 命名

命名、注释与区域标注的依据只有三类：代码行为、符号集与硬件资料、运行观察。没有依据就保留自动名，或在注释里写“未确认：……”，不猜。

1. 取名顺序：`disasm_list` 的 `auto-symbols` 加 `disasm_xrefs` 看引用数，从被引用最多的例程与变量开始；`disasm_callgraph` 看层次。
2. 命名前先 `disasm_routine`：看调用者、被调用者、读写了哪些地址、入口出口寄存器的推测（线性推测，只作参考）。
3. `disasm_apply`：`renameSymbol` 把自动名换成语义名；符号集里已有名字的硬件地址不另起名字；新增符号用 `defineSymbol`（`kind` 取 `procedure`、`label`、`variable`、`constant`、`data`，写 `description`）。
4. 命名规则：例程用动词开头的小写下划线名（`read_pad`、`copy_palette`），变量与数据用名词（`player_x`、`palette_table`），常量用大写；不与指令助记符、寄存器名、符号集里的常量重名；同一个名字只表示一个东西。
5. 变量的名字来自用途（谁写、谁读、取值范围），不来自地址；看不出含义时先在 `description` 里写观察到的事实。

## 5 注释

- 例程的块注释：`setComment` 的 `kind: "block"`，写在例程入口地址上，内容是作用、入口参数、返回值、副作用。
- 关键指令的行尾注释：`kind: "inline"`；硬件寄存器的读写写出含义（符号集里已有注释的不重复）。
- 操作数提示 `setHint`：立即数是字符、位掩码、地址低字节或高字节时，用 `form`（`char`、`binary`、`decimal`、`low`、`high`、`symbol`）让生成的源码可读；`low`、`high` 配合 `symbol` 引用被取低高字节的符号。
- 注释用中文，写“为什么”与约定，不重复指令字面意思；没有依据的推断写“未确认：……”。
- 找没有块注释的例程：`disasm_list`，`category: "uncommented-routines"`。

## 6 分层

用 `disasm_callgraph` 与符号名划分功能（启动与中断、输入、显示、声音、游戏逻辑、数据表……），用 `assignModule` 把地址范围归入模块；一个模块生成一个 `.s` 文件。每个例程只属于一个模块，数据表跟着使用它的模块走。

## 7 生成与校验

1. `disasm_generate` 写出 `src/<模块>.s`、`include/`、`link.cfg`。
2. `disasm_verify`：重新汇编链接，与原映像逐字节比较。
3. 不一致时看返回的第一处差异（段、地址、偏移、两边字节、所在符号与区域类型）。多数原因是：区域标注与实际用法不符（把数据标成代码，或相反）、跨块分支、操作数提示改变了字节。按差异所在的区域改正标注，重新生成再校验；同一处差异改两次仍不一致，停下汇报差异原文。
4. 之后的任何编辑都使回读结果过期，收尾前要重新 `disasm_verify`。

## 8 构建

目标支持时（NES 的 NROM、SB2000 的 `.EXE`），把生成的源码整理成 fcdev 项目（段名改成目标链接配置用的名字），用 `disasm_build`（`buildPath` 为该项目目录）构建并与原映像比较；不支持的目标跳过这一步，回读一致即完成。

## 报告

收尾时列出：

- 每个完成标准的数字：`symbols.auto`、`proceduresWithoutComment`、`regions.unknown.bytes`、`unresolved.indirectJumps`、`unresolved.crossBank`、`verification.state`。
- 工程目录、生成的文件、模块清单。
- 仍标着“未确认”的地方与没有处理的未解析项，各写原因。
- 没有验证到的部分，例如没有模拟器或 BIOS 时没做的运行记录。
