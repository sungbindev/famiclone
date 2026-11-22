---
name: fcdev-app-development
description: '使用 fcdev 扩展为 6502 兼容机开发程序时使用：开发 NES（红白机）程序、开发小霸王学习机 SB2000 的 SB-DOS 应用程序（.EXE）；包括新建项目、用 C 或 ca65 汇编编写、fcdev.json 配置、构建与在模拟器里运行、调试命令与 emu_* 工具（断点、读写内存、截图、按键）、反汇编、查询 SB2000 的 C 接口与系统调用、大程序换页、排查构建与运行问题；提供工具与命令速查、各目标的内存与程序格式、常用做法与易错点。'
user-invocable: false
---

# fcdev 程序开发

fcdev 是 VS Code 扩展（扩展 ID `sungbin.fcdev`）：用随扩展发布的 cc65 把 C 或 ca65 汇编构建成 6502 兼容机的程序，运行与调试用模拟器 Ember（不随扩展发布，要在用户设置 `fcdev.emulator.path` 里指向 `Ember.exe`）。首批目标：`nes`（NES，产物 `.nes`）与 `sb2000`（小霸王学习机的 SB-DOS，产物 `.EXE`；运行需要 `fcdev.json` 的 `firmware` 指向 BIOS，用 Ember 开机并在 SB-DOS 里输入程序名运行）。本 Skill 面向“用 fcdev 写目标机的程序”，不涉及修改 fcdev 自己的源码。

## 适用范围与依据

- 项目里有 `fcdev.json`，或用户要创建 NES、SB2000 程序时适用；扩展只支持 Windows 64 位。
- 结论取自 fcdev 的使用文档，并区分已在模拟器里验证过的用法与只读过代码的用法；没验证过的内容不得当作事实写进代码或回答，需要时先在模拟器里实测。
- 使用文档随扩展发布：用工具 `fcdev_docs`（不带查询词列出全部主题）或命令“fcdev：打开使用文档”读取，主题与章节索引见 [doc-index.md](references/doc-index.md)。不要让用户克隆 famiclone 仓库；工具不可用时请用户安装或升级扩展。
- 目标的头文件与符号文件随扩展发布，SB2000 的 C 接口用工具 `fcdev_api` 查询，整本参考文档是主题 `api`。

## 必须先读取的参考文件

| 任务 | 必须先读取 |
| --- | --- |
| 新建项目、写 `fcdev.json`、构建、增量与产物位置、构建报错、工具链与 Ember 的位置 | [build-and-config.md](references/build-and-config.md) |
| 开发 NES 程序（C 或汇编、显示、调色板、字库、内存与映射器限制） | [nes-development.md](references/nes-development.md) |
| 开发 SB2000 程序：程序格式、内存布局、启动代码、C 运行库与命名、大程序换页、示例程序、限制 | [sb2000-development.md](references/sb2000-development.md) |
| 在 SB2000 上做具体功能：文件读写、命令行参数、键盘手柄鼠标、页与窗口、中文显示、出错处理、出错码，以及避开已知的坑 | [sb2000-recipes.md](references/sb2000-recipes.md) |
| 在模拟器里运行与调试、截图核对、断点与内存读写、注入按键、反汇编已有程序 | [debugging-and-disassembly.md](references/debugging-and-disassembly.md) |
| 需要某个主题的完整细节（系统调用参数、硬件行为、构建规则、调试请求全表等），或上面的参考文件没覆盖的内容 | [doc-index.md](references/doc-index.md)，再按它的顺序读使用文档 |

任务同时涉及多项时全部读取。

## 工作流程

1. 确认目标：看 `fcdev.json` 的 `target`；新建项目先问清目标（`nes` 或 `sb2000`）与语言（`c` 或 `asm`）。不要猜目标。
2. 新建项目（空目录）：用工具 `fcdev_new_project`（参数 `directory`、`target`、`language`），或让用户运行命令“fcdev：新建项目”。生成 `fcdev.json`、`.gitignore`、`README.md`、`.vscode/tasks.json` 与 `src/hello.c` 或 `src/hello.s`；目录必须为空。
3. 写代码前先看目标的接口：
   - NES：读 `fcnes.h` / `fcnes.inc`，其余用 cc65 自带的 `nes.h`。
   - SB2000：用 `fcdev_api` 列出模块，`fcdev_api` 的 `query` 传 函数名、模块名、`$功能号` 或关键词查询；不要凭记忆写函数名与参数，也不要自己拼系统调用号。
   - 需要完整的参数、寄存器、命令表时，先用 `fcdev_docs` 找到文档再读，不要凭记忆补细节。
