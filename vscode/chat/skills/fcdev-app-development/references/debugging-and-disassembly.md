# 运行、调试与反汇编

在模拟器（Ember）里运行程序、自动核对结果、读写内存、反汇编已有程序时使用。前提：用户已在设置 `fcdev.emulator.path` 里指向 `Ember.exe`；没有设置时先请用户设置，不要自己猜路径。

## 运行方式

| 目标 | 运行（`fcdev_run`、命令“运行”、任务 `run`）的做法 |
| --- | --- |
| `nes` | Ember 直接加载 `.nes`，不需要固件；普通运行启动后立即返回 |
| `sb2000` | 需要 `fcdev.json` 的 `firmware`（没有时在构建之前报“需要固件”）：经 Ember 的调试服务添加 BIOS、开机、把程序所在的磁盘镜像放进软驱；普通运行时再在 SB-DOS 里跳过日期、时间并输入程序名运行，完成后断开；调试运行时停在开机状态，程序名要自己用 `emu_input`（按键）输入（程序名只能用字母与数字） |

- 调试运行（`fcdev_run` 的 `debug` 为真、命令“调试运行”）：用独立的用户数据目录（系统临时目录下的 `fcdev-ember/<端口>`）、设置 `fcdev.emulator.debugPort`（默认 5699）里的端口与随机令牌启动 Ember，连上调试服务后载入程序，上电但不运行；之后用 `emu_connect` 连接并继续。可以与平时在用的 Ember 同时运行。
- 启动前检查端口占用，被占用就报错；最多等 30 秒等调试服务可连接，Ember 提前退出时立即报错。
- Ember 进程与扩展脱离，窗口一直打开，由用户关闭。只结束自己启动的 Ember（`emu_status` 的 `close` 加 `terminate`，只对 `emu_launch` 启动的有效），不要按进程名结束，以免关掉用户自己打开的 Ember。
- 普通运行（NES）遵守 Ember 的单实例规则：已有 Ember 在运行，新进程只会把已有窗口提到前面，不会载入新程序；先关闭它，或改用调试运行。

## 调试：emu_* 工具

| 要做的事 | 工具 |
| --- | --- |
| 连接已运行的 Ember（默认按设置里的端口） | `emu_connect` |
| 启动一个空的调试 Ember、载入程序 | `emu_launch`、`emu_load`（卡带给 `file`，整机给 `machine`、`firmware`、`diskImage`、`program`，也可以按 `target` 的启动步骤载入） |
| 查看或关闭会话 | `emu_status`（`close`，再加 `terminate`） |
| 运行 N 帧，等运行结束或命中断点 | `emu_run`（`frames`） |
| 单步 | `emu_step`（`unit`：`instruction`、`cycle`、`scanline`、`frame`；`count`） |
| 断点 | `emu_break`（`action`：`add`、`list`、`remove`、`clear`；`kind`：`execute`、`read`、`write`；`address`），命中后 `emu_run` 的 `stopped` 为 `breakpoint` |
| 读寄存器与状态、读写内存、反汇编、解析地址落在哪块存储区 | `emu_inspect`（`what`：`state`、`memory`、`disassemble`、`resolve`、`spaces`、`write`、`setState`） |
| 截图 | `emu_screen`（作为图像返回；`file` 同时保存 PNG） |
| 注入输入 | `emu_input`（`kind`：`key`、`pad`、`mouse`、`familyKeyboard`、`releaseAll`；手柄掩码 1=A、2=B、4=选择、8=开始、16=上、32=下、64=左、128=右，或按键名数组） |
| 记录指令轨迹、CDL、调用记录 | `emu_record`（`start`、`stop`，结束时返回文件路径，交给 `disasm_import_record`） |

写内存、改寄存器、保存截图前会弹出确认。工具之外还有 Ember 的全部调试请求：用户可以用命令“fcdev：调用调试请求”（选请求、写参数 `key=value`，值按 JSON 解析，`0x8000` 与 `$8000` 是十六进制数）、“查看调试请求说明”、“保存调试截图”、“监视调试事件”操作，常用请求：

