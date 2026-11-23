---
name: ember-emulator
description: '用 emu_* 工具操作 Ember 模拟器时使用：启动或连接模拟器，载入 NES 文件或整机（SB2000、SB97、裕兴、步步高等）与软盘镜像，运行、单步、断点、读写内存、截图、按键与手柄输入、记录运行数据；给出任务判定、固定的起步顺序、各类载入的参数、观察与核对循环、收尾步骤，以及 emu_* 报错的处理表。'
user-invocable: false
---

# Ember 模拟器操作

目标：用 `emu_*` 工具把程序在 Ember 里跑起来，用画面、内存与寄存器得到可核对的结论。工具的参数与返回值见 [tool-reference.md](references/tool-reference.md)。

## 必须先读取的参考文件

| 情形 | 必须先读取 |
| --- | --- |
| 载入的是整机（学习机）、要在 SB-DOS 或别的机型里操作 | [machines.md](references/machines.md) |
| 任一工具报错 | [errors.md](references/errors.md) |
| 需要某个工具的全部参数 | [tool-reference.md](references/tool-reference.md) |

## 任务判定

先在下表选一行，再按“做法”走；表里没有的任务先问用户。

| 要做的事 | 做法 |
| --- | --- |
| 运行 fcdev 项目的程序，只需要打开窗口看看 | `fcdev_run`，不给 `debug`；Ember 窗口留给用户，之后工具不能再控制它 |
| 运行并核对 fcdev 项目的 NES 程序 | `fcdev_build`，再走“起步”与“载入”的 NES 一行，然后“观察与核对” |
| 运行并核对 fcdev 项目的 SB2000、SB97、裕兴或步步高程序 | `fcdev_build`（磁盘镜像随构建生成），再走“起步”与“载入”的对应一行，然后“观察与核对” |
| 载入不属于 fcdev 项目的 NES 文件 | “起步”，再 `emu_load` 的 `file` |
| 载入整机加软盘镜像（SB2000、SB97、步步高、金字塔、裕兴） | “起步”，再 `emu_load` 的 `machine`，开机后的操作读 [machines.md](references/machines.md) |
| 为反汇编记录运行数据 | 读 skill `disassemble-6502` 的 `references/records.md`，它先走本 skill 的“起步”与“载入” |

## 起步

固定顺序，不增减：

1. 调用 `emu_status`（不带参数），按返回处理：

| 返回 | 下一步 |
| --- | --- |
| `connected` 为 `false` | 调用 `emu_launch`（不带参数），成功后进入“载入” |
| `connected` 为 `true`、`launched` 为 `true` | 这是本工具启动的 Ember，直接进入“载入”，`emu_load` 会替换已载入的内容 |
| `connected` 为 `true`、`launched` 为 `false` | 这是用户自己打开的 Ember，直接使用，收尾时不结束它 |

2. `emu_launch` 报“端口已被占用”：调用 `emu_connect`（不带参数）。连上后按上表处理；连不上就停下，向用户汇报并请用户关闭占用端口的程序，或指定另一个端口。
3. `emu_launch` 或 `emu_connect` 的其他报错：按 [errors.md](references/errors.md) 处理。

## 载入

调用 `emu_load`，参数按下表。返回的 `runtime.mode` 应为 `paused`、`runtime.power` 应为 `on`；不是时调用 `emu_status` 看原因，不要继续运行。

| 要载入的内容 | 参数 | 载入后的状态 |
| --- | --- | --- |
| NES 文件 | `file`：`.nes` 的绝对路径 | 已通电，暂停在开机状态 |
| 需要 Family BASIC 键盘的 NES 文件 | `file` 加 `familyKeyboard: true` | 同上 |
| SB2000 程序 | `machine: "sb2000"`、`firmware`（BIOS 文件）、`diskImage`（软盘镜像）、`program`（程序名，只含字母与数字） | 已自动开机、跳过日期与时间、输入程序名并回车，程序刚启动，暂停 |
| SB97 程序 | `machine: "sb97"`、`firmware`（ROM 文件）、`diskImage`、`program` | 已自动开机、进入 DOS、输入程序名并回车，程序刚启动，暂停 |
| 裕兴程序 | `machine: "yuxing"`、`firmware`（ROM 文件）、`diskImage`、`program` | 已自动开机、经主菜单与“学DOS”进入 DOS、输入程序名加 `.EXE` 并回车，程序刚启动，暂停 |
| 步步高程序 | `machine: "bbg1"`、`firmware`（ROM 文件）、`diskImage`（带 DOS 的系统盘镜像）、`program` | 已自动开机、等系统盘启动、跳过日期与时间、输入程序名（不加扩展名）并回车，程序刚启动，暂停 |
| 其他整机，或只开机 | `machine`、`firmware`，可加 `diskImage`；不给 `program` | 已通电，暂停在开机状态，之后的操作读 [machines.md](references/machines.md) |

