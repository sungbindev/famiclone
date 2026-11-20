# fcdev 6502 开发

6502 兼容机（NES、小霸王学习机等）开发的 VS Code 扩展，配合 `fcdev` 命令行使用。目前提供 ca65 汇编的语法高亮与智能感知，以及给 Copilot 用的 6502 反汇编工具。

## 功能

### ca65 语法高亮

`.s`、`.asm`、`.inc` 文件按 ca65 语法着色：6502 指令、汇编指令、标签、常量、数字、字符串与注释。C 文件用 VS Code 内置的 C 语法。

### ca65 汇编

打开 `.s`、`.asm`、`.inc` 文件时：

- 悬停：6502 指令的功能、标志位与寻址方式；ca65 伪指令的写法；符号的定义、值与注释。
- 补全：指令、伪指令（输入 `.`）、局部标签（输入 `@`）与可见的符号。
- 转到定义：标签、常量、宏等符号；包括 `.include` 引入的文件与工作区里的其他汇编文件。
- 大纲：标签、常量、宏、过程等。

`.include` 的文件先按当前文件所在目录查找，再按 `fcdev info` 给出的汇编引入目录查找，所以目标自带的符号（如 SB2000 的 `sb2000.inc`）可以直接看到。

### C

扩展自动运行 `fcdev sync`，生成 `.vscode/c_cpp_properties.json` 与 `compile_flags.txt`，让 C/C++ 扩展或 clangd 找到 cc65 的头文件与预定义宏，头文件里的文档注释会在悬停时显示。

安装本扩展时会一并安装 C/C++ 扩展（`ms-vscode.cpptools`），提供 C 的补全、悬停与转到定义；改用 clangd 时可卸载它。

触发时机：打开含 `fcdev.json` 的工作区、`fcdev.json` 变化。也可以手动运行命令“fcdev：同步 C/C++ 智能感知配置”。

C/C++ 扩展用哪个配置要在状态栏里选。`.vscode/c_cpp_properties.json` 里除 `fcdev` 外还有其他配置时，同步后会提示你选择 `fcdev`，点“选择配置”即可打开选择列表。

### 6502 反汇编工具（Copilot）

扩展给 Copilot 提供 18 个反汇编工具，在聊天里用 `#disasm_open` 这样的名字引用，模型也会自己选用。模型用它们多轮完成“映像到可汇编源码工程”的全过程：

- 工程与映像：`disasm_open`（新建或打开工程）、`disasm_status`（进度）、`disasm_layout`、`disasm_hex`、`disasm_find`。
- 分析：`disasm_trace`（从入口追踪并自动命名）、`disasm_list`、`disasm_view`（带符号与注释的反汇编）、`disasm_xrefs`、`disasm_routine`、`disasm_callgraph`、`disasm_detect`、`disasm_reference`。
- 编辑：`disasm_apply`（批量命名、标注区域、加注释，整批生效或整批不改）、`disasm_symbols`。
- 生成与校验：`disasm_generate`、`disasm_verify`（重新汇编后与原映像逐字节比较）、`disasm_build`。

会修改工程文件、写生成文件或运行汇编链接的调用，执行前会请你确认。工程目录必须在工作区里；要反汇编的映像可以在工作区外，确认里会写出路径。

## 使用前提

项目里已安装 `fcdev`（`npm install`），扩展经项目里的 `node_modules/fcdev` 运行命令，不依赖全局安装。反汇编工具需要支持工具接口的 `fcdev` 版本，过旧时工具会提示升级。没有安装时，汇编智能感知只用当前文件与工作区里的源码，原因写在“输出”面板的 fcdev 通道。

## 设置

| 设置 | 默认值 | 说明 |
| --- | --- | --- |
| `fcdev.intelliSense.autoSync` | `true` | 打开项目、修改 `fcdev.json` 时自动运行 `fcdev sync` |
| `fcdev.intelliSense.promptConfigurationSelection` | `true` | 同步后，配置文件里还有其他配置时提示选择 `fcdev` 配置 |

## 开发

```bash
npm install
npm run check      # 类型检查、oxlint、测试、构建
npm run package    # 生成 .vsix
```

按 F5 可以在扩展开发宿主里调试。设计与目录说明见 `docs/06-VS Code扩展.md`。

## 许可证

Apache-2.0
