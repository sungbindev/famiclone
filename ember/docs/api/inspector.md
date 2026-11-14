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

### ModuleState

一个模块的状态。

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `module` | `string` | 是 | 模块标识：cpu、ppu；卡带的脚本状态是 mapper |
| `writable` | `boolean` | 是 | 状态是否可以用 setState 修改（cpu 的 cycles 除外） |
| `fields` | [inspector.StateField](inspector.md#statefield)[] | 是 | 状态项 |
