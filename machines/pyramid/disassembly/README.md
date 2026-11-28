# 金字塔 PEC-9588 反汇编工程

金字塔学习机 PEC-9588 的 NES 卡带映像 `PEC-9588.NES` 的反汇编工程。

## 状态

- 回读校验：重新汇编链接的结果与原映像逐字节一致（`disasm_verify` 为 `ok`）。
- 符号与注释：尚未完成，自动命名的符号与未注释的例程见 `disasm_status`；磁盘操作系统（bank 0）已命名主要入口与读写盘程序。

## 磁盘操作系统（LM-DOS 3.31）

5.25 英寸软驱用的磁盘操作系统固化在 bank 0（`$8000-$BFFF`，模块 `bank00_system`），不从磁盘装入。开机画面按任意键进入菜单，按 `6` 进入磁盘操作系统，提示符为 `]`。

- 输入输出钩子：`$36/$37` 指向 `dos_cout_hook`（`$9E60`），`$38/$39` 指向 `dos_keyin_hook`（`$9E2F`）；固定 bank 里苹果监控程序风格的入口 `$FDF0`、`$FD1B`、`$FD6A` 跳到 bank 0 的 `$80FE`、`$8131`、`$819E`。
- 命令：名字表 `dos_command_names`（`$A77A`，共 37 个，苹果 DOS 3.3 的命令加 `LM` 开头的自有命令），参数规则表 `dos_command_flags`（`$A843`，每命令 2 字节）。`INIT` 不带文件名。
- 读写盘程序 `rwts_entry`（`$BC16`）：寻道、找地址场（`rwts_find_address_field`，`$BA59`）、读写扇区；格式化用 `rwts_format_track`（`$BE1D`），逻辑扇区换物理扇区的交错表在 `$BEB8`。
- 盘片格式：143360 字节，40 磁道、每道 14 个 256 字节扇区；地址场 `D5 AA 96`、数据场 `D5 AA AD`，数据为 6 加 2 编码，卷号 254；目录结构仿苹果 DOS 3.3（磁道 17 扇区 0 为卷目录表）。与 SB-DOS、苹果与中华学习机的盘片不通用。
- 控制器寄存器在 `$5610-$561F`（磁盘 II 型：相位、马达、驱动器选择、读写切换）。
- 全部命令与系统接口见 [LM-DOS 命令](../docs/LM-DOS命令.md) 与 [LM-DOS 系统接口](../docs/LM-DOS系统接口.md)。

## 使用

以本目录为 `projectDir`，用反汇编工具操作：

- `disasm_status`：查看进度与校验结果。
- `disasm_generate`：由 `disasm.json` 生成 `src/`、`include/` 与 `link.cfg`。
- `disasm_verify`：重新汇编链接并与原映像比较。
- FC Dev 构建：目录里有 `fcdev.json`（`rom` 目标，直接使用 `link.cfg`），用任务“FCDEV：构建”或 `fcdev_build` 构建，产物 `build/rom/pec9588.bin`（524288 字节，不含 iNES 文件头）与 `ember/roms/pec9588.rom` 逐字节一致。

## 目录

| 路径 | 内容 |
| --- | --- |
| `disasm.json` | 工程文件：映像路径、段布局（34 段） |
| `fcdev.json` | FC Dev 项目配置：`rom` 目标、机型 `pec9588`、整块大小 |
| `src/` | 生成的 ca65 源码（每个模块一个文件） |
| `include/` | 生成的常量与全局符号 |
| `link.cfg` | 生成的链接配置，各段按映像偏移首尾相接输出到同一个文件 |
| `verify/` | 回读时生成的中间文件，已被 `.gitignore` 忽略 |
| `build/` | FC Dev 构建的输出，已被 `.gitignore` 忽略 |
