# fcdev

6502 兼容机（NES、小霸王学习机等）的开发工具：用 cc65 把 C 或 ca65 汇编构建成机型对应的程序，带调试用模拟器，并能把已有程序反汇编回 ca65 源码。首批支持 NES 与小霸王学习机 SB2000，更多机型按“目标”扩展。

通常不需要手动安装：用 `npm create fcdev@latest` 创建项目时会自动装好。

## 安装

```bash
npm install --save-dev fcdev
```

要求 Node.js 22 及以上。cc65 编译器与模拟器分别在 `@rujianjs/fcdev-cc65`、`@rujianjs/fcdev-emulator` 两个依赖包里，npm 会自动安装；也可以用环境变量 `FCDEV_CC65_HOME` 指向自备的 cc65 目录，用 `FCDEV_EMULATOR` 指向自备的模拟器，但调试命令需要该模拟器支持同样的调试服务。

## 使用

```bash
fcdev build            # 构建当前目录；目录里的全部 .c、.s、.asm 链接成一个程序
fcdev build src        # 构建目录 src
fcdev build src/a.c    # 只构建这个文件，它自己是一个程序
fcdev run src          # 构建后在模拟器里运行
fcdev disk src         # 构建后把程序放进磁盘镜像（只有声明了磁盘能力的目标）
fcdev disasm game.nes  # 反汇编成 ca65 源码，并回读校验
fcdev sync             # 生成 IDE 的 C/C++ 配置
fcdev info --json      # 显示目标、工具链与头文件目录
fcdev api sb_file_open # 查询目标的 C 接口文档（函数名、模块名、$功能号或关键词；不带词列出全部模块）
fcdev docs syscall     # 查开发文档的本机路径（主题名或关键词；不带词列出全部主题）
```

入口是 `main`（汇编程序写 `.export _main`）。产物放在项目根的 `build/<目标>/`，中间文件在 `build/obj/`。

### 调试

```bash
fcdev run --debug --port 5688 src   # 以调试模式启动模拟器，只在收到 run 命令时运行
FCDEV_DEBUG_PORT=5688 fcdev debug run 60
FCDEV_DEBUG_PORT=5688 fcdev debug shot screen.png
```

`debug` 把命令原样转发给模拟器的调试服务，命令表见模拟器的说明文档。调试端口默认 5699。

### 反汇编

```bash
fcdev disasm game.nes                                   # NES（NROM），入口取自中断向量
fcdev disasm prog.bin --base 8000 --entry 8008          # 平铺二进制：加载地址与入口
fcdev disasm game.nes --cdl game.cdl --symbols sym.txt  # 加 CDL 与符号文件
```

用 `da65` 生成源码，再用 `ca65`、`ld65` 重新汇编，与原映像逐字节比较，不一致就报错。只追得到的代码才会按指令输出，其余按数据输出；用 `--cdl`（FCEUX、Mesen 格式）或 `--trace`（模拟器 `trace` 命令的日志）补充运行时才执行到的代码。

## 配置

项目根的 `fcdev.json`，未知的配置项会报错：

| 项 | 默认值 | 说明 |
| --- | --- | --- |
| `target` | 无，必填 | 目标名：`nes` 或 `sb2000` |
| `name` | 目录名 | 目录构建时的程序名；SB2000 要符合 DOS 8.3 规则 |
| `out` | `build` | 输出目录 |
| `optimize` | `true` | 给 cc65 加优化选项 |
| `define` | 空 | 预定义宏，值为字符串或数字 |
| `include` | 空 | 额外的头文件目录 |
| `toolchain` | 空 | 自备 cc65 的根目录，优先于工具链包 |
| `firmware` | 无 | 固件（BIOS）文件；SB2000 运行时必填，fcdev 不提供任何 BIOS 或 ROM |
| `emulator` | 空 | 自备模拟器的路径，优先于模拟器包 |
| `targetOptions` | 空 | 目标专用选项，如 NES 的 `font`（是否链接 ASCII 字库，默认 `true`）、SB2000 的 `banks`（大程序的库页个数，默认 0） |

## 目标

| 目标 | 产物 | 头文件 | 说明 |
| --- | --- | --- | --- |
| `nes` | `.nes` | `nes.h`（cc65）、`fcnes.h` | NROM，16K PRG 加 8K CHR；`fcnes.h` 提供调色板、文字显示与等待垂直消隐 |
| `sb2000` | `.EXE` | `sb2000.h` | 程序从 `$8000` 起装入，默认最大 16K，设 `targetOptions.banks` 后可换页做到约 468K；C 栈在窗口顶端；运行库把 DOS 的系统调用和硬件寄存器封装成 200 多个 C 函数（文字、键盘手柄鼠标、文件目录、软驱、显示、声音、数学、打印机、输入法，以及 DMA、图形端口、定时器等硬件操作），用 `fcdev api` 查询，整本文档见 `targets/sb2000/api-reference.md` |

## 文档

完整的使用手册与开发文档随本包发布，装好后在 `node_modules/fcdev/docs/`。终端里运行 `fcdev docs` 列出全部主题，`fcdev docs <主题>` 给出该文档的本机路径。更新记录在本包的 `CHANGELOG.md`。

| 想做什么 | 主题 | 文档 |
| --- | --- | --- |
| 查命令、`fcdev.json` 配置与构建流程 | `build` | `docs/02-命令行与构建.md` |
| 开发 NES 程序 | `nes` | `docs/nes/01-NES目标.md` |
| 开发 SB2000（SB-DOS）程序 | `sb2000` | `docs/sb2000/01-开发指南.md` |
| 查 SB-DOS 系统调用与出错码 | `syscall` | `docs/sb2000/02-系统调用手册.md` |
| 查 SB2000 的硬件行为 | `hardware` | `docs/sb2000/03-硬件说明.md` |
| 查 SB2000 运行库的 C 函数 | `api` | `targets/sb2000/api-reference.md`（也可以用 `fcdev api` 查询） |
| 在模拟器里运行与调试，查调试命令 | `emulator` | `docs/05-模拟器与调试.md` |
| 反汇编已有程序 | `disasm` | `docs/06-反汇编.md` |
| 创建项目与模板 | `create` | `docs/07-创建包与模板.md` |
| 新增机型的目标 | `targets` | `docs/03-目标规范.md` |

## 限制

- 反汇编目前只支持 NROM 与平铺二进制；静态追踪只追到经地址表分发的间接跳转，其他间接调用的代码按数据输出，回读仍然一致。
- SB2000 程序的退出码被忽略，`main` 返回后总是回到 DOS。
- `run` 在非调试模式下沿用模拟器的单实例规则：已有普通的模拟器窗口时，会把程序交给它打开。

## 许可证

Apache-2.0。cc65 部分为 zlib 许可，模拟器为 GPL-3.0，各自随工具链包与模拟器包附带许可文件。