4. 构建：工具 `fcdev_build`（`path` 为项目的 `src` 目录或源文件，`disk` 为真时再生成磁盘镜像），或用户在 VS Code 里运行任务“fcdev：构建”。工具输出（`文件(行): Error: 信息`）原样给用户，不改写。
5. 运行与验证：工具 `fcdev_run`（`debug` 为真时调试模式启动）。需要自动核对时用调试模式、`emu_*` 工具与截图，见 [debugging-and-disassembly.md](references/debugging-and-disassembly.md)。验证结果写明用了什么手段；没在模拟器里跑过就明说“未运行验证”。
6. 缺少的系统服务用 `sb_syscall` 加符号常量自行调用，见 [sb2000-development.md](references/sb2000-development.md)。

## 工具、命令与任务速查

| 要做的事 | 智能体用 | 用户用 |
| --- | --- | --- |
| 新建项目 | `fcdev_new_project` | 命令“fcdev：新建项目” |
| 构建、生成磁盘镜像 | `fcdev_build`（`disk`） | 任务 `fcdev` 的 `build`、`disk`，命令“构建项目”“构建磁盘镜像”“构建当前文件”“构建当前目录” |
| 运行、调试运行 | `fcdev_run`（`debug`） | 任务 `run`，命令“运行”“调试运行” |
| 调试已运行的 Ember | `emu_connect`、`emu_run`、`emu_step`、`emu_break`、`emu_inspect`、`emu_screen`、`emu_input`、`emu_record` | 命令“调用调试请求”“查看调试请求说明”“保存调试截图”“监视调试事件”“显示调试服务地址” |
| 反汇编 | `disasm_*` 工具（反汇编工程师智能体） | 命令“反汇编映像文件” |
| 项目信息 | `fcdev_info` | 命令“显示项目信息” |
| SB2000 的 C 接口 | `fcdev_api` | 命令“查询 C 接口” |
| 使用文档 | `fcdev_docs` | 命令“打开使用文档” |
| 编辑器的头文件配置 | 自动同步 | 命令“同步 C/C++ 智能感知配置” |

## 通用规则

- 入口是 `main`；汇编写 `.export _main`。不要设置入口文件。
- 一个目录是一个程序：目录里放多个独立的 `main` 会链接失败，多个单文件程序只能逐个指定文件构建。
- C 语言按 cc65 的 C89 写法写（扩展给编辑器配的也是 `c89`）：变量声明放在块的开头；遇到语法错误先改成 C89 写法。cc65 不支持 `float`、`double`、`long long`。6502 只有 8 位寄存器与 256 字节硬件栈，另有 C 软件栈，局部变量、参数与递归深度都要小。`__fastcall__` 函数的参数约定见各目标文档。
- 汇编用 ca65 语法；C 函数符号带前导下划线（`_sb_puts`），汇编里导入导出时也要带。
- 程序名：NES 不限制；SB2000 必须符合 DOS 8.3 规则，不能与 DOS 内部命令重名（如 `COPY`、`TYPE`、`DIR`），超出规则扩展直接报错，不会截断。
- 不要把 BIOS、ROM 文件提交进仓库，也不要为了运行而发布它们；SB2000 的 BIOS 由用户提供，路径写进 `fcdev.json` 的 `firmware`。
- 构建产物与中间文件在 `build/`，已被模板的 `.gitignore` 忽略，不要手工修改。
- 构建只用扩展自带的 cc65，没有指向自备 cc65 的配置；Ember 的位置只能由用户在设置里指定，智能体不能代填。
- 用户没有提供 SB2000 的 BIOS，或没有设置 Ember 的位置，无法运行验证时，只做到构建通过，并说明限制。

## 常见问题定位

| 现象 | 先查 |
| --- | --- |
| 报“找不到工具链”或“工具链不完整” | 扩展目录里的工具链缺失，重新安装扩展；开发扩展本身时运行 `npm run build:toolchain`；见 [build-and-config.md](references/build-and-config.md) |
| 报“还没有设置 Ember 的位置”或“找不到 Ember” | 请用户在设置 `fcdev.emulator.path` 里填 `Ember.exe` 的路径 |
| `ld65` 报内存区放不下 | NES：程序超过 16K PRG 或 RAM；SB2000：超过 15K，考虑 `targetOptions.banks`，见各目标文档 |
| 缺少或重复 `main` | 目录里是否混入了别的程序的源文件 |
| SB2000 程序在 DOS 里“找不到命令” | 程序名与内部命令重名，或镜像里没有该文件（先用 `fcdev_build` 的 `disk` 生成镜像） |
| SB2000 程序运行后屏幕一闪而过 | 返回 DOS 会清屏，退出前先等键，见 [sb2000-recipes.md](references/sb2000-recipes.md) |
| 运行 SB2000 报缺固件 | `fcdev.json` 没写 `firmware` |
