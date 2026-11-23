# game（步步高）

在步步高多媒体学生电脑（BBGDOS）下运行 NES 游戏，只支持 mapper 0 的游戏。用汇编（ca65）开发，是 fcdev 的 bbg 目标项目，不依赖 C 运行库。`GAME.CMD` 只是加载程序，游戏是软盘上单独的 `.NES` 文件。

## 使用

把 `GAME.CMD` 与游戏文件（文件名符合 8.3，如 `MARIO.NES`）放在同一张带 BBGDOS 的系统盘上，在 DOS 提示符下输入：

```text
GAME MARIO
GAME MARIO.NES
```

不带参数时显示游戏菜单，列出盘上的 `.NES` 文件，手柄上下键选择，按 Start 运行；带参数时直接运行指定的游戏。BBGDOS 运行 `.CMD` 程序不用写扩展名；游戏名没有扩展名时按 `.NES` 处理。进入游戏后按手柄的 Start 开始。

两种写法等价：BBGDOS 运行 `.CMD` 程序不用写扩展名；游戏名没有扩展名时按 `.NES` 处理。进入游戏后按手柄的 Start 开始。

| 支持 | 不支持 |
| --- | --- |
| mapper 0，PRG 16K 或 32K，CHR 8K，水平或垂直镜像 | 其他 mapper，带 trainer 的文件，PRG 超过 32K 或 CHR 超过 8K，没有 CHR 的文件（CHR-RAM） |

游戏里不能回到 DOS，只能复位。出错时在命令行显示英文提示并回到 DOS：

| 提示 | 原因 |
| --- | --- |
| `No .NES file on the disk` | 盘上没有 .NES 文件，菜单为空 |
| `Bad file name` | 文件名超过 8 个字符的主名或 3 个字符的扩展名 |
| `No memory` | 可用的分页内存不够 |
| `Can't open file` | 找不到文件 |
| `Read error` | 读文件出错 |
| `Not an iNES file` | 文件头不是 iNES 格式 |
| `Mapper not supported` | mapper 不是 0 |
| `Size not supported` | PRG 或 CHR 大小超出支持范围 |
| `File is truncated` | 文件比头里写的大小短 |
| `Trainer not supported` | 文件头带 trainer |

## 构建

在 VS Code 里打开仓库，运行命令“fcdev：构建项目”选 `apps/bbg/game`（或任务“fcdev：构建”）：

- 产物是 `build/bbg/GAME.CMD`。
- `fcdev.json` 的 `diskImage` 指向系统盘镜像 `reference/floppy-images/bbg/系统类/001-3.IMG`（`DOS.IMG` 已装满，放不下文件），同时生成软盘镜像 `build/bbg/GAME.img`：系统盘的全部内容加上 `GAME.CMD` 与 `disk/` 目录下的全部文件；文件名须是 8.3 名字，不能有子目录。
- 要测试游戏，把游戏文件放进 `disk/`（例如把 `reference/mario.nes` 复制成 `disk/MARIO.NES`）后重新构建；`disk/*.nes` 不入库，目录里没有入库的文件。

`build/` 不入库。

## 在模拟器里运行

设置 `fcdev.emulator.path` 指向 Ember 后，任务“fcdev：运行”会用 `fcdev.json` 里 `firmware` 指向的 `ember/roms/bbg1_bios.bin` 开机，挂载镜像，等系统盘启动后输入 `game`（不带参数，显示游戏菜单）。要运行游戏，在 `A:\>` 提示符下手动输入 `GAME MARIO`。

## 原理

- DOS 把 `GAME.CMD` 装进当前空闲页的 `$8000`（装入地址在文件头里），按手册，从这一页起所有分页内存都归程序使用。程序用 BIOS 功能 2 取得自己所在的页，PRG 读进后面两页，CHR 读进第三页。
- 命令行在 `$0100`：第一个字节是长度，`$0101` 起是参数（含程序名后的空格）。程序取第一个词、转大写，没有扩展名时加 `.NES`。
- 文件用 DOS 的文件调用（入口 `$5803`）读：带路径打开、读 16 字节头检查、再按页读 PRG 与 CHR，缓冲是页加 `$8000`。
- 最后一步不再调用 DOS：把跳板复制到内部 RAM `$0300`、小桩复制到公共 RAM `$7E00`，进入跳板。跳板关掉 NMI、显示与声音，把 `$FF01` 设成 `$C000` 起用 DRAM、不用 MMC3 与分屏（名称表连接取自头里的镜像位），1K 字符页寄存器指向字符 RAM 的 0-7 页，把 CHR 页映射到 `$8000` 写进图案表，再映射 PRG（`$FF04` 选 `$8000` 的 16K 页，`$FF24`、`$FF2C` 选 `$C000` 起的两个 8K 页，16K 的 PRG 镜像成两份），`JMP ($FFFC)`。
- 步步高只有手柄口 1，`$4017` 的位 1-4 接键盘，没按键时读到 1，游戏读手柄 2 会以为 2 号手柄一直按着全部按键（超级马里奥的标题画面要求只按开始键，因此完全不响应）。跳板在启动前扫描 PRG，把 `LDA $4017`（`AD 17 40`）与 `LDA $4016,X`（`BD 16 40`）改成 `JSR` 到公共 RAM 里的小桩：读手柄 2 一律得到 0，读手柄 1 照常。扫描按字节匹配，数据里恰好出现同样的三个字节时会被误改。
- 链接配置 `link.cfg` 与 bbg 目标的相同，另加两段存在程序里、运行在别处的代码：`TRAMP`（`$0300`）与 `STUBS`（`$7E00`）。

## 文件

| 文件 | 说明 |
| --- | --- |
| `src/nes.s` | 程序：入口 `_main`、命令行与文件名、DOS 文件调用、iNES 头检查、跳板与小桩 |
| `link.cfg` | 链接配置：bbg 目标的配置加 `TRAMP`、`STUBS` 两段 |
| `fcdev.json` | fcdev 配置：目标 bbg，程序名 run，链接配置，运行用的固件与系统盘镜像 |
| `disk/` | 构建时全部放进软盘镜像的文件，只放自己的游戏文件，不入库 |
