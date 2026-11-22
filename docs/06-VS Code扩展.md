# VS Code 扩展

扩展放在仓库的 `vscode/` 目录，包名 `fcdev`，显示名“FC Development”，图标用模拟器 Ember 的 logo（`vscode/resources/icon.png`，复制自 `ember/resources/ember-logo.png`）。它是 fcdev 唯一的发行形式：构建、运行、调试、新建项目、反汇编、智能感知与 Copilot 工具都在这里，整体架构见 [01-软件架构设计](01-软件架构设计.md)。

## 1. 范围

- 构建：任务类型 `fcdev`、构建命令、问题匹配器 `$fcdev`，用随扩展发布的 cc65 构建 C 与 ca65 汇编程序，使用说明见 `vscode/docs/01-构建.md`。
- 运行与调试：命令“运行”“调试运行”、5 个调试命令，经 Ember 的调试服务工作；Ember 的位置来自设置，使用说明见 `vscode/docs/02-模拟器与调试.md`。
- 新建项目：命令“fcdev：新建项目”与五个模板，使用说明见 `vscode/docs/04-新建项目与模板.md`。
- 反汇编：命令“反汇编映像文件”与给 Copilot 用的工具，见第 6 节与 [09-AI反汇编工具](09-AI反汇编工具.md)。
- ca65 汇编（`.s`、`.asm`、`.inc`）的悬停、补全、转到定义、文档符号与语法高亮。
- C 代码的智能感知：自动同步 C/C++ 配置，让 C/C++ 扩展与 clangd 找到 cc65 的头文件与宏；头文件里符号与函数的文档注释（`/** */`）悬停时显示。C 用 VS Code 内置的 C 语法。
- 命令“fcdev：查询 C 接口”“fcdev：打开使用文档”与对应的工具。
- 给 Copilot 用的 36 个语言模型工具（19 个反汇编工具、11 个模拟器工具、6 个 fcdev 工具），以及两份指令（6502 工具使用规则、6502 反汇编规则）、三个 skill（反汇编、fcdev 程序开发、Ember 模拟器操作）与一个自定义智能体（“6502 工程师”），见第 6 节。

不含：调试适配器协议（DAP）与断点、单步等调试界面，调试经命令与 `emu_*` 工具完成，登记在 [待办备忘](待办备忘.md)。

## 2. 语言支持的分工

| 语言 | 由谁提供 | 做法 |
| --- | --- | --- |
| C | VS Code 的 C/C++ 扩展或 clangd | 本扩展同步 `.vscode/c_cpp_properties.json` 与 `compile_flags.txt`，里面有头文件目录与预定义宏 |
| 汇编 | 本扩展 | 自己解析 ca65 源码，不依赖其他扩展 |

本扩展的 `package.json` 用 `extensionPack` 带上 C/C++ 扩展（`ms-vscode.cpptools`），安装本扩展时一并安装；用户仍可单独卸载它，改用 clangd。

C 的头文件注释用 Doxygen 的 `/** */` 写法，C/C++ 扩展与 clangd 都会在悬停时显示；`sb2000-symbols.h` 里每个常量宏上方的注释由 `vscode/tools/gen/gen-sb2000-header.mjs` 从 `sb2000.inc` 的行尾注释生成。

### 2.1 C 的配置同步

| 触发 | 行为 |
| --- | --- |
| 扩展激活（工作区里有 `fcdev.json`） | 同步所有工作区文件夹里的 fcdev 项目 |
| 工作区文件夹增减 | 同步 |
| `fcdev.json` 新建或修改 | 丢弃该项目的信息缓存，合并 500 毫秒内的连续变化后同步该项目 |
| 命令“fcdev：同步 C/C++ 智能感知配置” | 手动同步，带进度，结束后弹出结果；没有 fcdev 项目时提示 |

自动同步只写日志，失败也不弹提示；日志在“输出”面板的 fcdev 通道。设置 `fcdev.intelliSense.autoSync`（默认开启）可关闭自动同步。

生成的头文件目录在扩展目录里，路径随扩展版本变化：激活时总是同步所有项目，所以升级扩展后配置自动更新。

