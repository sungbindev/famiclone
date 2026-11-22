# emu_* 工具参考

需要某个工具的全部参数与返回值时使用。流程与判断规则在 [SKILL.md](../SKILL.md)，这里只列参数。地址参数可以写整数，也可以写十六进制文字，如 `"$C000"`。

## 会话

| 工具 | 参数 | 返回 |
| --- | --- | --- |
| `emu_status` | 无参数：查看。`close`（布尔）关闭会话；`terminate`（布尔）关闭时同时结束自己启动的 Ember | 没有会话：`connected: false`。有会话：`connected`、`port`、`launched`（是否由 `emu_launch` 启动）、`pid`、`ember`（版本与协议）、`runtime`（`power`、`mode`、`frame`、`cycles`）、`records`（进行中的记录）、`cdl` |
| `emu_launch` | 可选 `port`（默认取设置 `fcdev.emulator.debugPort`，默认 5699） | `launched`、`executable`，以及 `emu_status` 的内容 |
| `emu_connect` | 可选 `port`；或 `url` 加 `token`；或 `userDataDirectory` | `connected: true`，以及 `emu_status` 的内容 |

## 载入与运行

| 工具 | 参数 | 返回 |
| --- | --- | --- |
| `emu_load` | `file` 与 `machine` 二选一；整机可加 `firmware`、`diskImage`、`program`；`familyKeyboard`；`powerOn`（卡带，默认 `true`）；`target` 加 `input`（按目标描述载入，此时不给 `machine`） | 运行状态（`runtime.status`） |
| `emu_run` | `frames`（默认 60，1 到 10000000） | `stopped`、`frames`、`cycles`、`state`、`cpu`（`pc`、`a`、`x`、`y`、`sp`、`status`、`cycles`） |
| `emu_step` | `unit`（`instruction`、`cycle`、`scanline`、`frame`，默认 `instruction`）、`count`（默认 1，最多 100000） | 实际执行数、停下时的 `pc` 与 CPU 状态 |
| `emu_break` | `action`（`add`、`list`、`remove`、`clear`，默认 `list`）；`add` 要 `kind`（`execute`、`read`、`write`）、`address`，可加 `endAddress`；`remove` 要 `id` | 当前全部断点 |

`execute` 断点在执行该地址的指令之前停下；`read`、`write` 断点在访问该范围的那条指令执行完后停下。

## 观察

| 工具 | 参数 | 返回 |
| --- | --- | --- |
| `emu_inspect` | `what`：`state`（可加 `module`，如 `cpu`、`ppu`、`mapper`）、`memory`（`address`、`length` 默认 64、最多 4096、`space` 默认 `cpu`）、`disassemble`（`address`、`count` 默认 16）、`resolve`（`space` 为 `cpu` 或 `ppu`，`address`）、`spaces`、`write`（`address`、`bytes`，需确认）、`setState`（`fields`: `[{ name, value }]`，需确认） | 各项对应的状态、十六进制视图、存储区与偏移 |
| `emu_screen` | `file`（工作区里的 PNG 路径，需确认）、`inline`（默认 `true`） | 图像、尺寸、文件路径；暂停与断电后也有上一帧 |

## 输入

`emu_input` 的 `kind`：

| `kind` | 参数 | 说明 |
| --- | --- | --- |
| `key` | `code`；`action`（`tap` 默认、`down`、`up`）；`downFrames`（默认 8）、`upFrames`（默认 4） | `tap` 会按下、运行 `downFrames` 帧、松开、再运行 `upFrames` 帧；机器要已载入并通电 |
| `pad` | `port`（默认 0）；`buttons`（掩码 0 到 255，或 `a`、`b`、`select`、`start`、`up`、`down`、`left`、`right` 数组）；`hold`（只按下不松开）；`downFrames`、`upFrames` | 掩码：A 为 1、B 为 2、选择为 4、开始为 8、上为 16、下为 32、左为 64、右为 128 |
| `mouse` | `event`（`move`、`down`、`up`、`wheel`、`leave`）；`x`、`y`、`dx`、`dy`、`buttons`（1 左、2 右、4 中）、`wheel` | 只在机器带鼠标时有效 |
| `familyKeyboard` | `connected`（默认 `true`） | 接入或断开 Family BASIC 键盘 |
| `releaseAll` | 无 | 松开全部按键与手柄 |

## 记录

`emu_record`：`action`（`start`、`stop`）与 `kind` 必填。

| `kind` | 开始时的参数 | 说明 |
| --- | --- | --- |
| `instruction` | 可选 `unique`（每个地址只记第一次）、`maxLines`（默认 100000）、`file`（文件名，不含目录） | 指令轨迹；结束时给 `id` |
| `cdl` | 无 | 代码数据日志；结束时一并保存文件 |
| `callLog` | `address`（子程序入口）、`stackBytes`（默认 0，至少 2 才能核实调用者）、`ranges`（最多 4 段）、`skip`（最多 4 项） | 每次执行到该地址的指令之前记一行；结束时给 `id` |
| `io`、`irq` | 可选 `maxLines`、`file` | 寄存器读写与中断，不能导入反汇编工程 |

结束时返回记录文件的完整路径 `file`（写在 Ember 的调试输出目录，可以在工作区之外）。各种记录导入后的效果见 skill `disassemble-6502` 的 `references/records.md`。
