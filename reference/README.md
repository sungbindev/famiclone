# 参考资料

反汇编、验证和测试用到的原版文件，入库保留。开发中需要的机型资料先查下面的索引。

## 机型资料索引

| 机型 | 工程源码与研究文档（`machines/`） | 原版资料（`reference/`） |
| --- | --- | --- |
| 小霸王 SB2000 | `machines/sb2000/`：BIOS 源码工程、`docs/NES兼容模式研究.md`、`assets/` 里的 BIOS 切片 | `study_roms/小霸王/sb2000_bios.nes`（整机 BIOS）、`floppy-images/sb2000/`（官方 22 张软盘）、`nes-games/`（测试游戏） |
| 小霸王 SB97 | `machines/sb97/`：`disassembly/` 反汇编工程、`docs/README.md` | `study_roms/小霸王/sb97.nes`（整机 ROM）、`floppy-images/sb97/`（官方软盘） |
| 步步高 BBG | `machines/bbg/`：BIOS 源码工程、`docs/计划与进度.md`、`docs/DOS反汇编与开发库计划.md`（BBGDOS.SYS 与 COMMAND.CMD 的反汇编及开发库计划） | `docs/BBGDOS源代码/`（`Bios.asm`、`Command.asm`、`Smcdos.asm` 原厂源码，`Bbgdos.sys`、`Command.sys`、`Smdos.sys` 二进制）、`docs/步步高多媒体学生电脑 BIOS系统 DOS系统使用手册（可搜索）.pdf`（系统调用手册）、`study_roms/步步高/`（`bbg1_bios.nes`、`bbg98_bios.nes`）、`floppy-images/bbg/`（系统盘与游戏） |
| 金字塔 PEC-9588 / PEC-586 / PEC-686F | `machines/pyramid/`：`disassembly/` 反汇编工程、`README.md` | `study_roms/金字塔/`（卡带映像与修改版模拟器） |
| 裕兴 | 程序资料在 `vscode/docs/yuxing/`（无机型工程） | `study_roms/裕兴/`（V8.0-C、V8.2-D、V8.3-D、V9.0-98、V9.2-98、V9.2-F 的 NES 文件）、`floppy-images/裕兴/`（学习软件、工具软件、游戏软件） |
| 普里奇 | 无 | `study_roms/普里奇/`、`tapes/`（声像磁带） |
| 通用 | - | `docs/SH6578 8-bit Educational Computer Chipset.pdf`（SH6578 芯片手册）、`docs/nes-games/`（游戏分类） |

## 参考文件说明

| 目录 | 内容 |
| --- | --- |
| `study_roms/` | `SB2K_BIOS.NES`：SB2000 的 BIOS（512KB，含 16 字节 iNES 文件头），整个反汇编工程的对象，`machines/sb2000/` 用 fcdev 构建的结果与它逐字节一致；`bbg1_bios.nes`（128KB）、`bbg98_bios.nes`（2MB）：小霸王其他版本的 BIOS，没有做分析 |
| `floppy-images/sb2000/` | 小霸王官方全部 22 张软盘（`FLPPY0`-`FLPPY21`），都是 1.44MB 的原始 `.img`，模拟器可直接挂载。`FLPPY1` 是 WPS 文字处理系统，`FLPPY18`-`FLPPY21` 是游戏集锦 |
| `floppy-images/bbg/` | 步步高（BBG）的软盘镜像，由 `machines/bbg` 使用 |
| `floppy-dos.img` | DOS 开机镜像：1.44MB 的 FAT12，不含 `AUTOEXEC.BAT`，开机停在 DOS 提示符，里面有 `HELLO.TXT`。 |
| `nes-games/` | 6 个 NES 游戏，用于 [NES 兼容模式研究](../machines/sb2000/docs/NES兼容模式研究.md) 里 `play` 程序的测试 |

## 官方软盘的内容

- `FLPPY1`（WPS 文字处理系统）是标准 FAT12，含 DOS 工具、`SB-WIN.EXE`、`WPS.EXE`、输入法模块 `WBX.BIN`/`BXM.BIN`/`RZM.BIN`；`AUTOEXEC.BAT` 依次执行 `install wbx` 与 `sb-win.exe`，启动后进入 SB-Win98 图形环境并自动进入 “SUBOR-WPS 2.00” 文字处理主菜单。
- 游戏集锦软盘（如 `FLPPY20`，“游戏集锦(027)”）只有 `GAME.EXE`（3456 字节）与 `AUTOEXEC.BAT`（内容 `game`），启动后显示游戏集锦菜单；游戏数据不在文件系统里，装载方式见 [NES 兼容模式研究](../machines/sb2000/docs/NES兼容模式研究.md) 第 3 节。
