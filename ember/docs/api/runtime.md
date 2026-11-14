# runtime 模块

请求与事件的规则、连接与认证见 [README](README.md) 与 Ember 的《调试服务》文档。

## 请求

| 请求 | 简介 |
| --- | --- |
| [`runtime.status`](#runtimestatus) | 返回电源、运行模式、主机型号、卡带、帧号、总周期数与速度 |
| [`runtime.powerOn`](#runtimepoweron) | 上电：创建或重置全部芯片，卡带的存储区重新从文件载入 |
| [`runtime.powerOff`](#runtimepoweroff) | 断电：停止运行，芯片状态丢弃 |
| [`runtime.reset`](#runtimereset) | 复位：soft 只发 RESET 线，不改变电源与运行模式 |
| [`runtime.pause`](#runtimepause) | 暂停：已暂停时没有效果 |
| [`runtime.resume`](#runtimeresume) | 继续：按真实帧率连续运行 |
| [`runtime.run`](#runtimerun) | 授权运行：不限速地连续运行指定的帧数后暂停（运行中不输出声音，每帧之间照常处理输入等命令） |
| [`runtime.step`](#runtimestep) | 单步：先暂停（正在进行的授权运行被打断），再按单位运行指定的数量后保持暂停 |
| [`runtime.setFrameEvents`](#runtimesetframeevents) | 设置 runtime.frame 事件的发布间隔：每 frames 帧发一次，0 表示不发（默认） |

### runtime.status

返回电源、运行模式、主机型号、卡带、帧号、总周期数与速度。

**参数**

（无）

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `power` | `"off" \| "on"` | 是 | 电源 |
| `mode` | `"paused" \| "running"` | 是 | 运行模式 |
| `machine` | `string` | 否 | 主机型号；没有装入卡带时没有这一项 |
| `cartridge` | [runtime.CartridgeSummary](runtime.md#cartridgesummary) | 否 | 当前的卡带；没有时没有这一项 |
| `frame` | `integer（0..）` | 是 | 自上电以来完成的帧数 |
| `cycles` | `integer（0..）` | 是 | 自上电以来的 CPU 周期数 |
| `speed` | `integer（1..）` | 是 | 速度百分比，目前固定为 100 |

**错误码**：无专有错误码（参数不合格返回 `core.invalidParams`）

**示例**

```text
ember-debug call runtime.status
```

```ts
await client.api.runtime.status();
```

### runtime.powerOn

上电：创建或重置全部芯片，卡带的存储区重新从文件载入；默认进入暂停，resume 为 true 时直接运行；已通电时等同硬复位。

**参数**

| 名称 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `resume` | `boolean` | 否 | 上电后是否直接运行，默认 false |

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `state` | [runtime.RuntimeState](runtime.md#runtimestate) | 是 |  |

**错误码**：`runtime.noCartridge`

**示例**

```text
ember-debug call runtime.powerOn
```

```ts
await client.api.runtime.powerOn();
```

### runtime.powerOff

断电：停止运行，芯片状态丢弃；卡带保持插着。已断电时没有效果。

**参数**

（无）

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `state` | [runtime.RuntimeState](runtime.md#runtimestate) | 是 |  |

**错误码**：无专有错误码（参数不合格返回 `core.invalidParams`）

**示例**

```text
ember-debug call runtime.powerOff
```

```ts
await client.api.runtime.powerOff();
```

### runtime.reset

复位：soft 只发 RESET 线，不改变电源与运行模式；hard 断电后再上电，运行模式保持。

**参数**

| 名称 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `kind` | `"soft" \| "hard"` | 是 | 复位方式 |

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `state` | [runtime.RuntimeState](runtime.md#runtimestate) | 是 |  |

**错误码**：`runtime.noCartridge`、`runtime.notPowered`

**示例**

```text
ember-debug call runtime.reset kind=<"soft" | "hard">
```

```ts
await client.api.runtime.reset({ kind: … });
```

### runtime.pause

暂停：已暂停时没有效果；只在通电时有效。

**参数**

（无）

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `state` | [runtime.RuntimeState](runtime.md#runtimestate) | 是 |  |

**错误码**：`runtime.notPowered`

**示例**

```text
ember-debug call runtime.pause
```

```ts
await client.api.runtime.pause();
```

### runtime.resume

继续：按真实帧率连续运行；已在运行时没有效果；只在通电时有效。

**参数**

（无）

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `state` | [runtime.RuntimeState](runtime.md#runtimestate) | 是 |  |

**错误码**：`runtime.notPowered`

**示例**

```text
ember-debug call runtime.resume
```

```ts
await client.api.runtime.resume();
```

### runtime.run

> 只经调试服务开放（界面不能发起）

授权运行：不限速地连续运行指定的帧数后暂停（运行中不输出声音，每帧之间照常处理输入等命令）。wait 为 true 时等运行结束才返回结果（结果里有 stopped）；否则立即返回，运行结束时发事件 runtime.runFinished。已在授权运行中时旧的被打断，由新的替代；等待中的请求被取消（如连接断开）时运行被打断并暂停。之后会加入 cycles、scanlines、instructions 等其他授权量。

**参数**

| 名称 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `frames` | `integer（1..10000000）` | 是 | 要运行的帧数 |
| `wait` | `boolean` | 否 | 是否等运行结束再返回，默认 false |

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `state` | [runtime.RuntimeState](runtime.md#runtimestate) | 是 | 返回时的运行状态 |
| `frames` | `integer（0..）` | 是 | 实际运行的帧数；wait 为 false 时为 0 |
| `cycles` | `integer（0..）` | 是 | 实际经过的 CPU 周期数；wait 为 false 时为 0 |
| `stopped` | `"completed" \| "interrupted" \| "powerOff" \| "breakpoint" \| "error"` | 否 | 停止原因（只有 wait 为 true 时有）：completed 运行完授权的帧数；interrupted 被暂停、继续、硬复位、换卡带、新的授权运行或请求被取消打断；powerOff 断电；breakpoint 命中断点；error 运行出错 |

**错误码**：`runtime.noCartridge`、`runtime.notPowered`

**示例**

```text
ember-debug call runtime.run frames=<integer>
```

```ts
await client.api.runtime.run({ frames: … });
```

### runtime.step

> 只经调试服务开放（界面不能发起）

单步：先暂停（正在进行的授权运行被打断），再按单位运行指定的数量后保持暂停。instruction 一条指令（含随后响应的中断序列）；cycle 至少这么多 CPU 周期（指令不被打断，实际周期数可能多几个）；scanline PPU 的扫描线推进这么多行；frame 完成这么多帧画面。结果里的 pc 是停下时下一条要执行的指令地址。经过的帧会照常显示。

**参数**

| 名称 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `unit` | `"instruction" \| "cycle" \| "scanline" \| "frame"` | 是 | 单位 |
| `count` | `integer（1..100000）` | 否 | 数量，默认 1；要运行更长时间用 runtime.run |

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `state` | [runtime.RuntimeState](runtime.md#runtimestate) | 是 | 单步后的运行状态（总是暂停） |
| `instructions` | `integer（0..）` | 是 | 实际执行的指令数 |
| `cycles` | `integer（0..）` | 是 | 实际经过的 CPU 周期数 |
| `frames` | `integer（0..）` | 是 | 期间完成的帧数 |
| `pc` | `integer（0..65535）` | 是 | 停下时程序计数器的值，即下一条要执行的指令地址 |

**错误码**：`runtime.noCartridge`、`runtime.notPowered`、`runtime.debugUnsupported`

**示例**

```text
ember-debug call runtime.step unit=<"instruction" | "cycle" | "scanline" | "frame">
```

```ts
await client.api.runtime.step({ unit: … });
```

### runtime.setFrameEvents

> 只经调试服务开放（界面不能发起）

设置 runtime.frame 事件的发布间隔：每 frames 帧发一次，0 表示不发（默认）。这是全局设置，对订阅了 runtime.frame 的所有连接生效。

**参数**

| 名称 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `frames` | `integer（0..3600）` | 是 | 间隔帧数，0 表示不发 |

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `frames` | `integer（0..）` | 是 | 设置之后的间隔帧数 |

**错误码**：无专有错误码（参数不合格返回 `core.invalidParams`）

**示例**

```text
ember-debug call runtime.setFrameEvents frames=<integer>
```

```ts
await client.api.runtime.setFrameEvents({ frames: … });
```

## 事件

事件要先用 `debug.subscribe` 订阅才会收到，以 JSON-RPC 通知送达。

### runtime.stateChanged

电源或运行模式改变（含装入或取出卡带）。

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `power` | `"off" \| "on"` | 是 | 新的电源状态 |
| `mode` | `"paused" \| "running"` | 是 | 新的运行模式 |
| `reason` | `"load" \| "eject" \| "powerOn" \| "powerOff" \| "reset" \| "pause" \| "resume" \| "run" \| "runEnded" \| "step" \| "breakpoint" \| "error"` | 是 | 原因：run 开始授权运行，runEnded 授权运行跑完后暂停，step 单步，breakpoint 命中断点后暂停 |

### runtime.frame

每完成一帧发出；默认不订阅，开启后才发布。

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `frame` | `integer（0..）` | 是 | 帧号 |
| `cycles` | `integer（0..）` | 是 | 自上电以来的 CPU 周期数 |
| `pc` | `integer（0..65535）` | 是 | CPU 的程序计数器 |

### runtime.runFinished

授权运行（run）结束：跑完、或被暂停、继续、复位、断电、换卡带、新的授权运行或出错打断。

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `frames` | `integer（0..）` | 是 | 实际运行的帧数 |
| `cycles` | `integer（0..）` | 是 | 实际经过的 CPU 周期数 |
| `stopped` | `"completed" \| "interrupted" \| "powerOff" \| "breakpoint" \| "error"` | 是 | 停止原因，含义同 run 的结果 |

### runtime.breakpointHit

命中断点，机器已暂停（自由运行与授权运行都会停下）。执行断点停在指令执行之前，读写断点停在那条指令执行完之后；断点由 inspector.addBreakpoint 添加。

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `id` | `integer（1..）` | 是 | 断点编号 |
| `kind` | `"execute" \| "read" \| "write"` | 是 | 断点种类 |
| `address` | `integer（0..65535）` | 是 | 执行断点是将要执行的指令地址；读写断点是被访问的地址 |
| `value` | `integer（0..255）` | 否 | 读写断点读到或写入的值；执行断点没有这一项 |
| `pc` | `integer（0..65535）` | 是 | 停下时的程序计数器 |
| `a` | `integer（0..255）` | 是 | 累加器 |
| `x` | `integer（0..255）` | 是 | X 寄存器 |
| `y` | `integer（0..255）` | 是 | Y 寄存器 |
| `sp` | `integer（0..255）` | 是 | 栈指针 |
| `status` | `integer（0..255）` | 是 | 状态寄存器 |

### runtime.error

运行中的错误，如 mapper 脚本异常；发出后模拟器已暂停。

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `module` | `string` | 是 | 出错的模块 |
| `code` | `string` | 是 | 错误码 |
| `message` | `string` | 是 | 面向用户的说明 |

## 数据类型

### CartridgeStorage

卡带上的一块存储区。

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `id` | `string` | 是 | 清单里的存储区标识 |
| `kind` | `"rom" \| "ram"` | 是 | 种类 |
| `size` | `integer（0..）` | 是 | 实际容量（字节） |
| `battery` | `boolean` | 是 | 是否带电池 |

### CartridgeSummary

插在卡槽里的卡带：文件头描述、匹配到的插件与存储区清单。

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `path` | `string` | 是 | NES 文件的路径 |
| `mapper` | `integer（0..）` | 是 | mapper 号 |
| `variant` | `integer（0..）` | 是 | 变体 |
| `prgSize` | `integer（0..）` | 是 | PRG 的字节数 |
| `chrSize` | `integer（0..）` | 是 | CHR 的字节数，没有 CHR 时为 0 |
| `mirroring` | `"horizontal" \| "vertical" \| "four"` | 是 | 文件头里的镜像方式 |
| `battery` | `boolean` | 是 | 文件头里的电池标志 |
| `crc32Prg` | `string（长度 8..8）` | 是 | PRG 的 CRC-32，8 位十六进制 |
| `crc32All` | `string（长度 8..8）` | 是 | PRG 加 CHR 的 CRC-32，8 位十六进制 |
| `pluginId` | `string` | 是 | 匹配到的插件标识 |
| `pluginName` | `string` | 是 | 匹配到的插件显示名称 |
| `pluginVersion` | `string` | 是 | 匹配到的插件版本 |
| `host` | `string` | 是 | 装配的主机型号 |
| `storage` | [runtime.CartridgeStorage](runtime.md#cartridgestorage)[] | 是 | 存储区清单 |

### RuntimeState

运行状态：电源与运行模式；运行模式只在通电时有意义，断电时为 paused。

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `power` | `"off" \| "on"` | 是 | 电源：off 断电，on 通电 |
| `mode` | `"paused" \| "running"` | 是 | 运行模式：paused 暂停，running 连续运行 |
