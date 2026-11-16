# VS Code 扩展

扩展放在仓库的 `vscode/` 目录，包名 `fcdev`，显示名“fcdev 6502 开发”。首版做 ca65 汇编的语法高亮，以及 ca65 汇编与 cc65 的 C 的智能感知。扩展不内置编译器，只经 `node_modules` 里的 `fcdev` 命令行读取项目信息。

## 1. 范围

首版包含：

- ca65 汇编（`.s`、`.asm`、`.inc`）的悬停、补全、转到定义、文档符号。
- C 代码的智能感知：自动运行 `fcdev sync`，让 C/C++ 扩展与 clangd 找到 cc65 的头文件与宏。
- 头文件里符号与函数的文档注释（`/** */`），悬停时显示。
- ca65 汇编的语法高亮（TextMate 语法）。C 用 VS Code 内置的 C 语法，不另提供。

首版不含，登记在 [待办备忘](待办备忘.md)：任务与问题匹配器、构建与运行命令、新建项目、调试。

## 2. 语言支持的分工

| 语言 | 由谁提供 | 做法 |
| --- | --- | --- |
| C | VS Code 的 C/C++ 扩展或 clangd | 本扩展运行 `fcdev sync`，生成 `.vscode/c_cpp_properties.json` 与 `compile_flags.txt`，里面有头文件目录与预定义宏 |
| 汇编 | 本扩展 | 自己解析 ca65 源码，不依赖其他扩展 |

本扩展的 `package.json` 用 `extensionPack` 带上 C/C++ 扩展（`ms-vscode.cpptools`），安装本扩展时一并安装；用户仍可单独卸载它，改用 clangd。

C 的头文件注释用 Doxygen 的 `/** */` 写法，C/C++ 扩展与 clangd 都会在悬停时显示；`sb2000-symbols.h` 里每个常量宏上方的注释由 `tools/gen-sb2000-header.mjs` 从 `sb2000.inc` 的行尾注释生成。

### 2.1 C 的配置同步

| 触发 | 行为 |
| --- | --- |
| 扩展激活（工作区里有 `fcdev.json`） | 同步所有工作区文件夹里的 fcdev 项目 |
| 工作区文件夹增减 | 同步 |
| `fcdev.json` 新建或修改 | 丢弃该项目的信息缓存，合并 500 毫秒内的连续变化后同步该项目 |
| 命令“fcdev：同步 C/C++ 智能感知配置” | 手动同步，带进度，结束后弹出结果；没有 fcdev 项目时提示 |

自动同步只写日志，失败也不弹提示；日志在“输出”面板的 fcdev 通道。设置 `fcdev.intelliSense.autoSync`（默认开启）可关闭自动同步。

`fcdev sync` 的预定义宏里带 `__fastcall__`、`__cdecl__` 两个空宏：cc65 的头文件用它们标注调用约定，C/C++ 扩展与 clangd 不认识，不定义会报语法错误。这两个宏只出现在 `info` 与 `sync` 的输出里，不参与构建。

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
2. 当前文件 `.include` 的文件，最多展开 8 层；路径先按当前文件所在目录找，再按 `fcdev info --json` 给出的汇编引入目录找（含目标的 `asminclude`，所以 `sb2000.inc` 的符号能直接看到）。
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

## 3. 项目信息的来源

扩展经 `node <项目根>/node_modules/fcdev/bin/fcdev.mjs` 调用命令行，不经 shell，不依赖全局安装。

| 命令 | 用途 |
| --- | --- |
| `fcdev info --json` | 读取目标、头文件目录、汇编引入目录与预定义宏；每个项目缓存一份，`fcdev.json` 变化后失效 |
| `fcdev sync` | 生成 C/C++ 扩展的配置 |

项目没有安装 `fcdev` 或命令失败时，汇编智能感知只用当前文件与工作区里的源码，引入目录里的符号看不到；原因写在日志里，不弹提示。

## 4. 目录与分层

按分层组织，层内再按功能分目录，具体类只在组装根创建：

| 目录 | 职责 |
| --- | --- |
| `src/contracts` | 接口：汇编智能感知服务、汇编源文件存取、C 配置同步服务、C/C++ 配置检查器、命令行执行器、项目信息提供者 |
| `src/shared` | 纯函数与数据：ca65 解析、行内词法、数字格式、指令与伪指令参考、悬停文字、项目信息与 c_cpp_properties.json 的解析 |
| `src/logic` | 业务：符号解析（含 `.include` 展开）、汇编智能感知服务、C 配置同步服务 |
| `src/infra` | 与外部交互：读工作区文件、调用命令行、查找项目、读取 c_cpp_properties.json |
| `src/entry` | VS Code 入口：四个汇编提供者、命令与文件监视器 |
| `src/platform` | 日志（输出通道） |
| `src/bootstrap` | 组装根 `extension.ts` |
| `syntaxes` | ca65 的 TextMate 语法文件（静态资源，不经分层） |

- 层之间的依赖方向、文件头、注释宽度与 `package.json` 声明和代码的一致性，由 `test/structure.test.ts` 检查。
- 各层之间只经 `contracts` 里的接口协作；只有 `entry`、`platform`、`infra` 可以引用 `vscode`。

## 5. 构建、测试与打包

在 `vscode/` 目录运行：

| 命令 | 用途 |
| --- | --- |
| `npm run check` | 类型检查、oxlint、测试、构建 |
| `npm run package` | 生成 `.vsix`（不入库）；发布到应用市场需要发布者账号，由用户操作 |

测试在 Node 环境里运行，`vscode` 模块用 `test/support/vscode-mock.ts` 替代；入口层的测试验证提供者与命令的行为，真实 VS Code 里的显示效果要手动打开项目核对。语法文件的测试用开发依赖 `vscode-textmate` 与 `vscode-oniguruma` 真实加载语法并分词，不依赖 VS Code；颜色由用户的主题决定，只检查作用域名。
