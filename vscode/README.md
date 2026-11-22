# 6502 开发（fcdev）

NES、小霸王学习机（SB2000）等 6502 兼容机的 VS Code 开发工具：用随扩展发布的 cc65 构建 C 与 ca65 汇编程序，在模拟器 Ember 里运行与调试，反汇编已有程序，并给 Copilot 提供反汇编与模拟器工具。打开含 `fcdev.json` 的文件夹即可使用，不需要 Node 项目，也不需要另外安装 cc65。扩展只支持 Windows 64 位。

## 功能

### 构建与运行

- 任务类型 `fcdev`：为每个含 `fcdev.json` 的项目提供构建、构建磁盘镜像、运行三个任务，在“终端 → 运行任务”里选用；错误通过问题匹配器 `$fcdev` 出现在“问题”面板。
- 命令：“fcdev：构建项目”“构建磁盘镜像”“构建当前文件”“构建当前目录”“运行”“调试运行”“显示项目信息”“新建项目”。
- 运行与调试运行需要 Ember：在用户设置 `fcdev.emulator.path` 里指向 `Ember.exe`。扩展不自带 Ember，没设置时会提示来设置。
- 学习机（SB2000）目标需要 BIOS：在项目的 `fcdev.json` 里用 `firmware` 指向它。

### 调试

“调试运行”启动 Ember 的调试模式，程序载入后停在开机状态，之后用命令调试：“fcdev：调用调试请求”“查看调试请求说明”“保存调试截图”“监视调试事件”“显示调试服务地址”。调试服务的全部请求见 Ember 帮助文档里的“调试接口手册”。

### ca65 汇编

`.s`、`.asm`、`.inc` 文件按 ca65 语法着色，并提供：

- 悬停：6502 指令的功能、标志位与寻址方式；ca65 伪指令的写法；符号的定义、值与注释。
- 补全：指令、伪指令（输入 `.`）、局部标签（输入 `@`）与可见的符号。
- 转到定义：标签、常量、宏等符号；包括 `.include` 引入的文件与工作区里的其他汇编文件。
- 大纲：标签、常量、宏、过程等。

`.include` 的文件先按当前文件所在目录查找，再按项目目标的汇编引入目录查找，所以目标自带的符号（如 SB2000 的 `sb2000.inc`）可以直接看到。

### C

扩展自动同步 C/C++ 配置，生成 `.vscode/c_cpp_properties.json` 与 `compile_flags.txt`，让 C/C++ 扩展或 clangd 找到 cc65 的头文件与预定义宏，头文件里的文档注释会在悬停时显示。安装本扩展时会一并安装 C/C++ 扩展（`ms-vscode.cpptools`）；改用 clangd 时可卸载它。

触发时机：打开含 `fcdev.json` 的工作区、`fcdev.json` 变化。也可以手动运行命令“fcdev：同步 C/C++ 智能感知配置”。C/C++ 扩展用哪个配置要在状态栏里选；`.vscode/c_cpp_properties.json` 里除 `fcdev` 外还有其他配置时，同步后会提示你选择 `fcdev`。

“fcdev：查询 C 接口”查 SB2000 运行库的函数、模块与系统调用号。

### 新建项目

命令“fcdev：新建项目”选目标（NES、SB2000）与语言（C、汇编），在空目录里生成项目、示例程序与构建任务。

### 6502 反汇编

命令“fcdev：反汇编映像文件”把 NES 程序、SB-DOS 程序或平铺二进制反汇编成可重新汇编、逐字节一致的 ca65 工程。

### Copilot 工具

扩展给 Copilot 提供 6502 反汇编工具（19 个）、模拟器工具（11 个）与 fcdev 工具（构建、运行、项目信息、新建项目、C 接口、使用文档），在聊天里用 `#disasm_open` 这样的名字引用，模型也会自己选用。

