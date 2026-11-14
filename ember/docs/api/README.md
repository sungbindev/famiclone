# Ember 调试服务协议参考

本目录由 `npm run docs` 根据随 SDK 打包的 OpenRPC 文档（`debug-sdk/generated/openrpc.json`）生成，不要手改。

## 连接

- 地址 `ws://127.0.0.1:<端口>/`（WebSocket），默认端口 5688；可选子协议 `ember-debug.v1`。
- 报文是 JSON-RPC 2.0，参数按名称传递；事件是没有 `id` 的通知。
- 每条连接的第一个请求必须是 `debug.hello`（带令牌），5 秒内不握手或令牌不对，连接以关闭码 4001 关闭。
- 结果或事件数据里顶层的 `data` 是二进制时，整条消息改发一个二进制帧：4 字节大端的 JSON 长度、JSON 报文（不含 `data`）、`data` 的原始字节。SDK 已处理，`data` 直接是 `Uint8Array`。
- 令牌与端口在 Ember 的首选项“调试服务”分组里；Ember 运行时用户数据目录的 `run/debug-endpoint.json` 里有当前的地址与令牌，SDK 与终端客户端用它自动连接。
- 完整的协议规则（关闭码、限制、保活、事件队列）见 Ember 的《调试服务》文档（docs/04-调试服务.md）。

## 错误

错误响应的 `error.code` 是 JSON-RPC 错误码（-32700 报文不是 JSON、-32600 不是合法请求、-32601 方法不存在、-32602 参数不合格、-32603 核心内部错误、-32000 业务错误），`error.data.code` 是模块登记的错误码（如 `runtime.noCartridge`）。SDK 把它抛成 `EmberRequestError`（`code` 为错误码）。

## 模块

### [cartridges](cartridges.md)

| 请求 | 简介 |
| --- | --- |
| `cartridges.load` | 读取 NES 文件，解析文件头，按 mapper 号与变体匹配插件，装配主机并插入卡槽 |
| `cartridges.eject` | 取出卡带：主机随之拆除，电源状态变为断电 |
| `cartridges.info` | 当前卡带的描述、匹配到的插件与存储区清单 |

事件：`cartridges.loaded`

### [debug](debug.md)

| 请求 | 简介 |
| --- | --- |
| `debug.hello` | 握手：每条连接上第一个请求必须是它，令牌不对时连接被关闭（关闭码 4001） |
| `debug.subscribe` | 订阅事件 |
| `debug.unsubscribe` | 取消订阅：写法要与订阅时一致 |
| `debug.status` | 调试服务的状态：是否开启、是否在监听、端口与当前的连接 |

事件：`debug.statusChanged`、`debug.eventsDropped`

### [input](input.md)

| 请求 | 简介 |
| --- | --- |
| `input.key` | 按下或松开一个物理键，交给卡带的 mapper 脚本 |
| `input.mouse` | 鼠标事件，交给卡带的 mapper 脚本 |
| `input.gamepad` | 游戏手柄一个手柄的当前全部状态，交给卡带的 mapper 脚本 |
| `input.pad` | 直接设置主机某个手柄口上标准手柄的按键，不经 mapper 脚本 |
| `input.releaseAll` | 松开经 key 按下的全部键、把经 pad 设置过的手柄口清零、并让经 gamepad 注入的手柄断开 |

### [inspector](inspector.md)

| 请求 | 简介 |
| --- | --- |
| `inspector.listSpaces` | 列出全部内存空间：总线视角的 cpu 与 ppu，主板与芯片里的存储，以及卡带上的存储区（cart.<存储区标识>） |
| `inspector.readMemory` | 读一段内存，不改变主机的任何状态：cpu 与 ppu 空间里寄存器类地址读出 0（不读寄存器，所以不会清标志），卡带脚本的寄存器也不会被回调 |
| `inspector.writeMemory` | 写一段内存 |
| `inspector.resolve` | 解析一个总线地址：它落在哪块存储区的哪个偏移，或哪个寄存器 |
| `inspector.getState` | 读状态：cpu（寄存器与周期数）、ppu（扫描线、点、帧数、滚动寄存器、标志）与卡带的脚本状态（mapper） |
| `inspector.addBreakpoint` | 添加断点 |
| `inspector.listBreakpoints` | 列出全部断点，按编号升序 |
| `inspector.removeBreakpoint` | 删除一个断点 |
| `inspector.clearBreakpoints` | 删除全部断点 |
| `inspector.setState` | 修改一个模块的状态：目前只有 cpu 可以改（pc、a、x、y、sp、status，只在指令之间生效），其余模块只读 |

### [mappers](mappers.md)

| 请求 | 简介 |
| --- | --- |
| `mappers.list` | 列出生效的插件：同一标识有多个来源时只列版本最高的，版本相同时用户安装的优先 |
| `mappers.info` | 返回一个插件的摘要与清单详情 |
| `mappers.install` | 安装一个 .emap 插件包：检查通过后放进用户插件目录 |
| `mappers.uninstall` | 卸载用户安装的插件 |
| `mappers.setEnabled` | 启用或停用一个插件 |
| `mappers.reload` | 重新读取预装目录与用户目录里的插件包，下次装配或硬复位时生效 |

事件：`mappers.changed`

### [output](output.md)

| 请求 | 简介 |
| --- | --- |
| `output.setVolume` | 设置声音的音量与静音 |
| `output.screenshot` | 截取最近一次完成的画面（256x240，暂停与断电后也有上一帧） |

### [preferences](preferences.md)

| 请求 | 简介 |
| --- | --- |
| `preferences.list` | 列出全部分组与设置项（含当前取值） |
| `preferences.set` | 修改一个设置项：核心按登记的种类校验取值，保存到文件，并发出 preferences.changed |
| `preferences.reset` | 把一个设置项恢复为出厂默认值 |

事件：`preferences.changed`

### [runtime](runtime.md)

| 请求 | 简介 |
| --- | --- |
| `runtime.status` | 返回电源、运行模式、主机型号、卡带、帧号、总周期数与速度 |
| `runtime.powerOn` | 上电：创建或重置全部芯片，卡带的存储区重新从文件载入 |
| `runtime.powerOff` | 断电：停止运行，芯片状态丢弃 |
| `runtime.reset` | 复位：soft 只发 RESET 线，不改变电源与运行模式 |
| `runtime.pause` | 暂停：已暂停时没有效果 |
| `runtime.resume` | 继续：按真实帧率连续运行 |
| `runtime.run` | 授权运行：不限速地连续运行指定的帧数后暂停（运行中不输出声音，每帧之间照常处理输入等命令） |
| `runtime.step` | 单步：先暂停（正在进行的授权运行被打断），再按单位运行指定的数量后保持暂停 |
| `runtime.setFrameEvents` | 设置 runtime.frame 事件的发布间隔：每 frames 帧发一次，0 表示不发（默认） |

事件：`runtime.stateChanged`、`runtime.frame`、`runtime.runFinished`、`runtime.breakpointHit`、`runtime.error`

### [system](system.md)

| 请求 | 简介 |
| --- | --- |
| `system.version` | 返回应用版本与核心版本 |

### [rpc](rpc.md)

| 请求 | 简介 |
| --- | --- |
| `rpc.discover` | 返回本文档 |
