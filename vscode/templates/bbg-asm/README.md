# {{name}}

用 fcdev 扩展开发的步步高学生电脑（BBGDOS）程序，ca65 汇编。

## 构建

在 VS Code 里运行“终端 → 运行生成任务”（默认任务“fcdev：构建”），生成 `build/bbg/HELLO.CMD`。程序名必须符合 DOS 8.3 规则（主名不超过 8 个字符），在 `fcdev.json` 的 `name` 里修改。

步步高的 DOS 在软盘上，所以软盘镜像要建在一张带 DOS 的系统盘上：在 `fcdev.json` 里用 `diskImage` 指向系统盘镜像，构建时在它的基础上加入 HELLO.CMD 和项目 `disk` 目录下的全部文件（文件名必须是 8.3 名字，不能有子目录），生成 `build/bbg/HELLO.img`，系统盘本身不改。没有设置 `diskImage` 时只构建程序，不生成镜像。步步高的系统盘大多是装满的，要选剩余空间够放程序的；系统盘里有 AUTOEXEC.BAT 时，开机后先运行它，再出现命令提示符。

## 在模拟器里运行（只支持 Windows）

扩展不提供模拟器、ROM 与系统盘。先在设置 `fcdev.emulator.path` 里指定模拟器 Ember 的位置，再在 `fcdev.json` 里用 `firmware` 指定步步高软驱一号的 ROM 文件，用 `diskImage` 指定系统盘镜像：

```json
{
  "target": "bbg",
  "name": "hello",
  "firmware": "D:/rom/bbg1_bios.bin",
  "diskImage": "D:/disk/001-3.IMG"
}
```

然后运行任务“fcdev：运行”：Ember 以软驱一号开机，从镜像读入 DOS，跳过日期与时间，输入 `hello`。

要在真机或其他模拟器里运行，把软盘镜像挂载到软驱，开机进入 DOS 命令行，输入 `hello` 并回车（不用写扩展名）。

## 程序格式

- CMD 文件的前 2 个字节是装入地址（低字节在前），后面是程序内容；DOS 把内容装到这个地址并从这里开始执行。目标把程序放在 `$8000-$BFFF`（一个 16K 页），文件大小不超过 16386 字节。
- 文件的第一条指令是 `JMP`，由目标自带的启动代码提供；程序的入口是导出的 `_main`，由启动代码调用，`_main` 返回后启动代码调用 DOS 的“程序结束返回”回到命令行。
- 段：`CODE`、`RODATA`、`DATA`（都在程序所在的 RAM 里，可直接读写）、`BSS`（紧接在程序后面，不写进文件，也不会被清零，要自己赋初值）、`ZEROPAGE`（零页 `$0020-$007F`）。
- DOS 调用的入口与功能号在 `bbg.inc`，用 `.include "bbg.inc"` 引入。DOS 调用不保留寄存器。

## 文件

| 文件 | 说明 |
| --- | --- |
| `src/hello.s` | 示例程序：用 DOS 的显示字符功能显示两行英文，按任意键后回到 DOS，入口是导出的 `_main` |
| `fcdev.json` | fcdev 配置：目标 bbg，程序名 hello；运行时要加 `firmware` 与 `diskImage` |
| `.vscode/tasks.json` | 构建与运行的任务 |
