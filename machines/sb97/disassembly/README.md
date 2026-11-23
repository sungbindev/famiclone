# SB97 BIOS 反汇编工程

小霸王 97 学习机（SB97）的 BIOS 映像 `ember/roms/sb97.bin`（1MB，64 页 16K，mapper 168）的反汇编工程。

## 状态

- 回读校验：重新汇编链接的结果与原映像逐字节一致（`disasm_verify` 为 `ok`）。
- 页布局：第 0 页映射到 `$C000`（固定页），第 1 到 63 页分别映射到 `$8000`；第 0 页在 `$8000` 的映射没有单独列出，因为同一段文件内容不能重复映射。
- 符号与注释：尚未完成，自动命名的符号与未注释的例程见 `disasm_status`。

## 使用

以本目录为 `projectDir`，用反汇编工具操作：

- `disasm_status`：查看进度与校验结果。
- `disasm_generate`：由 `disasm.json` 生成 `src/`、`include/` 与 `link.cfg`。
- `disasm_verify`：重新汇编链接并与原映像比较。
- fcdev 构建：目录里有 `fcdev.json`（`rom` 目标，直接使用 `link.cfg`），用任务“fcdev：构建”或 `fcdev_build` 构建，产物 `build/rom/sb97.bin`（1048576 字节）与 `ember/roms/sb97.bin` 逐字节一致。

## 目录

| 路径 | 内容 |
| --- | --- |
| `disasm.json` | 工程文件：映像路径、页布局 |
| `fcdev.json` | fcdev 项目配置：`rom` 目标、机型 `sb97`、整块大小 |
| `src/` | 生成的 ca65 源码 |
| `include/` | 生成的常量与全局符号 |
| `link.cfg` | 生成的链接配置，各页首尾相接输出到同一个文件 |
| `verify/` | 回读时生成的中间文件，已被 `.gitignore` 忽略 |
| `build/` | fcdev 构建的输出，已被 `.gitignore` 忽略 |
