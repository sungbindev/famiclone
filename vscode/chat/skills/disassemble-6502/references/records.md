# 模拟器记录

静态追踪看不到的代码（表在 RAM 里的分发、间接调用、运行时才确定的入口）靠运行记录补。工具：`emu_*` 记录，`disasm_import_record` 导入。

## 闭环

1. 会话：`emu_status` 看有没有；没有就 `emu_launch`（启动随包安装的 Ember）或 `emu_connect`（连接已开启调试服务的 Ember）。
2. 载入：`emu_load`。NES 用 `target: "nes"` 加 `file`；SB-DOS 程序用 `target: "sb2000"` 加 `firmware`（学习机的 BIOS）、`diskImage`（放着该 `.EXE` 的软盘镜像）、`program`（程序名）；载入后机器是暂停的。
3. 开始记录：`emu_record`，`action: "start"`，`kind` 取下表之一。
4. 让程序跑到要覆盖的代码：`emu_run`（`frames`）、`emu_input`（按键、手柄、鼠标）、`emu_step`。覆盖越全，记录越有用：进入菜单、触发各种操作。
5. 结束记录：`action: "stop"`，返回记录文件的完整路径 `file`。
6. 导入：`disasm_import_record`，`kind` 与记录一致，`file` 给上一步的路径。
7. 再追踪：`disasm_trace`，看新增的指令数与例程数；回到第 2 步的 `unresolved`，看还剩什么。

收尾：`emu_status` 加 `close: true`；要结束自己启动的 Ember 再加 `terminate: true`。

## 记录种类

| kind | 内容 | 导入后的效果 |
| --- | --- | --- |
| `cdl` | 每个字节的标记：被当作代码执行、被当作数据读取、跳转目标、子程序入口 | 代码段的起点、跳转目标与子程序入口成为入口；只被当数据读取的字节标成 `bytes` 区域 |
| `instruction` | 执行过的指令轨迹（`unique: true` 时每个地址只记第一次，文件小得多） | 执行过的地址成为入口 |
| `callLog` | 每次执行到某条指令（给 `address`，通常是子程序入口）前记一行，`stackBytes` 至少 2 才能核实调用者 | 被记录的地址成为入口；对照映像核实过的 `JSR` 调用者也成为入口 |
| `io`、`irq` | 寄存器读写、中断 | 不导入，供自己阅读（看谁在访问某个硬件寄存器） |

`maxLines` 限制行数（默认 100000）；记录文件由 Ember 写在它的调试输出目录，可以在工作区之外，导入时会提示确认。

## 补充手段

- `emu_break`：在可疑地址设执行断点，`emu_run` 命中后 `emu_inspect` 的 `state` 看寄存器，`memory` 看栈与表，`disassemble` 对照 Ember 自己的反汇编。
- `emu_inspect` 的 `resolve`：把 CPU 地址落实成存储区与偏移，用来确定 bank 布局。
- `emu_screen`：截图，核对程序跑到了哪个画面；较旧的 VS Code 不能返回图像时给 `file` 存成文件。
- 写内存与寄存器（`write`、`setState`）会弹确认，只在需要把程序推进到某个状态时用。
