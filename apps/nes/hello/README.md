# hello

用 FC Dev 扩展开发的任天堂红白机（NES）程序，C 语言。

## 构建与运行

在 VS Code 里运行“终端 → 运行生成任务”（默认任务“FCDEV：构建”），生成 `build/nes/hello.nes`；
“终端 → 运行任务 → FCDEV：运行”在模拟器里运行。运行需要先在设置 `fcdev.emulator.path` 里指定模拟器 Ember 的位置，只支持 Windows。

## 文件

| 文件 | 说明 |
| --- | --- |
| `src/hello.c` | 示例程序：在屏幕上显示 HELLO, NES! |
| `fcdev.json` | FC Dev 配置：目标 nes，程序名 hello |
| `.vscode/tasks.json` | 构建与运行的任务 |

## 文档

命令“FCDEV：打开使用文档”列出全部主题，选“NES 开发指南”查 NES 目标的内存、函数与运行方式。