- 机型标识只有这些：`fc`、`sb97`、`sb2000`、`bbg1`、`bbg98`、`pec9588`、`pec586`、`yuxing`。
- 整机必须给 `firmware`。路径取自用户的话、`fcdev.json` 的 `firmware`（相对 `fcdev.json` 所在目录）或项目 README；都没有时问用户，不搜索磁盘。
- SB2000、SB97、裕兴、步步高的磁盘镜像由 `fcdev_build` 生成（步步高要先在 `fcdev.json` 里用 `diskImage` 指定系统盘镜像），镜像路径取自它的输出；不猜扩展名。
- `file`、`firmware`、`diskImage` 在工作区之外时会弹确认，直接调用即可。

## 观察与核对

每一轮固定为“动作、观察、判断”三步，不连续做多个动作再统一看结果：

1. 动作：`emu_run`（运行 N 帧）、`emu_input`（输入）或 `emu_step`（单步），一次一个。
2. 观察：能用数字判断时用 `emu_inspect`（`what: "memory"` 或 `"state"`），文字结果占用的上下文少；需要看画面时用 `emu_screen`，每个观察点只截一张。
3. 判断：把观察结果与预期逐项对照，写出“符合”或“不符合”，不符合时先查原因再做下一个动作。

规则：

- 运行帧数不凭印象：NES 程序第一轮 60 帧；SB2000 程序已由 `program` 启动，第一轮 60 帧。没到预期画面时，下一轮帧数是已运行总帧数的一倍，最多累计到 1920 帧；仍没到就停下，汇报当前截图与寄存器。
- `emu_run` 返回的 `stopped` 按下表处理：

| `stopped` | 含义与下一步 |
| --- | --- |
| `completed` | 帧数跑完，进入观察 |
| `breakpoint` | 命中断点，用 `emu_inspect` 的 `state` 读寄存器、`memory` 读内存 |
| `powerOff` | 机器断电，用 `emu_status` 看状态，汇报程序关机 |
| `interrupted`、`error` | 汇报返回的 `state` 与 CPU 状态，不重试 |

- 输入的帧数：`emu_input` 的按键与手柄默认按下 8 帧、松开 4 帧；在 SB-DOS 命令行里输入字符时 `downFrames` 与 `upFrames` 都给 8，更短会丢键。
- 手柄按键掩码：A 为 1、B 为 2、选择为 4、开始为 8、上为 16、下为 32、左为 64、右为 128，也可以写按键名数组。
- 键盘按键的 `code` 是 `KeyboardEvent.code`，如 `Enter`、`Space`、`ArrowLeft`、`KeyA`、`Digit1`。

## 断点与单步

1. 要在某处停下：`emu_break` 的 `action: "add"`、`kind: "execute"`、`address`；要在变量被写时停下：`kind: "write"`。
2. `emu_run` 命中后 `stopped` 为 `breakpoint`；读结果后用 `emu_break` 的 `action: "remove"` 与 `id` 删除，或 `action: "clear"`。
3. `emu_step` 忽略断点，用来看几条指令的效果；`unit` 取 `instruction`、`cycle`、`scanline`、`frame`。
4. fcdev 项目的符号地址在 `build/obj/<目标>/<程序名>/<程序名>.dbg` 里：找 `sym` 行，`name` 是符号名（C 符号带前导下划线），`val` 是地址。

## 读写内存与寄存器

- 读：`emu_inspect` 的 `what: "memory"`，给 `address`、`length`（最多 4096）；`space` 默认 `cpu`，`what: "spaces"` 列出全部内存空间。
- 地址落在哪块存储区：`what: "resolve"`，`space` 取 `cpu` 或 `ppu`。
- 写内存与改寄存器（`write`、`setState`）只在“需要把程序推进到某个状态”时用，调用前在回复里写明地址与原因。

## 记录运行数据

1. `emu_record`，`action: "start"`、`kind`（`instruction`、`cdl`、`callLog`、`io`、`irq`）；`callLog` 还要 `address`，`stackBytes` 至少 2。
2. 用 `emu_run`、`emu_input` 让程序跑到要覆盖的代码。
3. `emu_record`，`action: "stop"`；`instruction` 与 `callLog` 要给 `id`（开始时返回），只有一个记录时可省略。返回的 `file` 是记录文件的完整路径。
4. 导入反汇编工程：`disasm_import_record`，`kind` 的对应关系为 `instruction` 对 `trace`、`callLog` 对 `callLog`、`cdl` 对 `cdl`；`io` 与 `irq` 不导入，只读文件内容。

## 收尾

任务结束时（用户要求保留窗口除外）：

1. `launched` 为 `true` 的会话：调用 `emu_status`，`close: true`、`terminate: true`。
2. `launched` 为 `false` 的会话：只调用 `emu_status` 的 `close: true`，不结束进程。
3. 回复里写明会话已关闭，还是留给用户。

## 完成检查

- 起步按固定顺序做，没有搜索路径或读设置。
- 每一轮都有“动作、观察、判断”，结论来自返回值、截图或内存读数。
- 没有跑到的部分写了“未运行验证”和原因。
- 自己启动的 Ember 已结束，用户打开的没有动。
