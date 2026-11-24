# 金字塔 PEC-9588 反汇编工程

金字塔学习机 PEC-9588 的 NES 卡带映像 `reference/study_roms/金字塔/PEC-9588.NES` 的反汇编工程。

## 状态

- 回读校验：重新汇编链接的结果与原映像逐字节一致（`disasm_verify` 为 `ok`）。
- 符号与注释：尚未完成，自动命名的符号与未注释的例程见 `disasm_status`。

## 使用

以本目录为 `projectDir`，用反汇编工具操作：

- `disasm_status`：查看进度与校验结果。
- `disasm_generate`：由 `disasm.json` 生成 `src/`、`include/` 与 `link.cfg`。
- `disasm_verify`：重新汇编链接并与原映像比较。
- FC Dev 构建：目录里有 `fcdev.json`（`rom` 目标，直接使用 `link.cfg`），用任务“FCDEV：构建”或 `fcdev_build` 构建，产物 `build/rom/pec9588.bin`（524288 字节，不含 iNES 文件头）与 `ember/roms/pec9588.bin` 逐字节一致。

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
