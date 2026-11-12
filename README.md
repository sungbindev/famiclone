# famiclone

FC 兼容机（NES、小霸王学习机等 6502 兼容机）的开发工具与机型资料：

- **fcdev**：开发工具集。用 C 或 ca65 汇编写程序，用 cc65 构建成各机型能运行的程序，自带调试用模拟器，也能把已有程序反汇编回 ca65 源码。首批支持 NES 与小霸王学习机 SB2000，以后按“目标”扩展到步步高、裕兴等机型。
- **machines/**：各机型的资料与工程，一个机型一个目录，目前有 [sb2000](machines/sb2000/README.md)（小霸王学生电脑：BIOS 反汇编与源码）、[bbk](machines/bbk/README.md)（步步高学生电脑：BIOS 与 DOS 原厂源码、汇编器与分析工具；移植到 SB2000 的 BIOS 在 apps/sb2000/bbk-bios）。
- **apps/**：用 fcdev 开发的应用程序与示例，按机型分目录，目前有 [sb2000](apps/sb2000/)（SB-DOS 的汇编器、编辑器、NES 游戏运行器、步步高 BIOS 移植版与加载器、软盘镜像脚本，以及 [示例](apps/sb2000/examples/README.md)）。

下面是 fcdev 的安装与使用。

## 安装

要求 Node.js 22 及以上。也可以自备 cc65，见 [fcdev 的说明](packages/fcdev/README.md)。

不用单独安装，创建项目时会自动装好：

```bash
npm create fcdev@latest my-game
```

按提示选择目标（`nes` 或 `sb2000`）与语言（`c` 或 `asm`），会在目录里生成示例程序、配置文件并安装依赖。

## 使用

```bash
cd my-game
npm run build     # 构建，产物在 build/<目标>/
npm start         # 在模拟器里运行
npm run disk      # 把程序放进磁盘镜像（只有 SB2000 等带磁盘的目标）
```

SB2000 的运行需要自己提供 BIOS 文件，写进项目的 `fcdev.json`：

```json
{
  "target": "sb2000",
  "name": "hello",
  "firmware": "D:/roms/SB2K_BIOS.NES"
}
```

fcdev 不提供任何 BIOS 或 ROM（`machines/sb2000/reference/` 里的参考文件不随 npm 包发布）。全部命令、配置项、调试与反汇编的用法见 [packages/fcdev/README.md](packages/fcdev/README.md)，创建项目的选项见 [packages/create-fcdev/README.md](packages/create-fcdev/README.md)。完整的使用手册与开发文档在 [packages/fcdev/docs](packages/fcdev/docs)，随 `fcdev` 包发布，在项目里运行 `npx fcdev docs` 可以列出全部主题。

## 限制

- SB2000 程序默认最大 16K，设置 `targetOptions.banks` 后可做换页的大程序（最大约 468K）；`main` 的返回值被忽略。
- 反汇编目前只支持 NROM 的 NES 与平铺二进制。
- VS Code 扩展目前有智能感知（ca65 汇编的悬停、补全、转到定义、文档符号，以及 C 配置的自动同步）与 ca65 语法高亮，任务与调试还没有做。

## 目录结构

| 目录 | 内容 |
| --- | --- |
| `packages/fcdev` | 命令行与各机型目标 |
| `packages/create-fcdev` | `npm create fcdev` 的创建包与项目模板 |
| `packages/fcdev-cc65`、`packages/fcdev-emulator` | cc65 与模拟器的包（构建生成，不入库） |
| `vscode` | VS Code 扩展（独立的 npm 项目，说明见 [vscode/README.md](vscode/README.md)） |
| `machines` | 各机型的资料与工程，一个机型一个目录，目前有 [sb2000](machines/sb2000/README.md)、[bbk](machines/bbk/README.md) |
| `apps` | 用 fcdev 开发的应用程序与示例，按机型分目录；`apps/sb2000` 下有 `asm`、`edit`、`runnes`、`bbk-bios`、`bbk-loader`、`memtest`、`fcprobe`、`disk` 与 `examples` |
| `cc65`、`virtuanes` | cc65 与模拟器的源码，在本仓库维护，各自带许可文件 |
| `tools` | 构建、检查与发布前验证脚本 |
| `packages/fcdev/docs` | 各部分的开发文档与使用手册，随主包发布；清单见 [01-软件架构设计.md](packages/fcdev/docs/01-软件架构设计.md) 第 9 节，终端里运行 `fcdev docs` 查看 |

## 开发

```bash
npm install
npm run build:libs     # 构建各目标的运行库
npm test               # 全部测试
npm run check          # 依赖边界检查加测试
npm run release:check  # 发布前检查
npm run release:verify # 打包后在临时目录里创建、安装并构建四个模板
```

构建 cc65 与模拟器的脚本、升级 cc65 的步骤见 [cc65 工具链](packages/fcdev/docs/04-cc65工具链.md) 与 [模拟器与调试](packages/fcdev/docs/05-模拟器与调试.md)。设置环境变量 `FCDEV_TEST_FIRMWARE` 指向 SB2000 的 BIOS 文件，测试会额外在模拟器里运行 SB2000 程序；设置 `FCDEV_TEST_NES_ROM` 指向一个 NROM 的 `.nes` 文件，测试会额外反汇编它。

## 许可证

仓库与 `fcdev` 自己的代码为 Apache-2.0，见 [LICENSE](LICENSE)。`cc65/` 为 zlib 许可，`virtuanes/`（模拟器）为 GPLv3，各自的许可文件在对应目录。
