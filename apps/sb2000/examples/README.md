# SB2000 示例程序

用 fcdev 构建的小霸王学习机 SB2000（SB-DOS）示例，每个子目录是一个独立的 fcdev 项目（`fcdev.json` 加 `src/`）。除 `play` 是汇编外都是 C。接口说明见 [packages/fcdev/docs/sb2000/01-开发指南.md](../../../packages/fcdev/docs/sb2000/01-开发指南.md)。

## 构建

在仓库根目录运行，产物在各示例的 `build/sb2000/` 下：

```bash
node packages/fcdev/bin/fcdev.mjs build apps/sb2000/examples/hello
node packages/fcdev/bin/fcdev.mjs disk apps/sb2000/examples/hello   # 另外生成放有程序的软盘镜像
```

## 示例

| 目录 | 程序名 | 作用 | 用法（在 DOS 提示符下） |
| --- | --- | --- | --- |
| `hello` | `HELLO` | 显示英文与中文各一行，等待按键 | `hello` |
| `say` | `SAY` | 显示命令行参数 | `say 文字` |
| `show` | `SHOW` | 读文件并显示开头 90 字节 | `show 文件名` |
| `copy` | `CP` | 复制文件，程序名取 `CP` 以避开 DOS 内部的 `COPY` | `cp 源 目标` |
| `peek` | `PEEK` | 按十六进制偏移读文件 30 字节 | `peek 文件名 偏移` |
| `poke` | `POKE` | 按十六进制偏移把文字写进已有文件（最长 60 个字符，大写） | `poke 文件名 偏移 文字` |
| `pad` | `PAD` | 等手柄按键，显示键码与两个手柄的状态 | `pad` |
| `mouse` | `MOUSE` | 复位鼠标、设坐标范围，等一会儿后显示坐标与按键状态 | `mouse` |
| `play` | `PLAY` | 把 NES 游戏装入并运行（mapper 0 与 4，PRG、CHR 各不超过 128K） | `play 文件名.nes` |
| `newcall` | `NEWCALL` | 调用 BIOS 里新增的系统调用 $35 并显示扩展版本号，原始 BIOS 显示 NOT SUPPORTED | `newcall` |
| `syscall` | `SYSCALL` | 系统调用实验：按 ERAM $7E00 的请求块代为调用系统服务，供调试器使用 | `syscall` |

## 运行

fcdev 不提供 BIOS。在示例目录的 `fcdev.json` 里加上 `"firmware": "BIOS 文件路径"`（仓库里有一份：`ember/bios/sb2000_bios.bin`），用 `npx fcdev run apps/sb2000/examples/<名>` 在 Ember 里运行：程序会放进软盘镜像，开机后自动跳过 SB-DOS 的日期与时间提示并输入程序名。也可以只用 `npx fcdev build`、`npx fcdev disk` 构建并生成磁盘镜像。

`show`、`copy`、`peek`、`poke` 需要磁盘里有数据文件，可以自己用 `npx fcdev disk` 生成的镜像加上文件，或在模拟器里挂载别的镜像；`play` 需要 `.nes` 文件，放进软盘镜像时文件名要符合 8.3（如 `mario.nes`）。

用 `npx fcdev run --debug` 启动后，调试请求 `input.pad port=0 buttons=1`（按下 1 号手柄 A）、`input.mouse kind=move dx=10 dy=5`（移动鼠标）、`input.mouse kind=down buttons=1`（按下鼠标左键）可以给 `pad`、`mouse` 注入输入，用 `npx fcdev debug call <请求> <参数>` 发送，请求表见 [02-模拟器与调试](../../../packages/fcdev/docs/02-模拟器与调试.md)。