- 工程与映像：`disasm_open`（新建或打开工程；给 `target` 为 `nes` 或 `sb2000` 时自动选映像格式、符号集与图形数据端口）、`disasm_status`（进度）、`disasm_layout`、`disasm_hex`、`disasm_find`。
- 分析：`disasm_trace`（从入口追踪并自动命名）、`disasm_list`、`disasm_view`（带符号与注释的反汇编）、`disasm_xrefs`、`disasm_routine`、`disasm_callgraph`、`disasm_detect`、`disasm_reference`。
- 编辑：`disasm_apply`（批量命名、标注区域、加注释，整批生效或整批不改）、`disasm_symbols`、`disasm_import_record`（把模拟器记录的指令轨迹、调用记录或 CDL 导入工程）。
- 生成与校验：`disasm_generate`、`disasm_verify`（重新汇编后与原映像逐字节比较）、`disasm_build`。
- 模拟器（Ember）：`emu_launch`（启动）、`emu_connect`（连接已运行的）、`emu_status`、`emu_load`（载入 NES 文件或整机）、`emu_run`、`emu_break`、`emu_step`、`emu_inspect`（读写内存、寄存器、单步核对、解析 bank）、`emu_screen`（截图）、`emu_input`（按键、手柄、鼠标）、`emu_record`（指令跟踪、CDL、调用记录）。让程序跑起来、记录执行过的地址再导入工程，可以补上静态追踪找不到的代码。
- fcdev：`fcdev_build`、`fcdev_run`、`fcdev_info`、`fcdev_new_project`、`fcdev_api`、`fcdev_docs`。

会修改工程文件、写生成文件、运行汇编链接、启动 Ember、写模拟器内存或寄存器的调用，执行前会请你确认。工程目录必须在工作区里；要反汇编的映像、载入的文件、记录文件可以在工作区外，确认里会写出路径。截图作为图像返回，需要较新的 VS Code；较旧的版本用 `file` 参数保存成文件。停用扩展时会关闭模拟器会话并结束工具启动的 Ember。

扩展还带一份反汇编指令、两个 skill 与自定义智能体“6502 反汇编工程师”：在 Copilot 聊天的智能体下拉里选它，给出映像路径，它按识别、追踪、区域分类、命名、注释、分层、生成与回读、构建八步完成，每步有完成标准（例如自动命名的符号数为 0、回读一致）。仓库里的 `apps/disasm-example` 是一个可以走一遍的示例。其中 `fcdev-app-development` 是用 fcdev 开发 NES 与 SB2000 程序的知识（新建项目、构建、运行、调试、C 接口与易错点），Copilot 在你要写这类程序时会自动读取。这些内容需要较新的 VS Code。

### 使用文档

命令“fcdev：打开使用文档”列出随扩展发布的文档：构建、模拟器与调试、反汇编、新建项目、NES 与 SB2000 的开发指南、系统调用手册、硬件说明与 C 接口参考。

## 设置

| 设置 | 默认值 | 说明 |
| --- | --- | --- |
| `fcdev.emulator.path` | 空 | Ember 可执行文件的路径；只能在用户设置里指定，工作区设置不能指定要运行的程序 |
| `fcdev.emulator.debugPort` | `5699` | 调试运行与 `emu_launch` 启动 Ember 时用的调试服务端口，调试命令与 `emu_connect` 也按它找到这个 Ember |
| `fcdev.intelliSense.autoSync` | `true` | 打开项目、修改 `fcdev.json` 时自动同步 C/C++ 配置 |
| `fcdev.intelliSense.promptConfigurationSelection` | `true` | 同步后，配置文件里还有其他配置时提示选择 `fcdev` 配置 |

## 开发

```bash
npm install
npm run check      # 类型检查、oxlint、测试、构建
npm run package    # 生成 .vsix
npm run release    # 检查并打包，加 -- --publish 才上传到市场
```

按 F5 可以在扩展开发宿主里调试。设计与目录说明见仓库的 `docs/06-VS Code扩展.md`。

## 许可证

Apache-2.0
