---
name: 6502 反汇编规则
description: 编辑或使用反汇编工程（disasm.json）时遵循的规则：工程是唯一的修改入口，生成的文件不手改，映像与 BIOS 不提交，做法按 skill disassemble-6502。
applyTo: '**/disasm.json'
---

# 6502 反汇编规则

## 工程与文件

- 反汇编的对象是工程（工程目录里的 `disasm.json` 加映像），不是源码文本。所有修改都经 `disasm_*` 工具。
- 不手改 `src/`、`include/`、`link.cfg`、`verify/`，下次 `disasm_generate` 会覆盖。
- 映像与记录文件可以在工作区之外，工程目录与生成目录必须在工作区里。
- 不把有版权的映像、BIOS 复制进仓库，也不提交；只在本机分析。
- `disasm_status` 的 `image.stale` 为 `true` 时映像文件已变，工程已过期，停下并告诉用户，不继续编辑。

## 做法

- 必须先读取 skill `disassemble-6502`：八个步骤、每步的操作顺序与完成标准、命名与注释的依据和格式都在里面。
- 需要用模拟器记录运行数据时，再读 skill `ember-emulator`；模拟器的起步顺序与禁止事项见指令“6502 工具使用规则”。
