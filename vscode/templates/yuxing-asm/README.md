# {{name}}

用 FC Dev 扩展开发的裕兴学习机（Yuxing DOS）程序，ca65 汇编。

## 构建

在 VS Code 里运行“终端 → 运行生成任务”（默认任务“FCDEV：构建”），生成 `build/yuxing/HELLO.EXE`。要生成软盘镜像，在 `fcdev.json` 里加 `diskName`（如 `"diskName": "yuxing-hello.img"`），镜像生成在 `build/yuxing/` 下，里面有 HELLO.EXE 和项目 `disk` 目录下的全部文件（模板里是一份使用说明 `disk/README.TXT`，名字与内容可以自己改，文件名必须是 8.3 名字，不能有子目录）；没有 `diskName` 时不生成镜像。

程序名必须符合 DOS 8.3 规则（主名不超过 8 个字符），在 `fcdev.json` 的 `name` 里修改。

## 在模拟器里运行（只支持 Windows）

扩展不提供模拟器与 ROM。先在设置 `fcdev.emulator.path` 里指定模拟器 Ember 的位置，再在 `fcdev.json` 里用 `firmware` 指定裕兴学习卡的 ROM 文件（V8.0-C、V8.2-D、V8.3-D、V9.0-98 任选一个；目前只用 V8.2-D 验证过）：

```json
{ "target": "yuxing", "name": "hello", "firmware": "D:/rom/yuxing_v8.2-d.rom" }
```

然后运行任务“FCDEV：运行”：Ember 自动开机，经开机画面和主菜单进入“学 DOS”里的 DOS 命令行，输入 `hello.exe`。

要在真机或其他模拟器里运行，把软盘镜像挂载到软驱，进入 DOS，在提示符下输入 `hello.exe` 并回车（裕兴的 DOS 必须写扩展名，也不接受命令行参数）。

## 程序格式

- DOS 把文件原样装进工作 RAM `$6000` 起，再从 `$6000` 开始执行；文件没有头与签名，大小不超过 8192 字节（`$6000-$7FFF`）。
- 文件的第一条指令必须是 `JMP`，由目标自带的启动代码提供；程序的入口是导出的 `_main`，由启动代码调用。
- 程序不能返回 DOS，`_main` 返回后程序停住，要退出只能复位。
- 段：`CODE`、`RODATA`、`DATA`（都在程序所在的 RAM 里，可直接读写）、`ZEROPAGE`、`BSS`（内部 RAM `$0400` 起的 1K）。
- ROM 的显示功能与字符串控制码在 `yuxing.inc`，用 `.include "yuxing.inc"` 引入。

## 文件

| 文件 | 说明 |
| --- | --- |
| `src/hello.s` | 示例程序：用 ROM 的显示功能显示两行英文并停住，入口是导出的 `_main` |
| `fcdev.json` | FC Dev 配置：目标 yuxing，程序名 hello |
| `disk/README.TXT` | 放进磁盘镜像的使用说明示例，用 WinImage 等软件打开镜像时可直接查看；`disk` 目录下的文件都会放进镜像 |
| `.vscode/tasks.json` | 构建与运行的任务 |
