# {{name}}

用 FC Dev 扩展开发的中华学习机 CEC-I（Apple DOS 3.3）程序，ca65 汇编。

## 构建

在 VS Code 里运行“终端 → 运行生成任务”（默认任务“FCDEV：构建”），生成 `build/cec1/DEMO`。程序名必须以字母开头，只用字母与数字，最多 30 个字符，在 `fcdev.json` 的 `name` 里修改；不要用 `hello`，DOS 3.3 系统盘的启动程序常常叫 HELLO，它被锁定，构建磁盘镜像时不能被替换。

DOS 3.3 在软盘上，所以软盘镜像要建在一张带 DOS 3.3 的系统盘上：在 `fcdev.json` 里用 `diskImage` 指向系统盘镜像（`.dsk`，DOS 扇区顺序，143360 字节），构建时在它的基础上加入 DEMO（二进制文件，类型 B）和项目 `disk` 目录下的全部文件（按文本文件，类型 T，原样放入；文件名不超过 30 个字符，不能有子目录），系统盘本身不改。要生成镜像，在 `fcdev.json` 里加 `diskName`（如 `"diskName": "cec1-hello.dsk"`），镜像生成在 `build/cec1/` 下；没有设置 `diskImage` 或 `diskName` 时只构建程序，不生成镜像。

## 在模拟器里运行（只支持 Windows）

扩展不提供模拟器、ROM 与系统盘。先在设置 `fcdev.emulator.path` 里指定模拟器 Ember 的位置，再在 `fcdev.json` 里用 `firmware` 指定 CEC-I 的 ROM 文件，用 `diskImage` 指定系统盘镜像：

```json
{
  "target": "cec1",
  "name": "demo",
  "firmware": "D:/rom/cec1_v1.21.bin",
  "diskImage": "D:/disk/DOS33.DSK"
}
```

然后运行任务“FCDEV：运行”：Ember 以 CEC-I 开机，从镜像启动 DOS 3.3，输入 `BRUN DEMO`。

要在真机或其他模拟器里运行，把软盘镜像挂载到软驱，启动 DOS 3.3 后输入 `BRUN DEMO` 并回车。

## 程序格式

- 程序是 DOS 3.3 的二进制文件（类型 B）：文件前 4 个字节是装入地址与长度（低字节在前），后面是程序内容；`BRUN` 把内容装到这个地址并从这里开始执行。装入地址默认 `$6000`，在 `fcdev.json` 的 `targetOptions` 里用 `loadAddress` 修改，如 `"targetOptions": { "loadAddress": "$0800" }`，取值 `$0800` 到 `$8FFF`；程序放在装入地址到 `$9200`（汉字方式下用户程序的上限）之间。
- 程序的入口是导出的 `_main`，由目标自带的启动代码调用，`_main` 返回后启动代码跳到 DOS 的热启动入口 `$03D0`，回到 BASIC 的命令行（内存里的 BASIC 程序保留）。不用 `RTS` 回到 DOS：DOS 3.3 进入程序前把自己的几层调用退了栈，程序调用一次 `COUT` 就会把它返回时要用的保存信息改掉，`RTS` 回去会跑飞。
- 段：`CODE`、`RODATA`、`DATA`（都在程序所在的 RAM 里，可直接读写）、`BSS`（紧接在程序后面，不写进文件，也不会被清零，要自己赋初值）、`ZEROPAGE`（零页 `$001A-$001F`）。
- 监控程序入口、软开关与 DOS 3.3 入口的符号在 `cec1.inc`，用 `.include "cec1.inc"` 引入；显示与键盘的函数在运行库 `cec1.lib`，用 `JSR` 调用。

## 文件

| 文件 | 说明 |
| --- | --- |
| `src/hello.s` | 示例程序：清屏，显示两行英文，按任意键后回到 DOS，入口是导出的 `_main` |
| `fcdev.json` | FC Dev 配置：目标 cec1，程序名 demo；运行时要加 `firmware` 与 `diskImage` |
| `.vscode/tasks.json` | 构建与运行的任务 |
