# {{name}}

用 fcdev 扩展开发的小霸王学习机 SB2000（SB-DOS）程序，ca65 汇编。

## 构建

在 VS Code 里运行“终端 → 运行生成任务”（默认任务“fcdev：构建”），生成 `build/sb2000/HELLO.EXE`；
任务“fcdev：构建磁盘镜像”另外生成放有 HELLO.EXE 的软盘镜像 `build/sb2000/HELLO.img`。

程序名必须符合 DOS 8.3 规则（主名不超过 8 个字符），在 `fcdev.json` 的 `name` 里修改。

## 在模拟器里运行（只支持 Windows）

扩展不提供模拟器与 BIOS。先在设置 `fcdev.emulator.path` 里指定模拟器 Ember 的位置，再在 `fcdev.json` 里用 `firmware` 指定 SB2000 的 BIOS 文件：

```json
{ "target": "sb2000", "name": "hello", "firmware": "D:/rom/SB2K_BIOS.NES" }
```

然后运行任务“fcdev：运行”。DOS 启动后按两次回车跳过日期与时间提示，在提示符下输入 `hello`，显示后按任意键返回 DOS。

源文件用 UTF-8 保存，字符串里可以直接写汉字：构建时自动把它转成学习机用的 GB2312。汉字要写在 `.s` 文件里，不在 GB2312 里的字符会报错。

## 文件

| 文件 | 说明 |
| --- | --- |
| `src/hello.s` | 示例程序：显示一行英文和一行中文，等待按键后返回 DOS，入口是导出的 `_main` |
| `fcdev.json` | fcdev 配置：目标 sb2000，程序名 hello |
| `.vscode/tasks.json` | 构建、生成磁盘镜像与运行的任务 |

## 文档

命令“fcdev：打开使用文档”列出全部主题，选“SB2000 开发指南”或“SB-DOS 系统调用手册”查阅。
