---
name: disassemble-6502
description: 把 6502 程序（NES 卡带、学习机 BIOS、SB-DOS 的 .EXE、任意平铺二进制）反汇编成可重新汇编、逐字节一致的 ca65 工程时使用：给出八个步骤与每步的完成标准，以及 disasm_* 与 emu_* 工具的用法、区域类型与参数、命名与注释的依据、用模拟器记录补全静态追踪、回读校验。
---

# 反汇编 6502 程序

目标：输入一个 6502 映像，输出一个 ca65 工程，满足四点：重新汇编链接后与原映像逐字节一致；符号有含义明确的名字；代码有注释；按功能拆成多个源文件。

对象是工程（`disasm.json` 加映像），源码由工程与映像生成，每次结果确定。所有修改都经 `disasm_*` 工具，不要手改生成的文件。

## 步骤与完成标准

| 步骤 | 做什么 | 完成标准 |
| --- | --- | --- |
| 1 识别 | 确定输入种类与布局，新建工程，登记符号集 | 布局覆盖映像里属于 CPU 地址空间的每个字节，向量与入口已登记 |
| 2 追踪 | 静态追踪；有模拟器时运行并导入记录，再追踪 | 未解析的间接跳转与跨 bank 转移都处理了，或写明无法确定的原因 |
| 3 区域分类 | 识别并确认地址表、字符串、图形数据、填充 | 没有 `unknown` 区域，或每处都写明原因 |
| 4 命名 | 从被引用最多的例程与变量开始，换成语义名 | 自动命名的符号数为 0 |
| 5 注释 | 给例程写说明，给关键指令与硬件访问写注释 | 每个例程有块注释 |
| 6 分层 | 按功能划分模块 | 每个例程属于一个模块 |
| 7 生成与校验 | 生成源码并回读 | 回读一致；之后每次编辑都要重新校验 |
| 8 构建 | 目标支持时用 `disasm_build` | 产物与原映像一致 |

每做完一批编辑就调用 `disasm_status`，对照上表看哪一步还没达标。完成标准以 `disasm_status` 的数字为准：`symbols.auto`、`proceduresWithoutComment`、`regions.unknown`、`unresolved`、`verification.state`。

## 1 识别

先看文件头与大小，再选目标：

| 输入 | 打开方式 |
| --- | --- |
| NES 卡带（`.nes`，以 `NES` 加 `$1A` 开头） | `disasm_open` 给 `image`、`target: "nes"` |
| 需要 Family BASIC 键盘的卡带 | 同上，`input: "family-basic"`（反汇编不受影响，运行时会先接入键盘） |
| SB-DOS 程序（`.EXE`，头 `4C ?? ?? AA 55`） | `target: "sb2000"`（默认输入 `program`） |
| 学习机 BIOS 文件 | `target: "sb2000"`，`input: "bios"` |
| 其他 6502 映像 | `format: "flat"`，给 `address`（加载地址）或 `segments`（换页时每段的 `id`、`fileOffset`、`length`、`address`、`bank`），自己用 `entries` 给入口，符号集用 `symbolSets` |

`target` 会自动选映像格式、登记符号集（硬件寄存器的名字）并记下图形数据端口；自带的目标描述给 `profile.json` 的路径。打开后检查：

1. `disasm_layout`：`uncovered` 里没有应该属于程序的字节；每段的地址、bank 合理。
2. `disasm_list` 的 `entries`：向量或文件头的入口都在。
3. 布局只是初始猜测的情形（分页 ROM、MMC3 的 `$A000` 窗口、不在内置表里的映射器）要用模拟器核实，见 [目标说明](references/targets.md)；确认后 `disasm_layout` 给 `segments` 替换（会丢手工标注的区域，所以越早越好）。

已经打开过的工程只给 `projectDir` 就是继续做；先看 `disasm_status`。

## 2 追踪

1. `disasm_trace`：从入口沿控制流标出指令，识别经地址表分发的跳转，并给新发现的调用目标、跳转目标、数据与变量自动命名（`sub_`、`loc_`、`data_`、`var_` 加地址）。可以在 `entries` 里同时加入新入口。
2. `disasm_list` 的 `unresolved`：间接 `JMP` 与跨 bank 转移追不出去。逐个处理：
   - `disasm_routine` 看它所在的例程，推断目标表在哪；表在 RAM 里要靠运行记录。
   - 能确定目标地址时，`disasm_apply` 的 `addEntry`（或下一次 `disasm_trace` 的 `entries`）加入口。
   - 不能确定时在注释里写“未确认：间接跳转的目标取决于……”。
3. 有模拟器时补静态追踪看不到的代码：按 [模拟器记录](references/records.md) 的闭环记录 CDL、指令轨迹或调用记录，`disasm_import_record` 导入后再 `disasm_trace`。

