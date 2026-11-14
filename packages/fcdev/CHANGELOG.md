# 更新记录

适用于 `fcdev`、`create-fcdev`、`fcdev-cc65` 三个包，它们的版本号相同。

## 未发布

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
