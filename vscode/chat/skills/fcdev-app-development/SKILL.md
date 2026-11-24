---
name: fcdev-app-development
description: '使用 FC Dev 扩展为 6502 兼容机开发程序时使用：开发 NES（红白机）程序、开发小霸王学习机 SB2000 的 SB-DOS 应用程序（.EXE）；包括新建项目、用 C 或 ca65 汇编编写、fcdev.json 配置、构建、在模拟器里运行与核对、查询 SB2000 的 C 接口与系统调用、大程序换页、排查构建与运行问题；给出任务判定、五个流程的逐步做法与完成标准、工具速查与易错点。'
user-invocable: false
---

# FC Dev 程序开发

FC Dev 是 VS Code 扩展（`sungbin.fcdev`）：用随扩展发布的 cc65 把 C 或 ca65 汇编构建成 6502 兼容机的程序，运行与调试用模拟器 Ember（不随扩展发布，位置由用户设置 `fcdev.emulator.path` 指定）。目标有六个：`nes`（产物 `.nes`）、`sb2000`（小霸王学习机的 SB-DOS，产物 `.EXE`，运行需要 `fcdev.json` 的 `firmware` 指向 BIOS）、`sb97`（小霸王 97 的 DOS，只有汇编，产物 `.EXE`，运行需要 `firmware` 指向 ROM）、`yuxing`（裕兴学习机的 DOS，只有汇编，产物 `.EXE`，运行需要 `firmware` 指向学习卡的 ROM，DOS 里运行要写扩展名）、`bbg`（步步高学生电脑的 BBGDOS，只有汇编，产物 `.CMD`，运行需要 `firmware` 指向软驱一号的 ROM 且 `diskImage` 指向带 DOS 的系统盘镜像，磁盘镜像建在它上面）与 `rom`（整机 ROM 工程：各机型的 BIOS 与固件源码，只有汇编，产物 `.bin` 是整块固件，机型、底固件与软盘镜像写在 `fcdev.json` 里，没有新建项目模板）。本 Skill 面向“用 FC Dev 写目标机的程序”，不涉及修改 FC Dev 自己的源码。

## 必须先读取的参考文件

| 任务 | 必须先读取 |
| --- | --- |
| 新建项目、写 `fcdev.json`、构建、产物位置、构建报错 | [build-and-config.md](references/build-and-config.md) |
| 写 NES 程序 | [nes-development.md](references/nes-development.md) |
| 写 SB2000 程序（程序格式、内存布局、启动代码、C 运行库、换页） | [sb2000-development.md](references/sb2000-development.md) |
| 在 SB2000 上做文件、参数、键盘手柄鼠标、页与窗口、中文显示、出错处理等具体功能 | [sb2000-recipes.md](references/sb2000-recipes.md) |
| 运行与核对程序 | [run-and-verify.md](references/run-and-verify.md)，再读 skill `ember-emulator` |
| 参考文件没覆盖的细节（系统调用参数、硬件行为、构建规则） | [doc-index.md](references/doc-index.md)，再用 `fcdev_docs` 读使用文档 |

任务同时涉及多项时全部读取。结论只取自这些文件、`fcdev_docs`、`fcdev_api` 与工具的返回值；没有在模拟器里验证过的内容不写成事实。

## 任务判定

先在下表选一行，再走对应的流程。表里没有的任务先问用户。

| 情形 | 流程 |
| --- | --- |
| 要新建 NES、SB2000、SB97、裕兴或步步高程序，目录里还没有 `fcdev.json` | 流程 1 |
| 要在已有的 FC Dev 项目里写或改程序 | 流程 2 |
| 只要构建（配置了 `diskName` 的学习机目标会同时生成磁盘镜像） | 流程 3 |
| 要运行、看画面、调试、核对结果 | 流程 4 |
| 构建失败、运行不对、找不到命令 | 流程 5 |
| 项目里没有 `fcdev.json`，或目标不是 `nes`、`sb2000`、`sb97`、`yuxing`、`bbg`、`rom`（如直接用 ca65 与 ld65 构建的整机程序） | 不用 `fcdev_build`：构建命令取自项目的 README 或构建脚本，在终端运行；运行与验证走 skill `ember-emulator` |

## 流程 1：新建项目

1. 确认目标与语言：用户的话里要有明确的 `nes` 或 `sb2000`（小霸王学习机就是 `sb2000`），以及 `c` 或 `asm`。缺哪项问哪项，一次问全，不猜。
2. 确认项目目录：用户没给就问；目录必须为空（允许已有 `.git`）。
3. `fcdev_new_project`，参数 `directory`、`target`、`language`。
4. `fcdev_build`，`path` 为 `<目录>/src`，确认模板能构建。完成标准：返回产物路径与大小。
5. 汇报生成的文件、产物路径，以及下一步（流程 2 或流程 4）。

## 流程 2：编写与修改程序

1. 读项目的 `fcdev.json`，确认 `target`、`name`、`firmware`、`targetOptions`；需要头文件目录时用 `fcdev_info`。
2. 按目标读参考文件（见上表）。
3. 写代码前查接口：
   - SB2000：`fcdev_api` 不给 `query` 列出全部模块；`query` 给函数名、模块名、`$7B` 或 `$7B/$09` 查详细文档。没有封装的功能按 [sb2000-recipes.md](references/sb2000-recipes.md) 的“没有示例的情况下怎么做”。
   - NES：读目标的 `fcnes.h` 与 cc65 的 `nes.h`，头文件位置用 `fcdev_info` 取得。
   - 需要系统调用参数、硬件寄存器的完整细节：`fcdev_docs`，`query` 给 `syscall` 或 `hardware`。
