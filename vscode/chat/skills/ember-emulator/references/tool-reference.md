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
| `emu_load` | `file` 与 `machine` 二选一；整机可加 `firmware`、`diskImage`、`program`；`familyKeyboard`；`powerOn`（卡带，默认 `true`）；`target` 加 `input`（按目标描述载入，此时不给 `machine`）；`preview: true` 加 `file`（只预览文件头与匹配的插件，不装入） | 运行状态（`runtime.status`）；预览返回 `header`（`mapper`、`variant`、`prgSize`、`chrSize`、`mirroring`、`battery`、`crc32Prg`、`crc32All`）与 `match`（插件与主机型号）或 `matchError`（没有匹配的原因） |
| `emu_run` | 授权量给一个：`frames`（默认 60，1 到 10000000）、`cycles`（至少这么多 CPU 周期）、`instructions`（这么多条指令）、`scanlines`（PPU 扫描线推进这么多行） | `stopped`、`frames`、`cycles`（按 `cycles`、`instructions`、`scanlines` 授权时还有 `instructions`）、`state`、`cpu`（`pc`、`a`、`x`、`y`、`sp`、`status`、`cycles`） |
| `emu_step` | `unit`（`instruction`、`cycle`、`scanline`、`frame`，默认 `instruction`）、`count`（默认 1，最多 100000） | 实际执行数、停下时的 `pc` 与 CPU 状态 |
| `emu_break` | `action`（`add`、`list`、`remove`、`clear`、`enable`、`disable`、`update`，默认 `list`）；`add` 要 `kind`（`execute`、`read`、`write`、`irq`、`nmi`、`brk`、`reset`），前三种还要 `address`，可加 `endAddress`；`add` 与 `update` 可带 `condition`（`update` 给空字符串表示去掉条件）；`remove`、`enable`、`disable`、`update` 要 `id` | 当前全部断点（含 `enabled`、`condition`、`hits`） |

`execute` 断点在执行该地址的指令之前停下；`read`、`write` 断点在访问该范围的那条指令执行完后停下；`irq`、`nmi`、`brk` 断点在 CPU 响应这种中断之后、处理程序的第一条指令执行之前停下；`reset` 断点在上电或复位之后、第一条指令执行之前停下，后四种不给地址。禁用的断点保留在表里，不触发。

`condition` 在地址满足之后再求值，结果为 0 就不停下：数值写 `$C000` 或十进制；变量 `a`、`x`、`y`、`sp`、`pc`、`p`、`cycles`、`frame`、`scanline`、`dot`，读写断点还有 `address`、`value`、`write`；`[地址]` 读一个字节，`{地址}` 读 16 位；运算符同 C。例：`a == $40 && [$10] != 0`。

## 观察

| 工具 | 参数 | 返回 |
| --- | --- | --- |
| `emu_inspect` | `what`：`state`（可加 `module`，如 `cpu`、`ppu`、`mapper`）、`memory`（`address`、`length` 默认 64、最多 4096、`space` 默认 `cpu`）、`disassemble`（`address`、`count` 默认 16）、`resolve`（`space` 为 `cpu` 或 `ppu`，`address`）、`spaces`、`write`（`address`、`bytes`，需确认）、`setState`（`fields`: `[{ name, value }]`，需确认）、`callStack`（调用栈，最里层的在前）、`palette`（当前 64 个调色板颜色，每行 8 个，写作 `编号=#RRGGBB`） | 各项对应的状态、十六进制视图、存储区与偏移 |
| `emu_screen` | `file`（工作区里的 PNG 路径，需确认）、`inline`（默认 `true`） | 图像、尺寸、文件路径；暂停与断电后也有上一帧 |
| `emu_profile` | `action`（`start`、`get`、`stop`，默认 `get`） | `start` 开始统计；`get` 读取当前结果（统计继续）；`stop` 结束并返回结果。结果含 `totalCycles`、`entries`（每项的 `kind`、`address`、`calls`、`inclusiveCycles`、`exclusiveCycles`，按不含子程序的周期数从大到小）与 `text` |
| `emu_events` | `action`（`start`、`get`、`stop`，默认 `get`）；`maxEvents`（`start` 保留的事件数默认 65536，`get` 读的事件数默认 20000）；`fromFrame`（`get` 只读这一帧及以后的事件） | `get` 返回 `text`（每行一个事件：帧、行、点，读写的地址与值、发起的指令，或中断的处理程序）、`count`、`dropped`、`frame`；记录 PPU 与 mapper 寄存器的读写和 IRQ、NMI、BRK 中断 |
| `emu_speed` | `percent`（10 到 1000，相对真实速度的百分比，100 恢复正常）或 `max`（不限速）二选一；`audio`（`real` 按真实时间播放，`fast` 采样丢弃）；至少给一项 | 设置之后的 `speed`、`maxSpeed` 与 `mode`；只影响自由运行，不影响 `emu_run` 与 `emu_step`；速度不是 100% 或不限速时不输出声音；不保存 |