预定义宏里带 `__fastcall__`、`__cdecl__` 两个空宏：cc65 的头文件用它们标注调用约定，C/C++ 扩展与 clangd 不认识，不定义会报语法错误。这两个宏只出现在项目信息与同步的结果里，不参与构建。

生成的 `compile_flags.txt` 第一行是标记 `-DFCDEV_GENERATED`，`c_cpp_properties.json` 里的配置名是 `fcdev`；已有同名文件但没有标记时视为用户自己的文件，不覆盖。

#### 提示选择配置

C/C++ 扩展用哪个配置，只保存在它自己的工作区状态里，其他扩展无法读写，所以本扩展不能自动切换到 `fcdev` 配置。`.vscode/c_cpp_properties.json` 里除 `fcdev` 外还有其他配置时（例如名为 `Win32` 的默认配置），C/C++ 扩展可能选的不是 `fcdev`，这时 cc65 的头文件不会生效。扩展在同步成功后检查这个文件，条件满足时弹出提示：

- 提示带“选择配置”按钮，点击后运行 C/C++ 扩展的命令 `C_Cpp.ConfigurationSelect`，由用户在列表里点选 `fcdev`。
- 只在安装了 C/C++ 扩展（`ms-vscode.cpptools`）时提示；只用 clangd 的项目不受影响。
- 每个项目每次 VS Code 会话只提示一次；在同一次同步里有多个项目时合并成一条。
- 设置 `fcdev.intelliSense.promptConfigurationSelection`（默认开启）可关闭提示。
- 只检查同步成功的项目；读取配置文件出错只写日志，不影响同步。

### 2.2 汇编

| 功能 | 行为 |
| --- | --- |
| 悬停 | 6502 指令：功能、影响的标志位、寻址方式的写法与字节数；ca65 伪指令：写法与说明；符号：定义写法、种类、常量的值（同时给出十进制、十六进制与二进制）、源码里的说明注释、所在分组与文件行号 |
| 补全 | 指令助记符、伪指令（输入 `.` 触发）、局部标签（输入 `@` 触发）、可见的符号 |
| 转到定义 | 标签、常量、宏等符号跳到定义处；同名定义在同一范围内有多处时全部列出 |
| 文档符号 | 大纲与面包屑里列出标签、常量、`.define`、宏、`.proc`、`.scope`、`.struct`、`.enum` |

可见的符号来自：

1. 当前文件，含局部标签与未保存的修改；名字重名时优先取这里的定义。
2. 当前文件 `.include` 的文件，最多展开 8 层；路径先按当前文件所在目录找，再按项目信息给出的汇编引入目录找（含目标的 `asminclude`，所以 `sb2000.inc` 的符号能直接看到）。
3. 工作区里其他汇编文件里的非局部符号。

后两项读自磁盘并缓存；汇编文件变化时，丢弃该文件的解析缓存与工作区文件列表。扩展不理解条件汇编的取舍，`.if` 里的符号都会被收集。

### 2.3 汇编的语法高亮

语法文件是 `syntaxes/ca65.tmLanguage.json`，作用域名 `source.ca65`，对 `.s`、`.asm`、`.inc` 生效。它按行匹配，不理解上下文，所以只靠行首位置和固定词表分类。

| 内容 | 作用域 |
| --- | --- |
| `;` 注释 | `comment.line.semicolon.ca65` |
| `"..."`、`'...'` 字符串（行尾自动结束，字符串里的分号不算注释） | `string.quoted.double.ca65`、`string.quoted.single.ca65` |
| 行首的标签定义（`name:`、`@local:`、`name::`），行首的 `:` 匿名标签 | `entity.name.function.label.ca65`、`entity.name.function.label.anonymous.ca65` |
| 行首的常量定义（`NAME = 值`、`NAME := 值`） | `variable.other.constant.ca65` |
| `.proc`、`.macro` 后的名字；`.scope`、`.struct`、`.enum` 后的名字 | `entity.name.function.ca65`、`entity.name.type.ca65` |
| `.if` 系列、`.else`、`.elseif`、`.endif` | `keyword.control.conditional.ca65` |
| 伪函数（`.defined`、`.sizeof`、`.lobyte` 等） | `support.function.pseudo.ca65` |
| 其他以 `.` 开头的汇编指令 | `keyword.other.directive.ca65` |
| 分支、跳转、返回指令（`bcc`、`jmp`、`jsr`、`rts`、`brk` 等） | `keyword.control.flow.ca65` |
| 其他 6502 指令 | `keyword.other.mnemonic.ca65` |
| `asl a` 等的累加器、`,x`、`,y` | `variable.language.register.ca65` |
| `$` 十六进制、`%` 二进制、十进制数 | `constant.numeric.hex.ca65`、`constant.numeric.binary.ca65`、`constant.numeric.decimal.ca65` |
| `#` 立即数标记 | `keyword.operator.immediate.ca65` |
| `@name` 局部标签引用、`:+`、`:-` 匿名标签引用 | `variable.other.local.ca65`、`variable.other.anonymous.ca65` |

