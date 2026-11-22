# 模拟器记录

静态追踪看不到的代码（表在 RAM 里的分发、间接调用、运行时才确定的入口）靠运行记录补。模拟器的启动、载入与运行方法在 skill `ember-emulator`，本文只写反汇编用的闭环、记录种类与导入效果。

## 开始前的判断

| 条件 | 做法 |
| --- | --- |
| 映像是 NES 卡带，或 SB-DOS 程序加上学习机 BIOS 与放着该 `.EXE` 的软盘镜像 | 可以记录，走下面的闭环 |
| 缺 BIOS 或软盘镜像 | 不记录，在报告里写明“未做运行记录：缺少……”，第 2 步用静态追踪的结果 |
| `unresolved` 已为 0 且 `unknown` 里没有疑似代码 | 不需要记录 |

## 闭环

1. 起步与载入：按 skill `ember-emulator` 的“起步”与“载入”。NES 用 `emu_load` 的 `target: "nes"` 加 `file`；SB-DOS 程序用 `target: "sb2000"` 加 `firmware`、`diskImage`（放着该 `.EXE` 的软盘镜像）、`program`（程序名）；载入后机器是暂停的。
2. 开始记录：`emu_record`，`action: "start"`，`kind` 按下表选。
3. 让程序跑到要覆盖的代码：`emu_run`、`emu_input`、`emu_step`。覆盖越全，记录越有用：进入菜单、触发各种操作。每一轮都用 `emu_screen` 或 `emu_inspect` 确认程序跑到了哪里。
4. 结束记录：`action: "stop"`，返回记录文件的完整路径 `file`。
5. 导入：`disasm_import_record`，`file` 给上一步的路径，`kind` 按下表对应。
6. 再追踪：`disasm_trace`，看新增的指令数与例程数；回到 skill `disassemble-6502` 第 2 步看剩余的 `unresolved`。
7. 收尾：按 skill `ember-emulator` 的“收尾”关闭会话。

## 记录种类与导入

| `emu_record` 的 `kind` | `disasm_import_record` 的 `kind` | 内容 | 导入后的效果 |
| --- | --- | --- | --- |
| `cdl` | `cdl` | 每个字节的标记：被当作代码执行、被当作数据读取、跳转目标、子程序入口 | 代码段的起点、跳转目标与子程序入口成为入口；只被当数据读取的字节标成 `bytes` 区域 |
| `instruction` | `trace` | 执行过的指令轨迹（`unique: true` 时每个地址只记第一次，文件小得多） | 执行过的地址成为入口 |
| `callLog` | `callLog` | 每次执行到某条指令（给 `address`，通常是子程序入口）前记一行，`stackBytes` 至少 2 才能核实调用者 | 被记录的地址成为入口；对照映像核实过的 `JSR` 调用者也成为入口 |
| `io`、`irq` | 不导入 | 寄存器读写、中断 | 供自己阅读，看谁在访问某个硬件寄存器 |

- 选哪一种：要找“哪些字节被当作代码执行”用 `cdl`；要找“实际执行过的指令地址”用 `instruction` 并给 `unique: true`；要找“谁调用了这个子程序”用 `callLog`。
- `maxLines` 限制行数（默认 100000）；记录文件由 Ember 写在它的调试输出目录，可以在工作区之外，导入时会弹出确认。

## 补充手段

- `emu_break`：在可疑地址设执行断点，`emu_run` 命中后 `emu_inspect` 的 `state` 看寄存器，`memory` 看栈与表，`disassemble` 对照 Ember 自己的反汇编。
- `emu_inspect` 的 `resolve`：把 CPU 地址落实成存储区与偏移，用来确定 bank 布局。
- `emu_screen`：截图，核对程序跑到了哪个画面。
- 写内存与寄存器（`write`、`setState`）会弹确认，只在需要把程序推进到某个状态时用。
