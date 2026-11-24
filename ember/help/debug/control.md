# 运行控制与典型流程

## 状态

| 状态 | 操作 |
| --- | --- |
| 电源（断电、通电） | `runtime.powerOn`、`runtime.powerOff`、`runtime.reset`（硬复位等于断电再上电） |
| 运行模式（暂停、运行） | `runtime.pause`、`runtime.resume`、`runtime.run`、`runtime.step`；只在通电时有效 |
| 卡带（空、已插） | `cartridges.load` 切换并重新装配主机，`cartridges.eject` 取出；整机机型不插卡带，经 `machines.boot` 装配主机 |
| 每个驱动器（无盘、有盘） | `media.floppyInsert`、`media.floppyEject`；驱动器之间互不影响，与电源无关 |
| 磁带机（停、播放、录音） | `media.tapePlay`、`media.tapeRecord`、`media.tapeStop`、`media.tapeSeek`（定位，停止与播放时都可以，录音时不行）；播放与录音要主机通电；`media.tapeTimeline` 读磁带上的声音大小与空白段，用来找程序段 |

- 软复位只发复位信号，不改变电源、运行模式、卡带和介质。
- 上电后默认进入暂停，调用 `resume` 或 `run` 才运行；`runtime.powerOn` 可以带 `resume: true` 直接运行。`cartridges.load` 默认装入后上电并运行，`powerOn`、`resume` 都可以设为 `false`。
- `powerOff` 后运行模式回到暂停；硬复位保持运行模式，帧号从头计；已通电时再上电等同硬复位；重复暂停、重复继续、重复断电、空槽取出都没有效果也不报错。
- 模拟器自由运行时，读写内存、读写状态等检查请求会在下一个帧边界执行，结果是一致的快照，不需要先暂停。需要精确停在某一点时，用 `runtime.step` 单步，或设断点让运行自己停下。

## 授权运行

`runtime.run` 让模拟器不限速地按授权量连续运行，之后暂停：

| 参数 | 说明 |
| --- | --- |
| `frames` | 要运行的帧数，1 到 10000000 |
| `cycles` | 至少这么多 CPU 周期（指令不被打断，实际周期数可能多几个） |
| `instructions` | 这么多条指令（含随后响应的中断序列） |
| `scanlines` | PPU 的扫描线推进这么多行 |
| `wait` | 默认 `false`；为 `true` 时等运行结束才返回响应 |

- `frames`、`cycles`、`instructions`、`scanlines` 必须给且只能给一个；按周期、指令、扫描线运行要主机型号支持调试。

- 运行中不输出声音，每帧之间照常处理输入等命令。
- `wait: true` 时，结果的 `stopped` 说明停止原因：`completed` 跑完、`interrupted` 被暂停、继续、硬复位、换卡带、新的授权运行或请求被取消打断、`powerOff` 断电、`breakpoint` 命中断点、`error` 运行出错；`frames` 与 `cycles` 是实际运行的帧数与 CPU 周期数，按周期、指令、扫描线授权时还有 `instructions`（实际执行的指令条数）。
- `wait: false` 时立即返回，结束时发事件 `runtime.runFinished`。
- 已在授权运行中时，旧的被打断，由新的替代。
- 命中断点时运行自动停止，不需要额外参数。

## 运行速度与声音

| 要做什么 | 请求 |
| --- | --- |
| 改运行速度 | `runtime.setSpeed`：`percent`（10 到 1000，相对真实速度的百分比）或 `max`（为 `true` 时不限速），都不给时恢复 100%；只影响自由运行，授权运行与单步不受影响；速度不是 100% 或不限速时不输出声音；不保存，每次启动都是 100% |
| 让声音不等声卡 | `output.setAudioMode`：`mode` 为 `fast` 时采样直接丢弃、设备播放静音，模拟器内部时序不变；`real` 恢复；不保存 |
| 装入前预览卡带 | `cartridges.parseHeader`：只解析 NES 文件头并预览匹配到的插件，不装配、不影响正在运行的卡带；匹配失败时结果里的 `matchError` 说明原因 |

## 典型流程

开发者或智能体调试一个程序，模拟器只启动一次：

1. 用 `--debug-port=5688` 启动模拟器（或在首选项里开启调试服务），读端点文件取得地址与令牌，连接并 `debug.hello`。
2. `machines.boot` 以学习机机型开机（`powerOn` 设为 `false`），或 `cartridges.load` 载入 FC 卡带；`media.floppyInsert` 插入盘片；`runtime.powerOn` 并带 `resume`。
3. `runtime.run` 等待若干帧，`output.screenshot` 看画面，`input.key` 输入命令，`inspector.readMemory` 读结果。
4. 改了程序：重新生成盘片后，再次 `media.floppyInsert`，`runtime.reset` 硬复位。
5. 改了 mapper 的脚本：`mappers.reload`，`runtime.reset` 硬复位。
6. 换另一个 ROM：`cartridges.load`。
7. 需要定位问题：`inspector.addBreakpoint`，`runtime.resume` 或 `runtime.run`（`wait: true`）等命中，`inspector.snapshot` 取全部状态。

各请求的参数与结果见[请求与事件总览](../api/index.md)。

## 调试与分析

`inspector` 模块还提供下面这些调试数据，只在 6502 类的机型（经典 FC 与全部学习机）上有，参数与结果见[请求与事件总览](../api/index.md)：

| 要看什么 | 请求 |
| --- | --- |
| 带条件的断点（如 `a == $40 && [$10] != 0`）、启用与禁用断点 | `inspector.addBreakpoint`（参数 `condition`）、`inspector.updateBreakpoint` |
| 在 CPU 响应 IRQ、NMI、BRK 之后、复位之后停下 | `inspector.addBreakpoint`（`kind` 取 `irq`、`nmi`、`brk`、`reset`，不给 `address`） |
| 调用栈（JSR 调用的子程序与中断处理程序） | `inspector.callStack` |
| 每个子程序花掉的 CPU 周期 | `inspector.profilerStart`、`profilerGet`、`profilerStop` |
| 寄存器读写与中断发生在哪一帧、哪一行、哪一点 | `inspector.eventsStart`、`eventsGet`、`eventsStop` |
| 现在显示的 64 种颜色的 RGB 值 | `inspector.getPalette` |
| 边记边读指令跟踪 | `inspector.traceStart`（参数 `ring` 为 `true` 保留最新的行）、`inspector.traceRead` |

## 事件

订阅后才会收到事件，高频事件（如 `runtime.frame`）还要再开启才会发。常用的事件有：

| 事件 | 触发 |
| --- | --- |
| `runtime.stateChanged` | 电源或运行模式改变 |
| `runtime.runFinished` | 授权运行结束 |
| `runtime.breakpointHit` | 命中断点，机器已暂停 |
| `runtime.error` | 脚本异常、总线错误等，发出后模拟器已暂停 |
| `media.changed` | 磁带或盘片状态变化 |
| `cartridges.loaded` | 卡带装配完成 |
| `preferences.changed` | 一个设置项的取值变了 |

全部事件及其数据在对应模块的页面里，见[请求与事件总览](../api/index.md)。