静态追踪追不到的字节会按数据输出，不影响回读，但会留在 `unknown` 里，要在第 3 步处理。

`unknown` 里较短的范围（几个到几十字节）多半是没有被调用到的库代码：用 `disasm_hex` 看内容像指令，就把范围起点加为入口再追踪；较长的范围按数据处理。

## 3 区域分类

1. `disasm_list` 的 `unknown` 列出没有识别的字节范围。
2. `disasm_detect` 给候选与依据（填充、地址表、字符串、图形数据），不修改工程。图形数据需要图形数据端口，用 `target` 打开的工程已经带上。
3. 用 `disasm_hex`、`disasm_find`、`disasm_xrefs` 核实：谁读了这段数据、怎么用、步长多少。确认后用 `disasm_apply` 的 `setRegion` 标注：

| type | 用于 | params |
| --- | --- | --- |
| `code` | 指令（追踪通常已标好） | 无 |
| `bytes` | 一般数据 | `perLine`（每行字节数，1 到 32） |
| `words` | 16 位地址表，长度必须是偶数 | `perLine`、`adjust`（0 或 1，RTS 表为 1） |
| `string` | 字符串 | `encoding`、`terminator`（`null`、`0` 或 `"high-bit"`） |
| `graphics` | 图形数据（如 CHR 图块） | `format` |
| `padding` | 填充 | `value` |

`disasm_detect` 的图形数据候选，长度只是上限（取到下一个已标注的字节）。要看循环次数、目标显存地址（例如 `$3F00` 是调色板，32 字节）才能确定实际长度，再把它后面的字节分开标注。数据区里夹着带 `$FFFF` 占位的字节，多半是复制到 RAM 再改写的代码模板，标成 `bytes` 并注明“未确认”，不要标成 `code`。

分不清的数据留作 `bytes` 并写注释，不要标成没有依据的类型；整张表被拆成多项时按“表项边界”切分，不要把一张字表切在奇数偏移上。

## 4 命名

1. 从被引用最多的例程与变量开始：`disasm_list` 的 `auto-symbols` 加 `disasm_xrefs` 看引用数，`disasm_callgraph` 看层次。
2. 命名前先看 `disasm_routine`：调用者、被调用者、读写了哪些地址、入口出口寄存器的推测（线性推测，不跟踪分支，只作参考）。
3. 用 `renameSymbol` 把自动名换成语义名；符号集里已有名字的硬件地址不要另起名字。新增符号用 `defineSymbol`（`kind` 取 `procedure`、`label`、`variable`、`constant`、`data`，写 `description`）。
4. 变量的名字来自它的用途（谁写、谁读、取值范围），不是来自地址；名字看不出含义时先在 `description` 里写观察到的事实。

## 5 注释

- 例程的块注释：`setComment` 的 `kind: "block"`，写在例程入口地址上，内容是作用、入口参数、返回值、副作用。
- 关键指令的行尾注释：`kind: "inline"`；硬件寄存器的读写写出含义（符号集里已有注释的不重复）。
- 操作数提示 `setHint`：立即数是字符、位掩码、地址低字节或高字节时，用 `form`（`char`、`binary`、`decimal`、`low`、`high`、`symbol`）让生成的源码可读；`low`、`high` 配合 `symbol` 引用被取低高字节的符号。
- 没有依据的推断写“未确认：……”。

## 6 分层

用 `disasm_callgraph` 与符号名划分功能（启动与中断、输入、显示、声音、游戏逻辑、数据表……），用 `assignModule` 把地址范围归入模块；一个模块生成一个 `.s` 文件。每个例程只属于一个模块，数据表跟着使用它的模块走。

## 7 生成与校验

1. `disasm_generate` 写出 `src/<模块>.s`、`include/`、`link.cfg`。
2. `disasm_verify`：重新汇编链接，与原映像逐字节比较。不一致时看返回的第一处差异（段、地址、偏移、两边字节、所在符号与区域类型），多数原因是区域标注与实际用法不符（把数据标成代码，或相反）、跨块分支、操作数提示改变了字节；修正后重新生成再校验。
3. 之后做的任何编辑都使回读结果过期，收尾前要重新 `disasm_verify`。

## 8 构建

目标支持时（NES 的 NROM、SB2000 的 `.EXE`），把生成的源码整理成 fcdev 项目（段名改成目标链接配置用的名字），用 `disasm_build` 构建并与原映像比较；不支持的目标跳过这一步，回读一致即完成。

## 报告

收尾时列出：每个完成标准的数字、仍未确认的地方（写在注释里的“未确认”）、没有处理的 `unresolved`、工程目录与生成的文件。只汇报做过的事，没有验证的写明原因。