- 指令与汇编指令不分大小写。
- 标签定义要求冒号紧跟在名字后面，行首 `bne :+` 因此不会被当成标签。
- 宏的调用与不在词表里的标识符不着色，因为单行匹配无法区分它们与符号。
- 指令词表与 `vscode/src/shared/assembly/instruction-reference.ts`、`vscode/src/shared/assembly/directive-reference.ts` 的覆盖，由 `test/syntaxes/ca65-grammar.test.ts` 逐条检查。

## 3. 构建、运行与项目信息

### 3.1 项目信息

项目信息由扩展自己读取，不经任何命令行：`logic/project` 读 `fcdev.json`，`logic/targets` 读 `vscode/targets/<目标>/target.json`，合成项目根、目标、头文件目录、汇编引入目录与预定义宏；每个项目缓存一份，`fcdev.json` 变化后失效。智能感知、同步与命令“显示项目信息”、工具 `fcdev_info` 共用它。项目没有 `fcdev.json` 或配置有误时，汇编智能感知只用当前文件与工作区里的源码，引入目录里的符号看不到；原因写在日志里，不弹提示。

### 3.2 任务、命令与工具

| 位置 | 内容 |
| --- | --- |
| `entry/build/build-task-provider.ts` | 任务类型 `fcdev`（`command`、`path`、`target`、`debug`），为每个含 `fcdev.json` 的项目提供构建、运行两个任务；解析 `.vscode/tasks.json` 里的任务；问题匹配器 `$fcdev` |
| `entry/build/build-task-runner.ts` | 任务在终端里运行：调用构建服务或运行服务，输出原始工具输出，失败输出“错误：”加说明与建议，退出码为 1 |
| `entry/build/build-commands.ts` | 命令“构建项目”“构建当前文件”“构建当前目录”“运行”“调试运行”“显示项目信息”，都经任务运行 |
| `entry/fcdev-tools/` | 工具 `fcdev_build`、`fcdev_run`、`fcdev_info`、`fcdev_new_project`、`fcdev_api`、`fcdev_docs` |
| `entry/debug-client/debug-commands.ts` | 5 个调试命令，结果写到“fcdev 调试”输出通道 |
| `entry/scaffold`、`entry/c-api`、`entry/usage-docs`、`entry/disassembly/disassembly-commands.ts` | 命令“新建项目”“查询 C 接口”“打开使用文档”“反汇编映像文件” |
| `entry/emulator/workspace-emulator-settings.ts` | 读设置 `fcdev.emulator.path` 与 `fcdev.emulator.debugPort`（作用域 `machine`，每次使用时读取） |

构建、运行、调试的行为细节在使用文档里，不在本文重复。

## 4. 目录与分层

按分层组织，层内再按功能分目录，具体类只在组装根 `src/bootstrap/extension.ts` 创建：