| 要做的事 | 请求 |
| --- | --- |
| 运行状态与卡带 | `runtime.status` |
| 运行 N 帧并等运行结束 | `runtime.run`（`frames=N wait=true`） |
| 单步 | `runtime.step`（`unit=instruction count=N`） |
| 继续运行、暂停、复位 | `runtime.resume`、`runtime.pause`、`runtime.reset`（`kind=soft` 或 `hard`） |
| 寄存器与芯片状态 | `inspector.getState`、`inspector.snapshot` |
| 读内存 | `inspector.readMemory`（`space=cpu address=0x8000 length=16`）；`inspector.listSpaces` 列出全部空间 |
| 断点 | `inspector.addBreakpoint`（`kind=execute address=0x8000`，还有 `read`、`write`） |
| 注入输入 | `input.key`（`code=KeyW down=true`，`code` 是 `KeyboardEvent.code`，由卡带的 mapper 脚本翻译）、`input.pad`（`port=0 buttons=1`） |
| 指令轨迹 | `inspector.traceStart`（`kind=instruction file=trace.txt`），运行后 `inspector.traceStop`（`id=<编号>`） |
| 代码与数据标记 | `inspector.cdlStart`，运行后 `inspector.cdlSave`（`file=prg.cdl`） |

`file` 参数（如 `inspector.traceStart` 的文件名）由 Ember 写进它的调试输出目录（调试运行时是用户数据目录里的 `debug-output`），结果里的 `file` 给出完整路径。请求默认最多等 60 秒。Ember 的调试服务是 WebSocket 加 JSON-RPC，只监听 `127.0.0.1`，需要令牌，客户端按端点文件自动取得地址与令牌。

## 自动化核对流程

NES：

```text
fcdev_run（path=项目的 src 目录，debug=true）      # Ember 载入程序，停在开机状态
emu_connect                                         # 连接这个 Ember
emu_run（frames=60）                                # 跑 60 帧
emu_screen                                          # 截图，查看图像核对画面
```

最后请用户关闭自己启动的调试 Ember，或用 `emu_status` 关闭会话（`terminate` 只对 `emu_launch` 启动的有效）。

要点：

- 帧数不够会截到启动画面；先截图确认当前画面，再决定下一步。
- 想在某个点停下，对该处的指令地址设执行断点，或对一个全局变量设写断点（`kind=write`），再 `emu_run`，命中后用 `emu_inspect` 读结果；`emu_step` 忽略断点。
- 符号地址从构建产物 `build/obj/<目标>/<程序名>/<程序名>.dbg` 里查（`sym` 行的 `name` 与 `val`，C 符号带前导下划线）；`.map` 只列被其他模块引用的导出。
- 这些步骤要用真实的 Ember，没有就明说“未运行验证”；不要把没跑过的结论当作已验证。

## 反汇编

用命令“fcdev：反汇编映像文件”（选映像、输入工程目录，一次完成读映像、追踪、生成源码工程与回读校验），或让反汇编工程师智能体用 `disasm_*` 工具多轮完成。

- 生成 ca65 源码工程：用扩展自己的生成器输出（不经 `da65`），再用 `ca65`、`ld65` 重新汇编并与原映像逐字节比较，不一致就报错，指出第一个差异的偏移与地址。
- 输出的工程目录里有 `disasm.json`、`src/main.s`、`include/`、`link.cfg`、`verify/`；映像文件不复制到工程里。
- 命令按文件头判断：iNES 或平铺二进制；SB-DOS 程序（`.EXE`）、学习机 BIOS 与带符号集的输入用工具 `disasm_open` 的 `target`（`nes`、`sb2000`）与 `input` 指定。平铺二进制要给加载地址，没有入口时再给入口，命令缺少时会询问；不会自动取头部的 `JMP`。
- 静态追踪追不到 `RTS` 跳转表等间接调用的代码，这部分按数据输出，回读仍然一致；要补全就用运行时记录：`emu_record` 记录指令轨迹、CDL（FCEUX、Mesen 格式也认）或调用记录，用 `disasm_import_record` 导入，再 `disasm_trace` 重新追踪。
- 符号文件每行“名字 = 地址”或“名字 地址”，地址十六进制，可带 `$` 或 `0x`，`;` 或 `#` 开头是注释；名字必须是合法的 ca65 标识符。
- BIOS 与固件的反汇编整理不在 fcdev 范围内，也不要把用户的 BIOS、ROM 提交进仓库。
