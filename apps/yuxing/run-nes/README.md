# run-nes（裕兴）

在裕兴学习机的 DOS 下运行 NES 游戏，支持 mapper 0 与 mapper 4（MMC3）。用汇编（ca65）开发，是 fcdev 的 yuxing 目标项目，不依赖 C 运行库。`RUN.EXE` 只是加载程序，游戏是软盘上单独的 `.NES` 文件。

## 使用

把 `RUN.EXE` 与游戏文件（扩展名 `.NES`，文件名符合 8.3）放在同一张软盘上，在 DOS 提示符下输入：

```text
RUN.EXE
```

裕兴的 DOS 必须写扩展名，也不接受命令行参数，所以不能指定游戏名：程序运行软盘根目录里的第一个 `.NES` 文件，一张盘只放一个游戏。进入游戏后按手柄的 Start 开始。

| 学习卡 | mapper 0 | mapper 4 |
| --- | --- | --- |
| V8.0-C、V8.2-D、V8.3-D（程序 RAM 128K） | 支持 | 不支持，显示 `Mapper not supported` |
| V9.0-98（程序 RAM 1MB） | 支持 | 支持 |

| 支持 | 不支持 |
| --- | --- |
| mapper 0：PRG 16K 或 32K，CHR 8K，水平或垂直镜像 | 其他 mapper，带 trainer 的文件，没有 CHR 的文件（CHR-RAM） |
| mapper 4：PRG 不超过 480K，CHR 8K 到 128K（8K 的整数倍） | 游戏选择了超出自己 PRG 或 CHR 大小的页号（真实卡带靠地址线不接把它折回，这里没有折回，游戏读到空页） |

游戏里不能回到 DOS，只能复位。出错时用 ROM 的显示功能在画面第 10 行显示英文提示并停住，需要复位：

| 提示 | 原因 |
| --- | --- |
| `Disk read error` | 软驱读盘出错 |
| `No .NES file on the disk` | 根目录里没有 `.NES` 文件 |
| `Not an iNES file` | 文件头不是 iNES 格式 |
| `Mapper not supported` | mapper 不是 0 与 4，或程序 RAM 只有 128K 的学习卡上遇到 mapper 4 |
| `Size not supported` | PRG 或 CHR 大小超出支持范围 |
| `File is truncated` | 文件比头里写的大小短 |
| `Trainer not supported` | 文件头带 trainer |

## 构建

在 VS Code 里打开仓库，运行命令“fcdev：构建项目”选 `apps/yuxing/run-nes`（或任务“fcdev：构建”）：

- 产物是 `build/yuxing/RUN.EXE`。
- 同时生成软盘镜像 `build/yuxing/RUN.img`（1.44MB FAT12），里面有 `RUN.EXE` 与 `disk/` 目录下的全部文件：目前是使用说明 `disk/README.TXT`（UTF-8 带 BOM、CRLF）；文件名须是 8.3 名字，不能有子目录。
- 要测试游戏，把游戏文件放进 `disk/`（例如把 `reference/mario.nes` 复制成 `disk/MARIO.NES`）后重新构建；`disk/*.nes` 不入库。镜像里有多个 `.NES` 文件时，运行的是根目录里排在最前面的一个。

`build/` 不入库。

## 在模拟器里运行

设置 `fcdev.emulator.path` 指向 Ember 后，任务“fcdev：运行”会用 `fcdev.json` 里 `firmware` 指向的 `ember/roms/yuxing_v8.2-d.bin` 开机，挂载镜像，经主菜单进入 DOS 并输入 `run.exe`。这套自动步骤只适用于 V8.x 的学习卡；V9.0-98 要手动进 DOS：开机后右键选中第一项，下键两次选“学DOS”，回车，动画播完后按空格，下键两次选“三.DOS系统操作”，回车，再输入 `RUN.EXE`。

## 原理

- DOS 把文件原样装进工作 RAM `$6000` 起并从那里执行，所以程序一直运行在 `$6000` 窗口里；`RUN.EXE` 是 8K 以内的单个文件。
- DOS 给 `$6000` 窗口选的 RAM 不固定（`$5500` 位 0-1 随学习卡不同）：程序先把 `$0300` 起的一小段代码复制到内部 RAM，再逐个试 4 种取值，找出窗口开头与自己一致的那个，之后所有 `$5500` 的写入都带上这个值，保证自己不会被换走。
- 软驱用 1.44MB 的布局直接读：寄存器是 `$4200`、`$4201`、`$4205`、`$4304`、`$4305`，根目录在扇区 19-32，FAT 在扇区 1-18，数据从扇区 33 开始；在根目录找第一个扩展名是 `NES` 的文件，再沿簇链读。
- PRG 按 16K 写进程序 RAM 的第 0 页起（`$5500` 位 2 为 1 进程序 RAM 模式，`$4800` 位 5 为 1 时写 `$8000` 是页寄存器），CHR 关掉显示后经 `$2006`、`$2007` 写进字符 RAM（`$5501` 选 8K 页）。
- mapper 0：16K 的 PRG 先复制成两份，改成 32K 页（`$5500` 位 6），按头里的镜像位设 `$5500` 位 3，`$4800`、`$5501` 清零（关汉字模式），`JMP ($FFFC)`。
- mapper 4：进入 MMC3 模式（`$5501` 位 7）后，`$C000-$FFFF` 固定接程序 RAM 的 8K 页 62、63（16K 页 31），所以把 PRG 最后的 16K 复制到那里；字符页在 MMC3 模式下按“位 3-2 与位 1-0 互换”重排，游戏的第 f 个 8K 图案写进字符 RAM 的第 swap(f) 页。复制与切换的代码放在内部 RAM `$0300`，因为 MMC3 模式下 `$6000` 窗口换成了 MMC3 自己的工作 RAM。
- 程序 RAM 只有 128K 的学习卡，页寄存器只取低 3 位，选第 8 页与选第 0 页是同一页：程序在第 0 页写 0、选第 8 页写 `$5A`、再回第 0 页读，读到 `$5A` 就判定为小容量，拒绝 mapper 4。
- 手柄 2 读 `$4017`，裕兴的 `$4017` 就是手柄 2（没有键盘混入），所以不需要像 SB97 那样改游戏的指令。

## 文件

| 文件 | 说明 |
| --- | --- |
| `src/nes.s` | 程序：入口 `_main`、窗口探测、软驱与 FAT12 读取、iNES 头检查、mapper 0 与 4 的启动 |
| `fcdev.json` | fcdev 配置：目标 yuxing，程序名 run，运行用的固件 |
| `disk/` | 构建时全部放进软盘镜像的文件，目前是使用说明 `README.TXT` |
