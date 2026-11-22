# {{name}}

用 fcdev 扩展开发的小霸王 97 学习机（SB97 DOS）程序，ca65 汇编。

## 构建

在 VS Code 里运行“终端 → 运行生成任务”（默认任务“fcdev：构建”），生成 `build/sb97/HELLO.EXE`，同时生成软盘镜像 `build/sb97/HELLO.img`，里面有 HELLO.EXE 和项目 `disk` 目录下的全部文件（模板里是一份使用说明 `disk/README.TXT`，名字与内容可以自己改，文件名必须是 8.3 名字，不能有子目录）。不想生成镜像时，在 `fcdev.json` 里加 `"disk": false`。

程序名必须符合 DOS 8.3 规则（主名不超过 8 个字符），在 `fcdev.json` 的 `name` 里修改。

## 在模拟器里运行（只支持 Windows）

扩展不提供模拟器与 ROM。先在设置 `fcdev.emulator.path` 里指定模拟器 Ember 的位置，再在 `fcdev.json` 里用 `firmware` 指定 SB97 的 ROM 文件：

```json
{ "target": "sb97", "name": "hello", "firmware": "D:/rom/sb97.bin" }
```

然后运行任务“fcdev：运行”：Ember 自动开机、进入 DOS 并输入 `hello`。

要在真机或其他模拟器里运行，把软盘镜像挂载到软驱，进入 DOS，在提示符下输入 `hello` 并回车。

## 程序格式

- DOS 运行的是 32K 映像（32768 字节），映射在 `$8000-$FFFF`；构建时自动补齐到这个大小，超出时链接报错。
- 映像偏移 `$7FF8`、`$7FF9` 必须是签名 `AA 55`，`$FFFA` 起是 NMI、复位、IRQ 向量；这些由目标自带的启动代码提供，源文件不要再定义 `VECTORS` 段。
- 程序的入口是导出的 `_main`，由启动代码调用；DOS 用 `JMP` 启动程序，不能返回 DOS，`_main` 返回后程序停住，要退出只能复位。
- `target.json` 之外的段：`CODE`、`RODATA`、`ZEROPAGE`、`BSS`（内部 RAM），`RAMCODE`（存在映像里，运行在工作 RAM `$6000` 起的 3K，需要自己复制），`WRAM`（工作 RAM `$6C00` 起的缓冲）。
- 硬件寄存器与 DOS 约定的地址在 `sb97.inc`，用 `.include "sb97.inc"` 引入。

## 文件

| 文件 | 说明 |
| --- | --- |
| `src/hello.s` | 示例程序：直接写 PPU，显示两行英文并停住，入口是导出的 `_main` |
| `fcdev.json` | fcdev 配置：目标 sb97，程序名 hello |
| `disk/README.TXT` | 放进磁盘镜像的使用说明示例，用 WinImage 等软件打开镜像时可直接查看；`disk` 目录下的文件都会放进镜像 |
| `.vscode/tasks.json` | 构建与运行的任务 |
