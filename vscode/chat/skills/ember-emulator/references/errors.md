# emu_* 与运行类工具的报错处理

任一 `emu_*` 工具、`fcdev_run` 报错时使用。错误的 `code` 与 `hint` 是依据；先按下表的“动作”处理，同一原因最多重试 1 次，第二次仍失败就停下，汇报错误原文与已完成的步骤。

## 自己可以处理的

| `code` 或现象 | 含义 | 动作 |
| --- | --- | --- |
| `EMULATOR_NOT_CONNECTED` | 还没有连接 Ember | 回到 SKILL 的“起步”，从 `emu_status` 做起 |
| `EMULATOR_CONNECT_FAILED` | 连不上调试服务 | 调用 `emu_launch`；它报“端口已被占用”时汇报给用户 |
| `EMULATOR_CONNECTION_LOST` | 连接中途断开，多半是 Ember 被关闭 | 调用 `emu_status` 确认 `connected` 为 `false`，从“起步”重做，并告诉用户已重启 |
| `EMULATOR_NOT_READY` | 启动后调试服务没有在时限内就绪 | 重试一次 `emu_launch`；仍失败就汇报，并附 `hint` |
| `EMULATOR_PARAM_INVALID` | 参数缺少或组合不对 | 按 `hint` 改正参数后重试一次 |
| `EMULATOR_REQUEST_FAILED` | Ember 拒绝了请求 | 读错误里的 Ember 返回信息，改正参数后重试一次；信息指向内存地址或机器状态时，先 `emu_status` 看机器是否已通电 |
| `DOS_KEY_UNSUPPORTED` | 程序名里有无法在 SB-DOS 输入的字符 | 程序名只用字母与数字；fcdev 项目改 `fcdev.json` 的 `name` 后重新 `fcdev_build` |
| `EMULATOR_LOAD_FAILED` | Ember 添加固件、开机或放入磁盘镜像失败 | 读错误里的具体步骤与原因；固件、镜像的文件内容有问题时汇报给用户，不换文件碰运气 |

## 必须汇报给用户的

| `code` 或现象 | 含义 | 汇报内容 |
| --- | --- | --- |
| `EMULATOR_PATH_NOT_SET` | 用户设置里没有 Ember 的位置 | 请用户在用户设置 `fcdev.emulator.path` 填 Ember 可执行文件的路径；只能在用户设置里填，工作区设置无效 |
| `EMULATOR_NOT_FOUND` | 设置指向的文件不存在 | 给出错误里的路径原文，请用户核对设置 |
| 端口已被占用（`emu_connect` 也连不上） | 有别的程序占着调试端口 | 请用户关闭占用端口的程序，或指定另一个端口（设置 `fcdev.emulator.debugPort`，或给工具的 `port`） |
| `FIRMWARE_MISSING` | 找不到固件（BIOS） | 请用户提供 BIOS 文件的路径；fcdev 项目写进 `fcdev.json` 的 `firmware` |
| `EMULATOR_FILE_MISSING` | 要载入的文件不存在 | 给出路径原文；文件是构建产物时先重新 `fcdev_build`，其余请用户提供 |
| `TARGET_NO_RUN` | 这个目标没有运行方案 | 说明目标名，请用户确认目标 |
| `fcdev_run` 报“需要固件” | SB2000 项目的 `fcdev.json` 没有 `firmware` | 请用户提供 BIOS 路径 |

## 不是错误的情形

- `emu_status` 返回 `connected: false`：没有会话，进入“起步”的 `emu_launch`。
- `emu_run` 的 `stopped` 不是 `completed`：不是工具失败，按 SKILL 的 `stopped` 表处理。
- `emu_screen` 在断电后仍返回上一帧：不代表机器在运行，以 `emu_status` 的 `runtime.power` 为准。
