# 目标说明

## NES 卡带

`target: "nes"` 的 `cartridge` 输入按映射器划分 PRG：

| 映射器 | 布局 |
| --- | --- |
| 0 NROM、3 CNROM | 16K 在 `$C000`，32K 在 `$8000`，没有 bank |
| 1 MMC1、2 UxROM | 最后 16K 固定在 `$C000`，其余每 16K 一个 bank，都放 `$8000`，bank 号从 0 起 |
| 7 AxROM | 每 32K 一个 bank，都在 `$8000` |
| 4 MMC3 | 可换的 8K bank 都放 `$8000`，最后两个 8K 固定在 `$C000`、`$E000` |

初始布局只按默认模式划分，真实程序的窗口配置可能不同，要核实：

1. 按 skill `ember-emulator` 的“起步”，再 `emu_load` 给 `target: "nes"`、`file`，`emu_run` 跑到要研究的位置（或断在要研究的代码上）。
2. 对关键地址 `emu_inspect` 的 `what: "resolve"`（`space: "cpu"`、`address`），返回它落在哪块存储区（如 `cart.prg`）的哪个偏移。
3. 偏移加 16 字节文件头（有 trainer 再加 512）就是映像文件里的偏移；与 `disasm_layout` 里该 bank 的 `fileOffset` 对照。
4. 与初始布局不符，或映射器不在表里（`DISASM_MAPPER_UNSUPPORTED`）时，用 `format: "flat"` 与 `segments` 手工给出每个 bank 的段，`disasm_layout` 给 `segments` 可以替换已有布局。
5. 没有条件核实（没有模拟器可用）时，保留初始布局，在报告里写明“布局未核实”。

向量指向可换页窗口时，工具按“向量是在哪个段读到的”确定 bank；追踪遇到跨 bank 转移会登记到 `unresolved`。

## 学习机 SB2000

### SB-DOS 程序（`input: "program"`）

- 文件头 `4C lo hi AA 55` 加 3 个字节；入口固定是 `$8000`（文件头的 `JMP`），工具已登记。
- 程序不超过 16K：一个段，`$8000` 起。
- 多页程序：文件页 0、1 是常驻部分（`$8000`，8K），页 2 是库页 1（窗口 2，`$A000`，bank 1），页 3 是栈页（`$B000`，不放程序内容），页 4 起是库页 2 起（都换进 `$A000`，bank 号等于库页号）。从常驻代码调用库页函数要经蹦床，跨 bank 转移会登记到 `unresolved`，用运行记录或读蹦床代码确定目标库页。
- 系统调用经 `SYSCALL`，功能号的名字在符号集 `sb2000.inc`；`disasm_find` 找 `JSR` 到系统调用入口的指令，再看调用前对功能号的赋值。

### BIOS（`input: "bios"`）

- BIOS 是 512K，每页 4K 共 128 页，页号就是 `$4040-$4047` 窗口寄存器里的值；开机后 `$C000-$FFFF` 固定为第 4 至 7 页，`$8000-$BFFF` 由程序换页。
- 初始布局：第 4 至 7 页固定在 `$C000`；其余页按“页号对 4 取余”放进 `$8000` 窗口，bank 号是每组第一页的页号。这只是猜测：每组代码真正放在窗口的哪个位置要核实。
- 核实：按 skill `ember-emulator` 的“起步”，再 `emu_load` 给 `target: "sb2000"`、`input: "bios"`、`firmware`（要研究的 BIOS 文件），`emu_run` 让它开机，`emu_inspect` 的 `what: "memory"` 读 `$4040-$4047` 得到各窗口当前的页号，`resolve` 查地址落在哪页哪个偏移；没有条件核实时保留初始布局，在报告里写明“布局未核实”。
- 向量取自 `$FFFA` 起的六个字节；复位向量是追踪的起点，其余入口来自系统调用分发表（地址表，用 `disasm_detect` 与 `disasm_xrefs` 找）。
- 整份 BIOS 很大，按页组分批做：先做固定页（启动、中断、系统调用分发），再逐个页组。

## 其他机器

没有内置目标时：`disasm_open` 用 `format: "flat"` 与 `address` 或 `segments`，自己准备符号集（`NAME = $值` 的 `.inc` 文件）用 `symbolSets` 或 `disasm_symbols` 导入；需要运行时用 `emu_load` 的 `machine` 与 `firmware`、`diskImage`。目标描述的结构见 `profile.json` 的内置样例（`nes`、`sb2000`）。