| 目录 | 职责 |
| --- | --- |
| `src/contracts` | 接口与数据类型：汇编智能感知服务、汇编源文件存取、C 配置同步服务、C/C++ 配置检查器、项目配置读取、目标仓库与目标描述读取、工具链定位与工具运行、构建服务、新建项目服务、使用文档服务、C 接口服务、反汇编操作、模拟器操作、进程启动与设置、调试连接器与会话、文件系统、扩展资源位置、工具描述；`debug-client/generated/api.ts` 是 Ember 生成的请求与事件类型 |
| `src/shared` | 纯函数与数据：ca65 解析、行内词法、数字格式、指令与伪指令参考、悬停文字、c_cpp_properties.json 的解析与生成、项目配置 fcdev.json 的校验、目标定义的解析与取值、构建用的文本处理（依赖文件、工具参数、多页链接配置）与 GB2312 转码、打包器与 FAT12 镜像、新建项目的文件、使用文档的主题、C 接口的解析、调试客户端的请求目录与参数解析、运行结果文字、反汇编的指令表、解码、映像格式、工程模型与视图、可预期错误 |
| `src/logic` | 业务：符号解析（含 `.include` 展开）、汇编智能感知服务、C 配置同步、项目信息、目标读取、构建规划与编译链接（含增量）、磁盘镜像、新建项目、使用文档与 C 接口查询、反汇编的分析、工程编辑、源码生成与回读、模拟器操作与会话、调试客户端（帧解码、会话、客户端、连接器） |
| `src/infra` | 与外部交互：读工作区文件、查找项目、读取 c_cpp_properties.json、工具链定位与工具进程运行、扩展资源位置、Ember 进程的启动与结束、端点文件、WebSocket 传输、接口描述文档读取、节点文件系统 |
| `src/entry` | VS Code 入口：汇编提供者、命令与文件监视器、任务、语言模型工具的注册与路径规则、设置读取 |
| `src/platform` | 日志（输出通道） |
| `src/bootstrap` | 组装根 `extension.ts` |
| `syntaxes` | ca65 的 TextMate 语法文件（静态资源，不经分层） |
| `targets`、`toolchain`、`templates`、`docs`、`debug`、`chat` | 随扩展发布的资源：目标定义与运行库、cc65 工具链（生成物）、项目模板、使用文档、调试接口描述、聊天贡献文件 |

- 层之间的依赖方向、文件头、注释宽度与 `package.json` 声明和代码的一致性，由 `test/structure.test.ts` 检查。
- 各层之间只经 `contracts` 里的接口协作；只有 `entry`、`platform` 与登记的两个 `infra` 文件（工作区汇编源文件存取、工作区项目定位）可以引用 `vscode`。
- `shared` 与 `logic` 只能引用 Node 的纯函数模块（`node:path`、`node:crypto`），文件、进程的读写都经 `contracts` 的接口。
- 仓库里的脚本经 `tools/extension.mjs` 加载 `vscode/out/` 下编译后的模块，所以这些层不能引用 `vscode`；这条由结构测试检查。
- 读取随扩展发布的资源（目标、工具链、模板、文档）一律经 `ExtensionResources` 取路径。

## 5. 构建、测试与打包

在 `vscode/` 目录运行：

| 命令 | 用途 |
| --- | --- |
| `npm run check` | 类型检查、oxlint、测试、构建、维护脚本的测试 |
| `npm run build:toolchain` | 用 `vscode/third_party/cc65/` 的源码构建并组装 `vscode/toolchain/`（需要 mingw32-make 与 gcc） |
| `npm run build:libs` | 编译各目标的运行库到 `vscode/targets/<目标>/lib`，需要先 `npm run build` 与 `npm run build:toolchain` |
| `npm run gen:docs` | 重新生成 SB2000 的符号头文件、接口参考、符号参考与示例源码 |
| `npm run package` | 生成 `.vsix`（不入库）；发布到应用市场需要发布者账号，由用户操作，见 [05-发布](05-发布.md) |
| `npm run check:package` | 核对 `.vsix` 的内容 |

测试在 Node 环境里运行，`vscode` 模块用 `test/support/vscode-mock.ts` 替代；入口层的测试验证提供者与命令的行为，真实 VS Code 里的显示效果要手动打开项目核对。语法文件的测试用开发依赖 `vscode-textmate` 与 `vscode-oniguruma` 真实加载语法并分词，不依赖 VS Code；颜色由用户的主题决定，只检查作用域名。测试分层见 [04-测试与验证](04-测试与验证.md)。

## 6. 反汇编与模拟器工具

扩展用 VS Code 的语言模型工具接口（`contributes.languageModelTools` 声明，`vscode.lm.registerTool` 实现，不使用 MCP）给 Copilot 提供 36 个工具：19 个反汇编工具、11 个模拟器工具与 6 个 fcdev 工具。工作流、工具的用途见 [09-AI反汇编工具](09-AI反汇编工具.md)，核心见 [07-反汇编设计](07-反汇编设计.md)。

