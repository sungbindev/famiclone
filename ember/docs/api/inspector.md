# inspector 模块

请求与事件的规则、连接与认证见 [README](README.md) 与 Ember 的《调试服务》文档。

## 请求

| 请求 | 简介 |
| --- | --- |
| [`inspector.listSpaces`](#inspectorlistspaces) | 列出全部内存空间：总线视角的 cpu 与 ppu，主板与芯片里的存储，以及卡带上的存储区（cart.<存储区标识>） |
| [`inspector.readMemory`](#inspectorreadmemory) | 读一段内存，不改变主机的任何状态：cpu 与 ppu 空间里寄存器类地址读出 0（不读寄存器，所以不会清标志），卡带脚本的寄存器也不会被回调 |
| [`inspector.writeMemory`](#inspectorwritememory) | 写一段内存 |
| [`inspector.resolve`](#inspectorresolve) | 解析一个总线地址：它落在哪块存储区的哪个偏移，或哪个寄存器 |
| [`inspector.getState`](#inspectorgetstate) | 读状态：cpu（寄存器与周期数）、ppu（扫描线、点、帧数、滚动寄存器、标志）与卡带的脚本状态（mapper） |
| [`inspector.snapshot`](#inspectorsnapshot) | 取得完整快照：各模块的状态、断点与监视项，以及此刻的 CPU 周期数，都在同一个时刻读出，是一致的 |
| [`inspector.watch`](#inspectorwatch) | 监视一个状态项：它的取值变化时发事件 inspector.stateChanged |
| [`inspector.unwatch`](#inspectorunwatch) | 取消监视 |
| [`inspector.disassemble`](#inspectordisassemble) | 从 address 起反汇编 count 条指令（CPU 总线上的地址） |
| [`inspector.cdlStart`](#inspectorcdlstart) | 开始代码与数据标记（CDL）：给卡带 PRG-ROM 的每个字节打标记，标记清零后，运行与单步执行的每条指令都记：Code（0x01，被当作指令执行，含操作数字节）、Data（0x02，被指令当数据读取，不含只写的存储指令、栈与中断向量）、JumpTarget（0x04，JMP 或成立的分支的目标）、SubEntry（0x08，JSR 的目标） |
| [`inspector.cdlStop`](#inspectorcdlstop) | 停止标记，已有的标记保留到下一次 cdlStart 或换卡带 |
| [`inspector.cdlSave`](#inspectorcdlsave) | 把当前的标记写成调试输出目录下的文件：PRG-ROM 每个字节一个标记字节，没有文件头，位定义同 Mesen |
| [`inspector.traceStart`](#inspectortracestart) | 开始跟踪，内容记在内存里，traceStop 时写成调试输出目录下的文件 |
| [`inspector.traceStop`](#inspectortracestop) | 结束跟踪并把内容写成文件 |
| [`inspector.callLogStart`](#inspectorcalllogstart) | 开始调用记录：每次执行到 address 这条指令之前记一行，含寄存器、周期数、栈顶的若干字节（SP 之上，栈顶在前）与指定的内存段 |
| [`inspector.callLogStop`](#inspectorcalllogstop) | 结束调用记录并把内容写成文件 |
| [`inspector.addBreakpoint`](#inspectoraddbreakpoint) | 添加断点 |
| [`inspector.listBreakpoints`](#inspectorlistbreakpoints) | 列出全部断点，按编号升序 |
| [`inspector.removeBreakpoint`](#inspectorremovebreakpoint) | 删除一个断点 |
| [`inspector.clearBreakpoints`](#inspectorclearbreakpoints) | 删除全部断点 |
| [`inspector.setState`](#inspectorsetstate) | 修改一个模块的状态：目前只有 cpu 可以改（pc、a、x、y、sp、status，只在指令之间生效），其余模块只读 |

### inspector.listSpaces

> 只经调试服务开放（界面不能发起）

列出全部内存空间：总线视角的 cpu 与 ppu，主板与芯片里的存储，以及卡带上的存储区（cart.<存储区标识>）。需要已开机。

**参数**

（无）

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `spaces` | [inspector.MemorySpaceInfo](inspector.md#memoryspaceinfo)[] | 是 | 内存空间 |

**错误码**：`inspector.noCartridge`、`inspector.notPowered`、`inspector.unsupported`

**示例**

```text
ember-debug call inspector.listSpaces
```

```ts
await client.api.inspector.listSpaces();
```

### inspector.readMemory

> 只经调试服务开放（界面不能发起）

读一段内存，不改变主机的任何状态：cpu 与 ppu 空间里寄存器类地址读出 0（不读寄存器，所以不会清标志），卡带脚本的寄存器也不会被回调。数据以二进制帧返回（结果里的 data，见 docs/04-调试服务.md 第 2.3 节）。需要已开机。

**参数**

| 名称 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `space` | `string（长度 1..64）` | 是 | 空间标识，见 listSpaces |
| `address` | `integer（0..4294967295）` | 是 | 起始地址 |
| `length` | `integer（1..1048576）` | 是 | 要读的字节数 |

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `space` | `string` | 是 | 空间标识 |
| `address` | `integer（0..）` | 是 | 起始地址 |
| `length` | `integer（1..）` | 是 | 字节数 |
| `data` | `二进制` | 否 | 读到的字节（二进制，以二进制帧送达） |

**错误码**：`inspector.noCartridge`、`inspector.notPowered`、`inspector.unsupported`、`inspector.unknownSpace`、`inspector.outOfRange`

**示例**

```text
ember-debug call inspector.readMemory space=<string> address=<integer> length=<integer>
```

```ts
await client.api.inspector.readMemory({ space: …, address: …, length: … });
```

### inspector.writeMemory

> 只经调试服务开放（界面不能发起）

写一段内存。cpu 与 ppu 空间的写经真实的总线，与 CPU 写一样有副作用（写寄存器会触发相应的行为）；卡带的 ROM 存储区不能写。需要已开机。

**参数**

| 名称 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `space` | `string（长度 1..64）` | 是 | 空间标识，见 listSpaces |
| `address` | `integer（0..4294967295）` | 是 | 起始地址 |
| `bytes` | `integer（0..255）[]` | 是 | 要写入的字节，每项 0 到 255 |

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `space` | `string` | 是 | 空间标识 |
| `address` | `integer（0..）` | 是 | 起始地址 |
| `written` | `integer（0..）` | 是 | 写入的字节数 |

**错误码**：`inspector.noCartridge`、`inspector.notPowered`、`inspector.unsupported`、`inspector.unknownSpace`、`inspector.outOfRange`、`inspector.readOnly`

**示例**

```text
ember-debug call inspector.writeMemory space=<string> address=<integer> bytes=<integer>
```

```ts
await client.api.inspector.writeMemory({ space: …, address: …, bytes: … });
```

### inspector.resolve

> 只经调试服务开放（界面不能发起）

解析一个总线地址：它落在哪块存储区的哪个偏移，或哪个寄存器；卡带的地址按当前的窗口映射（分页）解析，不回调脚本。需要已开机。

**参数**

| 名称 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `space` | `"cpu" \| "ppu"` | 是 | 总线：cpu 或 ppu |
| `address` | `integer（0..65535）` | 是 | 总线地址 |

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `target` | [inspector.AddressInfo](inspector.md#addressinfo) | 是 | 解析结果 |

**错误码**：`inspector.noCartridge`、`inspector.notPowered`、`inspector.unsupported`、`inspector.unknownSpace`、`inspector.outOfRange`

**示例**

```text
ember-debug call inspector.resolve space=<"cpu" | "ppu"> address=<integer>
```

```ts
await client.api.inspector.resolve({ space: …, address: … });
```

### inspector.getState

> 只经调试服务开放（界面不能发起）

读状态：cpu（寄存器与周期数）、ppu（扫描线、点、帧数、滚动寄存器、标志）与卡带的脚本状态（mapper）。省略 module 时返回全部模块。读取在帧边界或暂停时进行，是一致的快照。需要已开机。

**参数**

| 名称 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `module` | `string（长度 1..64）` | 否 | 模块标识；省略时返回全部 |

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `states` | [inspector.ModuleState](inspector.md#modulestate)[] | 是 | 各模块的状态 |

**错误码**：`inspector.noCartridge`、`inspector.notPowered`、`inspector.unsupported`、`inspector.invalidState`

**示例**

```text
ember-debug call inspector.getState
```

```ts
await client.api.inspector.getState();
```

### inspector.snapshot

> 只经调试服务开放（界面不能发起）

取得完整快照：各模块的状态、断点与监视项，以及此刻的 CPU 周期数，都在同一个时刻读出，是一致的。内存不在快照里，用 readMemory 读。省略 module 时包含全部模块。需要已开机。

**参数**

| 名称 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `module` | `string（长度 1..64）` | 否 | 只取这个模块的状态；省略时取全部 |

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `cycles` | `integer（0..）` | 是 | 自上电以来的 CPU 周期数 |
| `states` | [inspector.ModuleState](inspector.md#modulestate)[] | 是 | 各模块的状态 |
| `breakpoints` | [inspector.Breakpoint](inspector.md#breakpoint)[] | 是 | 全部断点 |
| `watches` | [inspector.WatchedValue](inspector.md#watchedvalue)[] | 是 | 全部监视项与当前取值 |

**错误码**：`inspector.noCartridge`、`inspector.notPowered`、`inspector.unsupported`、`inspector.invalidState`

**示例**

```text
ember-debug call inspector.snapshot
```

```ts
await client.api.inspector.snapshot();
```

### inspector.watch

> 只经调试服务开放（界面不能发起）

监视一个状态项：它的取值变化时发事件 inspector.stateChanged。路径是 <模块>.<状态项>，如 cpu.pc、ppu.scanline、mapper.prgBank（状态项见 getState）。取值在每一帧完成后、单步、命中断点和检查请求执行完之后检查，一帧里变化多次只报告最后的取值。已经在监视的路径再监视没有效果。监视最多 64 项，属于检查器本身，不随卡带更换清除。需要已开机。

**参数**

| 名称 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `path` | `string（长度 3..128）` | 是 | 状态路径 |

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `watch` | [inspector.WatchedValue](inspector.md#watchedvalue) | 是 | 监视项与当前取值 |

**错误码**：`inspector.noCartridge`、`inspector.notPowered`、`inspector.unsupported`、`inspector.invalidState`、`inspector.watchLimit`

**示例**

```text
ember-debug call inspector.watch path=<string>
```

```ts
await client.api.inspector.watch({ path: … });
```

### inspector.unwatch

> 只经调试服务开放（界面不能发起）

取消监视。没有监视这个路径时返回 inspector.noWatch。需要已开机。

**参数**

| 名称 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `path` | `string（长度 3..128）` | 是 | 状态路径 |

**结果**

（无）

**错误码**：`inspector.noCartridge`、`inspector.notPowered`、`inspector.unsupported`、`inspector.noWatch`

**示例**

```text
ember-debug call inspector.unwatch path=<string>
```

```ts
await client.api.inspector.unwatch({ path: … });
```

### inspector.disassemble

> 只经调试服务开放（界面不能发起）

从 address 起反汇编 count 条指令（CPU 总线上的地址）。读内存没有副作用；带索引的寻址用当前的 X、Y 算有效地址与注释里的取值；地址超过 $FFFF 时回到 $0000。需要已开机。

**参数**

| 名称 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `address` | `integer（0..65535）` | 是 | 起始地址 |
| `count` | `integer（1..256）` | 否 | 指令条数，默认 16 |

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `instructions` | [inspector.DisassembledInstruction](inspector.md#disassembledinstruction)[] | 是 | 反汇编的指令 |

**错误码**：`inspector.noCartridge`、`inspector.notPowered`、`inspector.unsupported`

**示例**

```text
ember-debug call inspector.disassemble address=<integer>
```

```ts
await client.api.inspector.disassemble({ address: … });
```

### inspector.cdlStart

> 只经调试服务开放（界面不能发起）

开始代码与数据标记（CDL）：给卡带 PRG-ROM 的每个字节打标记，标记清零后，运行与单步执行的每条指令都记：Code（0x01，被当作指令执行，含操作数字节）、Data（0x02，被指令当数据读取，不含只写的存储指令、栈与中断向量）、JumpTarget（0x04，JMP 或成立的分支的目标）、SubEntry（0x08，JSR 的目标）。标记的是物理位置（按当前的分页换算）。记录属于当前的主机，装入新卡带后作废。需要已开机，卡带要有 PRG-ROM 存储区。

**参数**

（无）

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `size` | `integer（1..）` | 是 | PRG-ROM 的字节数，也是标记的字节数 |

**错误码**：`inspector.noCartridge`、`inspector.notPowered`、`inspector.unsupported`、`inspector.cdlActive`

**示例**

```text
ember-debug call inspector.cdlStart
```

```ts
await client.api.inspector.cdlStart();
```

### inspector.cdlStop

> 只经调试服务开放（界面不能发起）

停止标记，已有的标记保留到下一次 cdlStart 或换卡带；已经停止时只返回统计。没有记录过时返回 inspector.noCdl。需要已开机。

**参数**

（无）

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `size` | `integer（1..）` | 是 | PRG-ROM 的字节数 |
| `codeBytes` | `integer（0..）` | 是 | 标了 Code 的字节数 |
| `dataBytes` | `integer（0..）` | 是 | 标了 Data 的字节数 |

**错误码**：`inspector.noCartridge`、`inspector.notPowered`、`inspector.unsupported`、`inspector.noCdl`

**示例**

```text
ember-debug call inspector.cdlStop
```

```ts
await client.api.inspector.cdlStop();
```

### inspector.cdlSave

> 只经调试服务开放（界面不能发起）

把当前的标记写成调试输出目录下的文件：PRG-ROM 每个字节一个标记字节，没有文件头，位定义同 Mesen。记录中或停止后都可以保存。没有记录过时返回 inspector.noCdl。需要已开机。

**参数**

| 名称 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `file` | `string（长度 1..100）` | 否 | 文件名，只能含字母、数字、点、下划线、短横线与空格，不含目录；省略时用 prg.cdl |

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `file` | `string` | 是 | 保存的文件的完整路径 |
| `size` | `integer（1..）` | 是 | PRG-ROM 的字节数，也是文件的字节数 |
| `codeBytes` | `integer（0..）` | 是 | 标了 Code 的字节数 |
| `dataBytes` | `integer（0..）` | 是 | 标了 Data 的字节数 |

**错误码**：`inspector.noCartridge`、`inspector.notPowered`、`inspector.unsupported`、`inspector.noCdl`、`inspector.invalidFileName`、`inspector.fileFailed`

**示例**

```text
ember-debug call inspector.cdlSave
```

```ts
await client.api.inspector.cdlSave();
```

### inspector.traceStart

> 只经调试服务开放（界面不能发起）

开始跟踪，内容记在内存里，traceStop 时写成调试输出目录下的文件。kind：instruction 每执行一条指令记一行（地址、指令字节、反汇编、寄存器与周期数，写法同 nestest 日志；芯片寄存器的取值显示 00）；io 每次访问 $2000-$5FFF（芯片、手柄口、卡带扩展寄存器）以及对 $8000 以上的写（mapper 寄存器）记一行（方向、地址、值、发起访问的指令地址与周期数）；irq 每次 CPU 响应 IRQ、NMI 或 BRK 记一行（种类、返回地址、处理程序地址、周期数）。单步与运行产生的内容都记，调试写内存不记。同时进行的记录最多 8 项，记录属于当前的主机，装入新卡带后作废。需要已开机。

**参数**

| 名称 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `kind` | `"instruction" \| "io" \| "irq"` | 是 | 跟踪的种类 |
| `file` | `string（长度 1..100）` | 否 | 文件名，只能含字母、数字、点、下划线、短横线与空格，不含目录；省略时按种类与编号生成（trace-编号.txt、calllog-编号.txt）。文件写在调试输出目录 |
| `maxLines` | `integer（1..1000000）` | 否 | 最多记录的行数，满了就不再记录，默认 100000 |
| `unique` | `boolean` | 否 | 只对 instruction 有效：每个地址只记第一次，默认 false |

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `id` | `integer（1..）` | 是 | 记录编号 |

**错误码**：`inspector.noCartridge`、`inspector.notPowered`、`inspector.unsupported`、`inspector.invalidFileName`、`inspector.recordingLimit`

**示例**

```text
ember-debug call inspector.traceStart kind=<"instruction" | "io" | "irq">
```

```ts
await client.api.inspector.traceStart({ kind: … });
```

### inspector.traceStop

> 只经调试服务开放（界面不能发起）

结束跟踪并把内容写成文件。需要已开机。

**参数**

| 名称 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `id` | `integer（1..）` | 是 | traceStart 返回的记录编号 |

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `file` | `string` | 是 | 保存的文件的完整路径 |
| `lines` | `integer（0..）` | 是 | 写入的行数 |
| `truncated` | `boolean` | 是 | 是否因为行数满了而少记了 |

**错误码**：`inspector.noCartridge`、`inspector.notPowered`、`inspector.unsupported`、`inspector.noRecording`、`inspector.fileFailed`

**示例**

```text
ember-debug call inspector.traceStop id=<integer>
```

```ts
await client.api.inspector.traceStop({ id: … });
```

### inspector.callLogStart

> 只经调试服务开放（界面不能发起）

开始调用记录：每次执行到 address 这条指令之前记一行，含寄存器、周期数、栈顶的若干字节（SP 之上，栈顶在前）与指定的内存段。内容记在内存里，callLogStop 时写成调试输出目录下的文件。同时进行的记录最多 8 项，记录属于当前的主机，装入新卡带后作废。需要已开机。

**参数**

| 名称 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `address` | `integer（0..65535）` | 是 | 要记录的指令地址 |
| `stackBytes` | `integer（0..32）` | 否 | 每次记录栈顶的字节数，默认 0 |
| `ranges` | [inspector.MemoryRangeSpec](inspector.md#memoryrangespec)[] | 否 | 每次记录的内存段，最多 4 段 |
| `file` | `string（长度 1..100）` | 否 | 文件名，只能含字母、数字、点、下划线、短横线与空格，不含目录；省略时按种类与编号生成（trace-编号.txt、calllog-编号.txt）。文件写在调试输出目录 |
| `maxLines` | `integer（1..1000000）` | 否 | 最多记录的行数，满了就不再记录，默认 100000 |

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `id` | `integer（1..）` | 是 | 记录编号 |

**错误码**：`inspector.noCartridge`、`inspector.notPowered`、`inspector.unsupported`、`inspector.invalidFileName`、`inspector.recordingLimit`

**示例**

```text
ember-debug call inspector.callLogStart address=<integer>
```

```ts
await client.api.inspector.callLogStart({ address: … });
```

### inspector.callLogStop

> 只经调试服务开放（界面不能发起）

结束调用记录并把内容写成文件。需要已开机。

**参数**

| 名称 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `id` | `integer（1..）` | 是 | callLogStart 返回的记录编号 |

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `file` | `string` | 是 | 保存的文件的完整路径 |
| `lines` | `integer（0..）` | 是 | 写入的行数 |
| `truncated` | `boolean` | 是 | 是否因为行数满了而少记了 |

**错误码**：`inspector.noCartridge`、`inspector.notPowered`、`inspector.unsupported`、`inspector.noRecording`、`inspector.fileFailed`

**示例**

```text
ember-debug call inspector.callLogStop id=<integer>
```

```ts
await client.api.inspector.callLogStop({ id: … });
```

### inspector.addBreakpoint

> 只经调试服务开放（界面不能发起）

添加断点。断点在机器运行时生效（自由运行与 runtime.run 都会停下，发事件 runtime.breakpointHit，运行模式变为暂停）；单步（runtime.step）忽略断点；继续运行时会先越过刚命中的执行断点。断点属于当前的主机，装入新卡带后清空。地址是 CPU 总线上的地址。需要已开机。

**参数**

| 名称 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `kind` | `"execute" \| "read" \| "write"` | 是 | 种类 |
| `address` | `integer（0..65535）` | 是 | 地址，或地址范围的第一个地址 |
| `endAddress` | `integer（0..65535）` | 否 | 地址范围的最后一个地址（含），省略时只有 address 一个地址 |

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `breakpoint` | [inspector.Breakpoint](inspector.md#breakpoint) | 是 | 添加的断点 |

**错误码**：`inspector.noCartridge`、`inspector.notPowered`、`inspector.unsupported`、`inspector.invalidBreakpoint`、`inspector.breakpointLimit`

**示例**

```text
ember-debug call inspector.addBreakpoint kind=<"execute" | "read" | "write"> address=<integer>
```

```ts
await client.api.inspector.addBreakpoint({ kind: …, address: … });
```

### inspector.listBreakpoints

> 只经调试服务开放（界面不能发起）

列出全部断点，按编号升序。需要已开机。

**参数**

（无）

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `breakpoints` | [inspector.Breakpoint](inspector.md#breakpoint)[] | 是 | 断点 |

**错误码**：`inspector.noCartridge`、`inspector.notPowered`、`inspector.unsupported`

**示例**

```text
ember-debug call inspector.listBreakpoints
```

```ts
await client.api.inspector.listBreakpoints();
```

### inspector.removeBreakpoint

> 只经调试服务开放（界面不能发起）

删除一个断点。需要已开机。

**参数**

| 名称 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `id` | `integer（1..）` | 是 | 断点编号 |

**结果**

（无）

**错误码**：`inspector.noCartridge`、`inspector.notPowered`、`inspector.unsupported`、`inspector.noBreakpoint`

**示例**

```text
ember-debug call inspector.removeBreakpoint id=<integer>
```

```ts
await client.api.inspector.removeBreakpoint({ id: … });
```

### inspector.clearBreakpoints

> 只经调试服务开放（界面不能发起）

删除全部断点。需要已开机。

**参数**

（无）

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `removed` | `integer（0..）` | 是 | 删除的个数 |

**错误码**：`inspector.noCartridge`、`inspector.notPowered`、`inspector.unsupported`

**示例**

```text
ember-debug call inspector.clearBreakpoints
```

```ts
await client.api.inspector.clearBreakpoints();
```

### inspector.setState

> 只经调试服务开放（界面不能发起）

修改一个模块的状态：目前只有 cpu 可以改（pc、a、x、y、sp、status，只在指令之间生效），其余模块只读。先全部检查，有任何一项不合格就一项也不改。需要已开机。

**参数**

| 名称 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `module` | `string（长度 1..64）` | 是 | 模块标识 |
| `fields` | [inspector.StateField](inspector.md#statefield)[] | 是 | 要修改的状态项 |

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `state` | [inspector.ModuleState](inspector.md#modulestate) | 是 | 修改之后的状态 |

**错误码**：`inspector.noCartridge`、`inspector.notPowered`、`inspector.unsupported`、`inspector.readOnly`、`inspector.invalidState`

**示例**

```text
ember-debug call inspector.setState module=<string> fields=<inspector.StateField[]>
```

```ts
await client.api.inspector.setState({ module: …, fields: … });
```

## 事件

事件要先用 `debug.subscribe` 订阅才会收到，以 JSON-RPC 通知送达。

### inspector.stateChanged

监视的状态项取值变了。同一次检查里变化的各项依次发出；监视项属于检查器，重新上电或换卡带之后的变化也会报告。

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `path` | `string` | 是 | 状态路径 |
| `oldValue` | `integer` | 是 | 上一次检查时的取值 |
| `newValue` | `integer` | 是 | 现在的取值 |
| `cycles` | `integer（0..）` | 是 | 检查时的 CPU 周期数 |

## 数据类型

### MemorySpaceInfo

一个内存空间。

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `id` | `string` | 是 | 空间标识：cpu、ppu（总线视角，读不产生副作用）；board.ram 等主板上的存储；chip.oam、chip.palette 等芯片内的存储；cart.<存储区标识> 卡带上的存储区 |
| `description` | `string` | 是 | 说明 |
| `size` | `integer（1..）` | 是 | 大小（字节），地址从 0 到大小减 1 |
| `writable` | `boolean` | 是 | 能否写入；卡带的 ROM 存储区不能写 |

### AddressInfo

一个总线地址对应的东西。

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `kind` | `"ram" \| "rom" \| "register" \| "none"` | 是 | 种类：ram 与 rom 是存储区里的字节；register 是芯片、设备或 mapper 脚本的寄存器；none 没有设备响应（读到总线上最近的值） |
| `region` | `string` | 否 | 所在存储区的空间标识（如 board.ram、cart.prg），或寄存器所属的芯片（如 chip.ppu、cart.mapper）；没有时省略 |
| `offset` | `integer（0..）` | 否 | 在存储区里的偏移，或在寄存器组里的序号；没有时省略 |
| `description` | `string` | 是 | 说明 |

### StateField

一项状态。

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `name` | `string（长度 1..64）` | 是 | 状态项名称 |
| `value` | `integer` | 是 | 取值；标志位是 0 或 1 |
| `description` | `string` | 否 | 说明（读出时有，写入时可省略） |

### Breakpoint

一个断点。

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `id` | `integer（1..）` | 是 | 断点编号 |
| `kind` | `"execute" \| "read" \| "write"` | 是 | 种类：execute CPU 要执行这个地址的指令之前停下；read、write 总线读写这个范围里的地址，那条指令执行完后停下 |
| `address` | `integer（0..65535）` | 是 | 地址范围的第一个地址 |
| `endAddress` | `integer（0..65535）` | 是 | 地址范围的最后一个地址（含）；单个地址时与 address 相同 |
| `hits` | `integer（0..）` | 是 | 已经命中的次数 |

### DisassembledInstruction

一条反汇编的指令。

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `address` | `integer（0..65535）` | 是 | 指令的地址 |
| `bytes` | `string` | 是 | 指令字节，十六进制，空格隔开，如 A9 42 |
| `text` | `string` | 是 | 助记符与操作数，带有效地址与取值的注释，如 LDA $0180 = 33；写法同 nestest 日志，芯片寄存器类地址的取值显示 00 |
| `unofficial` | `boolean` | 是 | 是不是非官方指令 |

### MemoryRangeSpec

一段内存，CPU 总线上的地址。

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `address` | `integer（0..65535）` | 是 | 起始地址 |
| `length` | `integer（1..256）` | 是 | 字节数 |

### WatchedValue

一个监视中的状态项。

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `path` | `string` | 是 | 状态路径，如 cpu.pc、ppu.scanline、mapper.prgBank |
| `value` | `integer` | 是 | 当前取值 |

### ModuleState

一个模块的状态。

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `module` | `string` | 是 | 模块标识：cpu、ppu；卡带的脚本状态是 mapper |
| `writable` | `boolean` | 是 | 状态是否可以用 setState 修改（cpu 的 cycles 除外） |
| `fields` | [inspector.StateField](inspector.md#statefield)[] | 是 | 状态项 |