## 输入

`emu_input` 的 `kind`：

| `kind` | 参数 | 说明 |
| --- | --- | --- |
| `key` | `code`（点一个键）或 `text`（输入一段文字），二选一；`action`（`tap` 默认、`down`、`up`，只对 `code` 有效）；`enter`（只对 `text`：输完再按一次回车）；`downFrames`（默认 8）、`upFrames`（`code` 默认 4，`text` 默认 8） | `tap` 会按下、运行 `downFrames` 帧、松开、再运行 `upFrames` 帧；`text` 按美式键盘布局逐字符点键，与机型无关，大写与 `!@#` 等符号自动按 Shift，`\n` 是回车，其余字符报错且一个键都不按；机器要已载入并通电 |
| `pad` | `port`（默认 0）；`buttons`（掩码 0 到 255，或 `a`、`b`、`select`、`start`、`up`、`down`、`left`、`right` 数组）；`hold`（只按下不松开）；`downFrames`、`upFrames` | 掩码：A 为 1、B 为 2、选择为 4、开始为 8、上为 16、下为 32、左为 64、右为 128 |
| `mouse` | `event`（`move`、`down`、`up`、`wheel`、`leave`）；`x`、`y`、`dx`、`dy`、`buttons`（1 左、2 右、4 中）、`wheel` | 只在机器带鼠标时有效 |
| `familyKeyboard` | `connected`（默认 `true`） | 接入或断开 Family BASIC 键盘 |
| `releaseAll` | 无 | 松开全部按键与手柄 |

## 记录

`emu_record`：`action`（`start`、`read`、`stop`）与 `kind` 必填。

| `kind` | 开始时的参数 | 说明 |
| --- | --- | --- |
| `instruction` | 可选 `unique`（每个地址只记第一次）、`ring`（环形：行数满了丢弃最早的行继续记）、`maxLines`（默认 100000）、`file`（文件名，不含目录） | 指令轨迹；结束时给 `id` |
| `cdl` | 无 | 代码数据日志；结束时一并保存文件 |
| `callLog` | `address`（子程序入口）、`stackBytes`（默认 0，至少 2 才能核实调用者）、`ranges`（最多 4 段）、`skip`（最多 4 项） | 每次执行到该地址的指令之前记一行；结束时给 `id` |
| `io`、`irq` | 可选 `ring`、`maxLines`、`file` | 寄存器读写与中断，不能导入反汇编工程 |

`action: "read"` 边记边读（不结束记录）：给 `id`（只有一个时可省略）、可选 `from`（第一行的行号，默认 0）与 `maxLines`（默认 1000），返回 `lines`、`first`、`total`（已产生的行数，下次从它读起就只读新增的行）、`oldest`（环形记录还留着的最早一行）、`truncated`。`cdl` 没有 `read`。

结束时返回记录文件的完整路径 `file`（写在 Ember 的调试输出目录，可以在工作区之外）。各种记录导入后的效果见 skill `disassemble-6502` 的 `references/records.md`。