### 6.1 工具

| 组 | 工具 |
| --- | --- |
| 工程与映像 | `disasm_open`、`disasm_status`、`disasm_layout`、`disasm_hex`、`disasm_find` |
| 分析 | `disasm_trace`、`disasm_list`、`disasm_view`、`disasm_xrefs`、`disasm_routine`、`disasm_callgraph`、`disasm_detect`、`disasm_reference` |
| 编辑 | `disasm_apply`、`disasm_symbols`、`disasm_import_record` |
| 生成与校验 | `disasm_generate`、`disasm_verify`、`disasm_build` |
| 模拟器 | `emu_connect`、`emu_launch`、`emu_status`、`emu_load`、`emu_run`、`emu_break`、`emu_step`、`emu_inspect`、`emu_screen`、`emu_input`、`emu_record` |
| fcdev | `fcdev_new_project`、`fcdev_build`、`fcdev_run`、`fcdev_info`、`fcdev_api`、`fcdev_docs` |

- 工具的名字、给模型的说明与参数结构只写在 `package.json` 的 `languageModelTools` 里；代码（`src/entry/disassembly` 每组一个文件、`src/entry/fcdev-tools`）只写“对应的操作、要不要确认、显示的文字、要检查的路径参数”，运行时按名字读 `package.json` 里的参数结构检查输入（`shared/disassembly/schema-validator.ts` 支持 JSON Schema 的一个子集）。`test/entry/disassembly-tools.test.ts` 与 `test/entry/fcdev-tools/` 核对两边的工具一一对应。
- 除 `disasm_build` 外，每个反汇编与模拟器工具对应核心的一个操作（`disasm_open` 对应 `open`，`disasm_import_record` 对应 `importRecord`，`emu_break` 对应 `breakpoint`，`emu_screen` 对应 `screenshot`，依此类推）；`disasm_reference` 不给 `projectDir` 时只查指令。模拟器工具的操作在 `src/logic/emulator/`，会话见 6.4。
- `disasm_build` 用构建服务构建项目（`logic/disassembly/build-service-comparer.ts`），把产物与工程记录的原映像逐字节比较；构建的是按生成的源码整理出来的 fcdev 项目，不是反汇编工程目录本身。

### 6.2 路径与确认

- 相对路径按第一个工作区文件夹解析；`projectDir`、`buildPath` 必须在工作区里，否则拒绝。要反汇编的映像（`image`）、符号集（`symbolSets`、`file`）可以在工作区外，但调用前的确认里会写出这些路径。
- 会修改工程文件、写生成文件、运行外部程序、启动进程或改模拟器状态的调用，在调用前弹出确认：`disasm_trace`、`disasm_apply`、`disasm_generate`、`disasm_verify`、`disasm_build`、`disasm_import_record`、`emu_launch`，以及带 `image` 的 `disasm_open`、带 `segments` 的 `disasm_layout`、`action` 为 `import` 的 `disasm_symbols`、`what` 为 `write` 或 `setState` 的 `emu_inspect`、带 `file` 的 `emu_screen`；`fcdev_build`、`fcdev_run`、`fcdev_new_project` 也会确认。其他调用只显示一行说明。
- 任何调用里有在工作区之外的路径时都要确认，确认里写出这些路径（如 `emu_load` 的 `file`、`firmware`、`diskImage`，`disasm_import_record` 的记录文件）；`emu_screen` 的保存路径必须在工作区里。
- 不向模型开放会改变被运行程序的参数：Ember 的位置只来自用户设置 `fcdev.emulator.path`，模型不能指定。

### 6.3 结果与出错

- 结果是缩进的 JSON；清单、十六进制视图、调用图这类多行文字（结果里的 `text` 与 `listing`）单独成块原样输出。
- 一次结果最多 20000 个字符，超出时截断并说明怎么缩小范围；列表类工具带 `limit`（默认 50，最多 500）与 `cursor`，下一页的游标是结果里的 `nextCursor`。
- 出错时不抛异常，把原因作为结果文字（以“错误：”开头）返回，模型据此改参数；非预期的错误写进“fcdev”输出通道，并返回通用说明。

### 6.4 模拟器会话

