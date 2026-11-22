# SB2000

小霸王学生电脑 SB2000（SB-DOS）的 BIOS 反汇编工程，以及在它之上整理出的 BIOS 源码。本目录是 famiclone 仓库里的一个机型目录（`machines/sb2000/`）。

## 目录

| 目录 | 内容 | 状态 |
| --- | --- | --- |
| [bios/](bios/) | 正式 BIOS 源码工程：可构建、可修改、可在模拟器里调试（自写汇编器构建，与原始 ROM 逐字节一致，不转 ca65） | 进行中（阶段 S2） |
| [docs/](docs/) | 研究文档：[NES 兼容模式研究](docs/NES兼容模式研究.md) | |
| [reference/](reference/) | 原版 ROM、官方软盘、NES 游戏 | |
| [apps/sb2000/](../../apps/sb2000/) | 用 fcdev 开发的 SB-DOS 应用程序，放在仓库根目录的 apps/sb2000/ 下，不在本目录里；目前有 [runnes](../../apps/sb2000/runnes/)（在 DOS 下运行 NES 游戏）、[asm](../../apps/sb2000/asm/)（6502 汇编器，生成 SB-DOS 的 EXE）、[edit](../../apps/sb2000/edit/)（文本编辑器，F5 调用汇编器）、[memtest](../../apps/sb2000/memtest/)（在真机上实测内存大小的探测程序）、[hwtest](../../apps/sb2000/hwtest/)（在真机上实测硬件行为的测试程序）、[hwprobe](../../apps/sb2000/hwprobe/)（补测 hwtest 没弄清的读回）、[family-basic-v3](../../apps/sb2000/family-basic-v3/)（Family BASIC v3 的 SB2000 移植版）、[examples](../../apps/sb2000/examples/)（示例程序） | |

## 环境

- Windows，Node.js 22 及以上（脚本无第三方依赖）。
- 编译模拟器需要 Visual Studio（含“使用 C++ 的桌面开发”）。

## 快速开始

```powershell
cd machines/sb2000/bios      # 在仓库根目录运行
npm run build                # 构建 BIOS 并与原始 ROM 逐字节比对
```

应用开发用的 fcdev、系统调用手册与示例在仓库根目录的 [vscode/](../../vscode/)（fcdev 扩展）与 [apps/sb2000/examples/](../../apps/sb2000/examples/)。