4. 写或改代码，遵守本文的“通用规则”。
5. `fcdev_build`。有错误时：只看第一条错误，改正后重新构建；同一条错误改两次仍不过，停下，汇报错误原文与已尝试的改法。
6. 构建通过后做流程 4；做不了流程 4 时，汇报“已构建，未运行验证”及原因。

## 流程 3：构建

1. `fcdev_build`：`path` 取项目的 `src` 目录，要构建单个文件时给文件路径；学习机目标（`sb2000`、`sb97`、`yuxing`）在项目 `fcdev.json` 配置了 `diskName` 时同时生成磁盘镜像，没有配置不生成；`bbg` 还要 `fcdev.json` 里有 `diskImage` 才生成。
2. 完成标准：返回产物路径、大小与重新编译的文件数；磁盘镜像的路径取自输出，不猜扩展名。
3. 汇报时把工具输出里的错误行原样给用户，不改写。

## 流程 4：运行与核对

1. 判断能否运行：
   - NES：直接可以。
   - SB2000：`fcdev.json` 要有 `firmware` 且文件存在；没有就停在构建通过，汇报“未运行验证：缺少 BIOS”，请用户提供路径。
2. Ember 是否可用以工具返回为准，不自行检查（规则见指令“6502 工具使用规则”）。
3. 只想让用户看到运行效果：`fcdev_run`，不给 `debug`，窗口留给用户。
4. 要自动核对：读 [run-and-verify.md](references/run-and-verify.md)，按它的清单做；每一步的工具用法读 skill `ember-emulator`。
5. 汇报写明：用了哪些工具、核对了什么（画面、内存、寄存器）、结果。没有跑过的部分写“未运行验证”。

## 流程 5：排查问题

| 现象 | 先做 |
| --- | --- |
| 报“找不到工具链”或“工具链不完整” | 汇报给用户：扩展目录里的工具链缺失，需要重新安装扩展；见 [build-and-config.md](references/build-and-config.md) |
| 报“还没有设置 Ember 的位置”或“找不到 Ember” | 按 skill `ember-emulator` 的 `references/errors.md` 汇报给用户 |
| `ld65` 报内存区放不下 | NES：程序超过 16K PRG 或 RAM，需要精简；SB2000：超过 15K，考虑 `targetOptions.banks`，读 [sb2000-development.md](references/sb2000-development.md) 的“大程序” |
| 缺少或重复 `main` | 检查目录里是否混入别的程序的源文件，汇编是否有 `.export _main` |
| SB2000 程序在 DOS 里“找不到命令” | 程序名与内部命令重名（如 `COPY`、`TYPE`、`DIR`），或镜像里没有该文件，重新 `fcdev_build`（要用镜像时确认 `fcdev.json` 里配置了 `diskName`） |
| SB2000 程序运行后屏幕一闪而过 | 返回 DOS 会清屏，退出前先等键，见 [sb2000-recipes.md](references/sb2000-recipes.md) |
| 运行 SB2000 报缺固件 | `fcdev.json` 没写 `firmware` |
| 怀疑增量缓存不一致 | 请用户确认后删除项目的 `build/` 目录重新构建 |

## 工具速查

| 要做的事 | 智能体用 |
| --- | --- |
| 新建项目 | `fcdev_new_project` |
| 构建（配置了 `diskName` 的学习机目标同时生成磁盘镜像） | `fcdev_build` |
| 只运行 | `fcdev_run` |
| 运行并调试 | `ember-emulator` skill 里的 `emu_*` 工具 |
| 反汇编 | `disasm_*` 工具，skill `disassemble-6502` |
| 项目信息 | `fcdev_info` |
| SB2000 的 C 接口 | `fcdev_api` |
| 使用文档 | `fcdev_docs` |

## 通用规则

- 入口是 `main`；汇编写 `.export _main`。不要设置入口文件。
- 一个目录是一个程序：目录里放多个独立的 `main` 会链接失败，多个单文件程序只能逐个指定文件构建。
- C 语言按 cc65 的 C89 写法：变量声明放在块的开头；cc65 不支持 `float`、`double`、`long long`。6502 只有 8 位寄存器与 256 字节硬件栈，另有 C 软件栈，局部变量、参数与递归深度都要小。
- 汇编用 ca65 语法；C 函数符号带前导下划线（`_sb_puts`），汇编里导入导出时也要带。
- SB2000 的程序名必须符合 DOS 8.3 规则，不能与 DOS 内部命令重名；超出规则扩展直接报错，不会截断。NES 的程序名不限制。
- 不把 BIOS、ROM 提交进仓库，也不为了运行而发布它们；SB2000 的 BIOS 由用户提供，路径写进 `fcdev.json` 的 `firmware`。
- 构建产物与中间文件在 `build/`，已被模板的 `.gitignore` 忽略，不要手工修改。
- 构建只用扩展自带的 cc65，没有指向自备 cc65 的配置；Ember 的位置只能由用户在设置里指定，智能体不能代填。

## 完成检查

- 目标与语言来自用户的明确说法，没有猜。
- 接口来自 `fcdev_api`、头文件或 `fcdev_docs`，没有凭记忆写函数名与参数。
- 构建结果来自 `fcdev_build` 的返回；运行结果来自模拟器的画面、内存或寄存器；没跑过的写了“未运行验证”和原因。