- 连接保存在模拟器操作的会话存储里，整个扩展主机进程只有一个会话；`emu_connect` 与 `emu_launch` 建立会话（已有会话先关闭，不结束进程），`emu_status` 查看，带 `close` 时关闭，再带 `terminate` 才结束自己启动的 Ember，不影响用户自己打开的。
- 扩展停用时（`deactivate`）如果用过模拟器，会关闭会话并结束自己启动的 Ember；没用过则什么也不做。
- `emu_screen` 的截图作为图像部分随结果返回（文字里不含 base64）。图像部分需要较新的 VS Code（工具结果里的 `LanguageModelDataPart`，按运行时是否存在检测）；不支持时结果里会说明，让模型给 `file` 参数把截图保存成文件。
- 记录文件由 Ember 写在它的调试输出目录，`emu_record` 结束时返回完整路径，再交给 `disasm_import_record` 导入。

### 6.5 激活

激活事件有 `workspaceContains:fcdev.json`、`workspaceContains:**/disasm.json` 与 `onStartupFinished`：只有一个映像文件的反汇编工作区没有 `fcdev.json`，工具也要能用。真实 VS Code 里 Copilot 是否发现并调用工具，要手动核对。

### 6.6 指令、skill 与 agent

| 文件 | 声明 | 内容 |
| --- | --- | --- |
| `chat/instructions/fcdev-tools.instructions.md` | `chatInstructions`，没有 `applyTo`，按 `description` 触发 | 按任务类型选 skill、读完 skill 才调用工具、不自行检查 Ember 的位置与设置、模拟器的固定起步顺序（`emu_status`、`emu_launch`、`emu_connect`）、不按进程名结束进程、同一错误最多重试一次、汇报内容 |
| `chat/instructions/6502-disassembly.instructions.md` | `chatInstructions`，`applyTo: '**/disasm.json'` | 只经工具改工程、不手改生成的文件、映像与 BIOS 不提交、`image.stale` 时停下、做法读 skill |
| `chat/skills/disassemble-6502/SKILL.md` 与 `references/` | `chatSkills` | 开始前收集的输入、新建与继续的判断、八步各自的操作顺序与完成标准（数字取自 `disasm_status`）、命名与注释的依据、各种输入的打开方式、区域类型与参数、运行记录的闭环与导入种类对应、符号与提示的形式 |
| `chat/skills/ember-emulator/SKILL.md` 与 `references/` | `chatSkills` | `emu_*` 工具的任务判定、起步与载入、观察与核对循环、断点与单步、记录、收尾；工具参数表、机型与开机后的操作、报错处理表 |
| `chat/skills/fcdev-app-development/SKILL.md` 与 `references/` | `chatSkills` | 用 fcdev 开发 NES 与 SB2000 程序：任务判定与五个流程（新建、编写、构建、运行核对、排查）、`fcdev.json`、构建、C 接口、各目标的内存与程序格式、使用文档的读取顺序；内容随扩展的命令与工具维护，不放在 `copilot-plugins` |
| `chat/agents/6502-engineer.agent.md` | `chatAgents` | “6502 工程师”：除 `agent` 以外的全部内置工具加本扩展的全部工具（写成 `sungbin.fcdev/工具引用名`），一个智能体覆盖开发、模拟器调试与反汇编；按任务类型选 skill、列合并的步骤、遇到指定情形停下询问，结束时按任务类型汇报 |

- 路径相对扩展根目录；skill 所在目录名必须与 `SKILL.md` 的 `name` 一致。`chat/` 随扩展打包。
- `test/chat-contributions.test.ts` 检查：声明的文件都存在；前置信息完整（反汇编指令还要有 `applyTo`）；agent 的工具恰好是除 `agent` 以外的全部内置工具加清单里的全部语言模型工具；skill 与指令里提到的 `disasm_*`、`emu_*`、`fcdev_*` 都存在。
- 这三类贡献点要较新的 VS Code，低版本会忽略它们，不影响扩展的其他功能。agent 里工具引用的写法与实际效果要在真实 VS Code 里核对。示例走法见 `apps/disasm-example/README.md`。
- `engines.vscode` 为 `^1.101.0`：调试客户端用全局 `WebSocket`，扩展宿主的 Node 要达到 22（VS Code 1.101 起的扩展宿主是 Node 22.15.1）。
