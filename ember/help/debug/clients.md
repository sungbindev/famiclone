# 客户端工具

官方的客户端工具是 VS Code 扩展 fcdev 里的调试命令与 Copilot 工具，不单独发布。协议是开放的，也可以不用它，直接按[连接与报文](protocol.md)连接。

| 方式 | 说明 |
| --- | --- |
| 调试命令 | 在 VS Code 的命令面板里输入“fcdev：”：显示调试服务地址、查看调试请求说明、调用调试请求、保存调试截图、监视调试事件；结果写到“输出”面板的“fcdev 调试” |
| Copilot 工具 | `emu_connect`、`emu_launch`、`emu_run`、`emu_inspect` 等，让 Copilot 的 Agent 载入程序、运行、读内存、单步、截图与注入按键 |

## 自动连接

Ember 开始监听后会写端点文件（见[连接与报文](protocol.md)），扩展读它自动连接。也可以用环境变量指定：

| 变量 | 说明 |
| --- | --- |
| `EMBER_DEBUG_URL`、`EMBER_DEBUG_TOKEN` | 直接指定调试服务的地址与令牌 |
| `EMBER_DEBUG_ENDPOINT` | 端点文件的路径 |
| `EMBER_USER_DATA` | Ember 的用户数据目录（用 `--user-data-dir` 启动了第二个实例时用它区分） |

查找顺序：显式指定、环境变量、端点文件。端点文件里的进程已退出时视为过期，不会连接。扩展用命令“fcdev：运行”“fcdev：调试运行”启动的 Ember 按设置 `fcdev.emulator.debugPort` 的端口区分用户数据目录，调试命令先找它，没有再找 Ember 自己的用户数据目录。

## 调试命令

| 命令 | 作用 |
| --- | --- |
| fcdev：显示调试服务地址 | 显示正在运行的 Ember 的调试服务地址（令牌打码） |
| fcdev：查看调试请求说明 | 选一个请求或事件，显示参数、结果、错误码与示例 |
| fcdev：调用调试请求 | 选一个请求，输入参数后调用 |
| fcdev：保存调试截图 | 把当前画面保存为 PNG |
| fcdev：监视调试事件 | 订阅事件并逐行写到输出，取消或连接关闭时停止 |

“调用调试请求”的参数写成 `key=value`（值按 JSON 解析，不是 JSON 就当字符串；`0x8000` 与 `$8000` 是十六进制数），或一个 JSON 对象，例如：

```text
cartridges.load      path=D:/roms/game.nes
runtime.run          frames=300 wait=true
inspector.readMemory space=cpu address=0x8000 length=16
```

结果以 JSON 写到输出；结果里有二进制数据时会问是否保存成文件。请求失败时提示错误码（如 `runtime.noCartridge`），连不上 Ember 时提示原因。
