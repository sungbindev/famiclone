# fbasic（SB2000 版）

把 Family BASIC v3.0（NS-HUBASIC V3.0）原生移植成小霸王学习机 SB2000 的 SB-DOS 程序 `FBASIC.EXE`。项目用 FC Dev 扩展构建，不用旧的任何工具。

## 构建与运行

- 构建：FC Dev 的“构建”（或 `fcdev_build`），产物在 `build/sb2000/`：`FBASIC.EXE`。要磁盘镜像时在 `fcdev.json` 里加 `diskName`。
- 运行：FC Dev 的“运行”（或 `fcdev_run`），固件取自 `fcdev.json` 的 `firmware`，需要在设置里指定 Ember 的位置。

## 目录

| 路径 | 内容 |
| --- | --- |
| `fcdev.json` | 项目配置：目标 sb2000，程序名 fbasic，固件，头文件目录 `src`，链接配置 `link.cfg` |
| `link.cfg` | 链接配置：程序文件的页布局（见下表） |
| `src/loader.s` | 加载器：程序入口 `_main`，复制 ERAM 小段，填窗口页号表，进入映像 |
| `src/stub.s` | ERAM 小段：进入映像、调用 SB-DOS 的入口门、退出映像 |
| `src/shell.inc` | 外壳共用常量：小段地址、现场交换缓冲、请求块、窗口页号表 |
| `src/fbasic.s` | 映像源码（32K，段 `IMAGE` 与 `VECTORS`） |
| `src/game.s` | 内置程序 GAME 0-3 的数据（段 `GAME`） |
| `src/chr.s` | CHR 图案表（段 `CHR`，两张共 8K） |
| `src/platform.inc`、`src/platform-code.inc`、`src/keyboard.inc`、`src/disk.inc` | 平台代码：显示、声音、手柄、键盘、磁盘 |
| `docs/程序文档.md` | 程序的启动、按键、保存与读取、限制与常见问题 |

## 程序文件页布局

| 页（文件内） | 内容 | 运行时位置 |
| --- | --- | --- |
| 0-1 | 加载器、ERAM 小段（常驻） | `$8000-$9FFF`（DOS 状态） |
| 2-9 | 整机映像 | `$8000-$FFFF`（窗口 0-7） |
| 10-11 | CHR 图案表 | 映像用 DMA 送进显存 |
| 12-14 | GAME 0-3 数据 | 运行 GAME 命令时用 DMA 搬进程序区 |

ERAM 小段运行在 `$7E00`，由加载器从程序页复制过去。

## 备注

- 旧项目的反汇编参考源码（`reference/fbasic-nes.s`）没有带入本项目：FC Dev 会编译项目内全部 `.s` 文件。它保留在旧项目的 git 历史里。
- 程序的使用说明见 [docs/程序文档.md](docs/程序文档.md)。
