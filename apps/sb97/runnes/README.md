# runnes（小霸王 97）

在小霸王 97 学习机（SB97）的 DOS 下运行 NES 游戏，只支持 mapper 0 的游戏。用汇编（ca65）开发，不依赖 fcdev 的 C 运行库。

## 使用

把 `NES.EXE` 与游戏文件（文件名符合 8.3，如 `MARIO.NES`）放在同一张软盘上，在 DOS 提示符下输入：

```text
NES MARIO
NES MARIO.NES
```

两种写法等价：参数没有扩展名时按 `.NES` 处理。进入游戏后按手柄的 Start 开始。

| 支持 | 不支持 |
| --- | --- |
| mapper 0，PRG 16K 或 32K，CHR 8K，水平或垂直镜像 | 其他 mapper，带 trainer 的文件，PRG 超过 32K 或 CHR 超过 8K，没有 CHR 的文件（CHR-RAM，未实测） |

游戏里不能回到 DOS，只能硬复位。出错时画面第 10 行显示英文提示并停住，需要重新开机：

| 提示 | 原因 |
| --- | --- |
| `Usage: NES FILE.NES` | 没有给文件名 |
| `Bad file name` | 文件名超过 8 个字符的主名或 3 个字符的扩展名 |
| `Disk read error` | 软驱读盘出错 |
| `File not found` | 软盘根目录里没有这个文件 |
| `Not an iNES file` | 文件头不是 iNES 格式 |
| `Mapper not supported` | mapper 不是 0 |
| `Size not supported` | PRG 或 CHR 大小超出支持范围 |
| `File is truncated` | 文件比头里写的大小短 |
| `Trainer not supported` | 文件头带 trainer |

## 构建

```text
node tools/build.mjs [游戏文件.nes]
```

- 用仓库里的 cc65（`build/cc65/bin`）汇编、链接，产物是 `build/NES.EXE`（32768 字节）。
- 同时生成测试软盘 `build/RUNNES.ima`（1.44MB FAT12），里面有 `NES.EXE` 与游戏文件；没有给游戏文件时用 `reference/mario.nes`。
- `tools/make-disk.mjs` 单独生成软盘：`node tools/make-disk.mjs <输出.ima> <文件>...`。

`build/` 不入库。

## 在模拟器里运行

用 Ember 的 SB97 机型，ROM 为 `ember/roms/sb97.bin`，软驱挂载 `build/RUNNES.ima`，开机后按任意键，等菜单出现后按左键把焦点移到 DOS，回车进入 DOS，再输入 `NES MARIO`。

## 原理

- SB97 的 DOS 运行的是 32K 映像：映射在 `$8000-$FFFF`，偏移 `$7FF8`、`$7FF9` 是签名 `AA 55`，`$FFFC` 是入口。`NES.EXE` 的向量区在 `src/nes.s` 的 `VECTORS` 段。
- DOS 把第一个参数写在 `$680E`（零结尾，大写）。程序启动后先把参数复制到零页。
- 装载程序（`LOADER` 段）复制到工作 RAM `$6000` 运行，之后切换程序 RAM 时执行的代码不会丢失。
- 软驱用 1.44MB 的布局直接读：根目录在扇区 19-32，FAT 在扇区 1-18，数据从扇区 33 开始。读命令与状态判断和 SB97 BIOS 保持一致。
- PRG 写入程序 RAM 的第 1 个 32K 页（`$5000=$81`），`$5200` 设为 32K 模式与镜像位。CHR 用 `$2006`、`$2007` 写入图案表。
- 工作 RAM 里的跳转桩设置 `$5000`、`$5200` 后执行 `JMP ($FFFC)`，进入游戏的复位向量。
- SB97 只有手柄口 1，`$4017` 接的是键盘，没按键时读到 1，游戏读手柄 2 会以为 2 号手柄一直按着全部按键（超级玛丽的标题画面要求只按开始键，因此完全不响应）。装载程序在启动前扫描 `$8000-$FFF7`，把 `LDA $4017`（`AD 17 40`）与 `LDA $4016,X`（`BD 16 40`）改成 `JSR` 到工作 RAM 里的小桩：读手柄 2 一律得到 0，读手柄 1 照常。扫描按字节匹配，数据里恰好出现同样的三个字节时会被误改。

## 文件

| 文件 | 说明 |
| --- | --- |
| `src/nes.s` | 程序：入口、装载程序、软驱与 FAT12 读取、iNES 头检查、启动游戏 |
| `src/sb97-exe.cfg` | 链接配置：32K 映像、装载程序的运行地址与缓冲 |
| `tools/build.mjs` | 构建 `NES.EXE` 与测试软盘 |
| `tools/make-disk.mjs` | 生成 FAT12 软盘镜像 |
