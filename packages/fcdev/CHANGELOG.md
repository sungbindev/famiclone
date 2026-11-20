# 更新记录

适用于 `fcdev`、`create-fcdev`、`fcdev-cc65` 三个包，它们的版本号相同。

## 未发布

### 模拟器与调试

- 模拟器 Ember 随 `fcdev` 一起安装（Windows 64 位），`fcdev run`、`fcdev debug` 不用再下载模拟器、不用设置路径；Ember 自带升级，全机共用一份；`fcdev run --debug` 启动的 Ember 不会在调试中途自行升级。
- Ember 的调试协议版本与 `fcdev` 不匹配时，连接会提示升级 `fcdev`。
- 调试 SDK 并入 `fcdev`，不再有单独的 `ember-debug-sdk` 包，不用另外安装；终端客户端用 `fcdev debug` 运行；原来随 SDK 提供的 MCP 服务（`ember-debug-mcp`）取消。
- 需要 Node.js 22.12 及以上。

### 反汇编

- `fcdev disasm` 生成完整的 ca65 工程（工程文件、源码与链接配置）：调用、跳转、数据、变量都自动起名，并用 `ca65`、`ld65` 回读校验，与原映像逐字节一致。
- `--format` 取 `ines` 或 `flat`。
- 指令轨迹里不在映像里的地址会被忽略，并给出警告。
- 新增供 VS Code 扩展里 Copilot 工具使用的反汇编与模拟器操作接口；运行记录（指令轨迹、调用记录、CDL）可以导入反汇编工程，让执行过的地址成为入口、只被当数据读取的字节成为数据区域。

## 0.8.0（2026-11-18）

### 小霸王学习机 SB2000

- 源文件里可以直接写汉字：用 UTF-8 保存，构建时自动转成学习机用的 GB2312，不再需要手写 `\xC4\xE3` 这样的转义。汉字要写在 `.c`、`.s` 文件里，不在 GB2312 里的字符会报错并指出位置。
- `npm create fcdev` 的 SB2000 模板改成直接写汉字。

### 文档

- 新增“SB2000 符号参考”（`npx fcdev docs symbols`）：列出功能号、子功能号、系统变量地址、出错码、硬件寄存器与位、运行库入口的汇编名、C 名、值与说明。
- 新增“SB2000 示例程序源码”（`npx fcdev docs examples`）：手册里提到的 11 个示例程序的程序名与完整源码。
- 手册里指向开发文档的引用去掉了，每篇都能独立读懂。
- Ember 的帮助文档里新增“fcdev 开发手册”，可以直接阅读这些手册、C 接口参考与符号参考。
- 随包发布的文档只保留使用手册；架构设计、目标规范、测试、发布等开发文档不再随包发布，`fcdev docs` 的主题相应减少，文件名重新编号。

## 0.7.0（2026-11-15）

### 模拟器与调试

- `fcdev run` 与 `fcdev debug` 改用新模拟器 Ember：用 `fcdev.json` 的 `emulator` 或环境变量 `FCDEV_EMULATOR` 指向 Ember 的可执行文件，不再支持旧模拟器。
- `fcdev debug` 的参数改为调试终端客户端 `ember-debug` 的命令，如 `debug call runtime.status`、`debug screenshot out.png`，不再支持旧模拟器的单行命令。
- `fcdev run --debug` 用独立的用户数据目录启动 Ember，可以和平时在用的 Ember 同时运行，程序载入后停在开机状态，等待 `debug` 命令。
- `fcdev` 依赖调试 SDK `ember-debug-sdk`，安装时自动带上。
- 学习机目标（SB2000）的程序可以用 `fcdev run` 在 Ember 里运行：在 `fcdev.json` 里用 `firmware` 指定 BIOS 文件，程序会放进软盘镜像、开机后自动在 SB-DOS 里输入程序名运行；加 `--debug` 则停在开机状态，由 `debug` 命令继续。

### 反汇编

- `fcdev disasm --trace` 改为读 Ember 记录的指令轨迹，不再读旧模拟器的轨迹。

## 0.6.0（2026-11-15）

### 安装

- cc65 工具链包改名为 `fcdev-cc65`，不再放在 `@rujianjs` 作用域下；`fcdev` 会自动安装它，使用者无须任何操作。

### 模拟器与调试

- 不再提供模拟器包 `@rujianjs/fcdev-emulator`：`fcdev run` 与 `fcdev debug` 需要自备模拟器，用 `fcdev.json` 的 `emulator` 或环境变量 `FCDEV_EMULATOR` 指定。

## 0.5.0（2026-11-13）

### 小霸王学习机 SB2000

- 新增直接操作硬件的函数：通用 DMA、图形端口（显存、调色板、精灵）和硬件定时器，程序不用再自己写寄存器。
- 新增硬件寄存器与位的符号，C 里以 `SB_HW_` 开头，汇编里以 `HW_` 开头。
- C 接口参考扩充到 204 个函数，用 `npx fcdev api dma`、`npx fcdev api ppu`、`npx fcdev api timer` 查询新增的三个模块。
- 硬件说明按真机实测更新：图形端口、DMA 和定时器的行为，并列出真机与模拟器仍有差异的读数。

### 反汇编

- 识别地址表：`LDA 表,X` 加间接跳转、`PHA`/`RTS`、`JMP (地址)` 指向的表，表里的地址作为新入口继续追踪。

### 模拟器与调试

- `fcdev debug` 的 `tape play`、`tape rec` 命令里的文件路径按终端当前目录解析。
- 模拟器包带上新版模拟器，改动见模拟器的更新记录。
- 命令的提示、错误说明和文档里的示例统一写成 `npx fcdev …`，终端里直接输入 `fcdev` 会找不到命令的原因也写进了使用说明。
