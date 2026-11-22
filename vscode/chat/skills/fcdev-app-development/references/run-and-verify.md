# 运行与核对 fcdev 项目的程序

运行 fcdev 项目的 NES 或 SB2000 程序，并用画面、内存与寄存器核对结果时使用。模拟器工具的用法与报错处理在 skill `ember-emulator`，本文只写 fcdev 项目特有的做法。

## 运行方式

| 目标 | `fcdev_run` 的做法 |
| --- | --- |
| `nes` | Ember 直接加载 `.nes`，不需要固件；不给 `debug` 时启动后立即返回，窗口留给用户 |
| `sb2000` | 需要 `fcdev.json` 的 `firmware`（没有时在构建之前报“需要固件”）：添加 BIOS、开机、把程序所在的磁盘镜像放进软驱 0；不给 `debug` 时再在 SB-DOS 里跳过日期、时间并输入程序名，随后断开连接；给 `debug` 时停在开机状态 |

- 给 `debug` 的 `fcdev_run` 与 `emu_launch` 用同一个调试端口。要用 `emu_*` 工具核对时，不用 `fcdev_run` 的 `debug`，改用 `fcdev_build` 加 `emu_launch` 加 `emu_load`，只有一条路径，不会端口冲突。
- 普通运行（NES）遵守 Ember 的单实例规则：用户平时用的 Ember 已在运行时，新进程只会把已有窗口提到前面，不会载入新程序；这时汇报给用户，由用户关闭它或改用 `emu_*` 核对。
- Ember 进程与扩展脱离，窗口一直打开，由用户关闭；工具只能结束自己用 `emu_launch` 启动的 Ember。

## 核对清单：NES

1. `fcdev_build`，产物是 `<out>/nes/<程序名>.nes`（`out` 默认 `build`，程序名取 `fcdev.json` 的 `name` 或目录名）。
2. 按 skill `ember-emulator` 的“起步”，再 `emu_load` 的 `file` 给产物的绝对路径。
3. `emu_run` 60 帧，`emu_screen` 一张。
4. 与预期对照：模板程序应显示 `HELLO, NES!`。画面不对时，先用 `emu_inspect` 的 `what: "state"` 看 `pc`，再决定是否设断点。
5. 需要输入时用 `emu_input` 的 `kind: "pad"`。

## 核对清单：SB2000

1. 读 `fcdev.json`，取 `name`（程序名，不含扩展名）与 `firmware`（相对 `fcdev.json` 所在目录）。没有 `firmware` 就停下，汇报“未运行验证：缺少 BIOS”。
2. `fcdev_build`，`disk` 为 `true`；磁盘镜像路径取自输出。
3. 按 skill `ember-emulator` 的“起步”，再 `emu_load`：`machine: "sb2000"`、`firmware`、`diskImage`、`program`（程序名）。
4. `emu_run` 60 帧，`emu_screen` 一张，与预期对照。
5. 需要输入时用 `emu_input` 的 `kind: "key"`（SB-DOS 里输入字符时 `downFrames` 与 `upFrames` 都给 8）或 `kind: "pad"`。
6. 程序返回 DOS 会清屏：要看输出的程序在返回前要等键；核对时也可以在返回前的指令地址设执行断点，命中后读屏幕或内存。

## 定位符号地址

构建产物的符号表在 `<out>/obj/<目标>/<程序名>/<程序名>.dbg`：找 `sym` 行，`name` 是符号名（C 符号带前导下划线），`val` 是地址。`.map` 只列被其他模块引用的导出，不用它查地址。得到地址后用 `emu_break` 设断点，用 `emu_inspect` 读内存。

## 已有程序的反汇编

要读懂已有的 `.nes`、`.EXE` 或 BIOS：用 skill `disassemble-6502` 与智能体“6502 工程师”，或用户的命令“fcdev：反汇编映像文件”。BIOS、ROM 与有版权的映像只在本机分析，不提交进仓库。
