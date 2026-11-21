# runnes

在小霸王学习机 97（sb97）的 DOS 下运行 NES 游戏。程序用 ca65 汇编，不依赖 BIOS 的 DOS 服务（进入游戏后 DOS 不可见），自带软驱与 FAT12 读取。

## 使用

把 `RUNNES.EXE` 与游戏文件（文件名要符合 8.3）放在同一张盘里，在 DOS 提示符下输入：

```text
RUNNES 1942.NES
```

读入游戏需要一会儿，之后直接进入游戏。游戏里不能回到 DOS，只能硬复位。

| 支持 | 不支持 |
| --- | --- |
| mapper 0（NROM），PRG 16K 或 32K，CHR 0 或 8K，没有 trainer，iNES 头（非 NES 2.0） | 其他 mapper，PRG 或 CHR 大小不符合上表，带 trainer，NES 2.0 头 |

出错时显示提示，按任意键经 BIOS 重新开机（DOS 不能返回）：

| 提示 | 原因 |
| --- | --- |
| `Usage: RUNNES FILE.NES` | 没有给文件名 |
| `Can't open file` | 根目录里找不到文件 |
| `Not an iNES file` | 不是 iNES 文件 |
| `Mapper not supported` | mapper 不是 0 |
| `Size not supported` | 超出上表的支持范围 |
| `File is truncated` | 文件比头里写的 PRG、CHR 大小短 |
| `Read error` | 读盘出错 |
| `Disk format not supported` | 不是 1.44MB、1 扇区一簇的 FAT12 盘 |

## 构建

```bash
node tools/build.mjs                              # 生成 build/sb97/RUNNES.EXE 与 RUNNES.img
node tools/build.mjs --disk 1942.NES CENTIP.NES   # 另把游戏文件打包进软盘镜像
```

## 文件

| 文件 | 说明 |
| --- | --- |
| `src/runnes.s` | 入口：解析命令行、加载游戏、进入 NES 模式 |
| `src/fat.s` | FAT12 查找文件、检查 iNES 头、把 PRG 写进程序 RAM、CHR 暂存到 WRAM |
| `src/fdc.s` | 软驱控制器驱动（uPD765 兼容） |
| `src/ramcode.s` | 在 RAM 里运行的换页与跳转例程 |
| `src/screen.s` | 出错提示 |
| `src/defs.inc` | 寄存器地址、内存分配、零页变量与错误码 |
| `runnes.cfg` | 链接配置：程序映像 32K，换页例程放在 $0200 |
