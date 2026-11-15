# machines 模块

请求与事件的规则、连接与认证见 [README](README.md) 与 Ember 的《调试服务》文档。

## 请求

| 请求 | 简介 |
| --- | --- |
| [`machines.list`](#machineslist) | 机型清单，顺序固定为 fc、sb97、sb2000、bbk10、bbk20 |
| [`machines.setBios`](#machinessetbios) | 读取并检查 BIOS 文件，复制进 BIOS 目录（新文件写好之后才删旧文件，失败时原有的 BIOS 不变） |
| [`machines.removeBios`](#machinesremovebios) | 删除机型的 BIOS 文件 |
| [`machines.boot`](#machinesboot) | 整机机型开机：读入已设置的 BIOS，装配该机型的主机，已有的卡带与主机被替换，任何一步失败都不改动原有的 |

### machines.list

机型清单，顺序固定为 fc、sb97、sb2000、bbk10、bbk20。

**参数**

（无）

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `machines` | [machines.MachineInfo](machines.md#machineinfo)[] | 是 | 机型项 |

**错误码**：无专有错误码（参数不合格返回 `core.invalidParams`）

**示例**

```text
ember-debug call machines.list
```

```ts
await client.api.machines.list();
```

### machines.setBios

> 界面里只能由主进程发起，调试服务可以调用

读取并检查 BIOS 文件，复制进 BIOS 目录（新文件写好之后才删旧文件，失败时原有的 BIOS 不变）；成功后发 machines.changed。

**参数**

| 名称 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `machine` | `string（长度 1..）` | 是 | 机型标识 |
| `path` | `string（长度 1..）` | 是 | BIOS 文件的路径，由主进程根据用户选择的文件给出，调试服务直接给出 |

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `machine` | [machines.MachineInfo](machines.md#machineinfo) | 是 | 更新后的机型项 |

**错误码**：`machines.notFound`、`machines.biosNotNeeded`、`machines.unsupported`、`machines.fileNotFound`、`machines.fileUnreadable`、`machines.biosInvalid`、`machines.biosAccessFailed`

**示例**

```text
ember-debug call machines.setBios machine=<string> path=<string>
```

```ts
await client.api.machines.setBios({ machine: …, path: … });
```

### machines.removeBios

删除机型的 BIOS 文件；没有 BIOS 时没有效果也不报错；删除后发 machines.changed。

**参数**

| 名称 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `machine` | `string（长度 1..）` | 是 | 机型标识 |

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `machine` | [machines.MachineInfo](machines.md#machineinfo) | 是 | 更新后的机型项 |

**错误码**：`machines.notFound`、`machines.biosNotNeeded`、`machines.biosAccessFailed`

**示例**

```text
ember-debug call machines.removeBios machine=<string>
```

```ts
await client.api.machines.removeBios({ machine: … });
```

### machines.boot

整机机型开机：读入已设置的 BIOS，装配该机型的主机，已有的卡带与主机被替换，任何一步失败都不改动原有的；成功后发 runtime.stateChanged（原因 load），不发 cartridges.loaded。

**参数**

| 名称 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `machine` | `string（长度 1..）` | 是 | 机型标识 |
| `powerOn` | `boolean` | 否 | 开机后是否上电，默认 true |
| `resume` | `boolean` | 否 | 上电后是否直接运行，默认 true；powerOn 为 false 时无效 |

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `machine` | [machines.MachineInfo](machines.md#machineinfo) | 是 | 开机的机型项 |
| `state` | [runtime.RuntimeState](runtime.md#runtimestate) | 是 |  |

**错误码**：`machines.notFound`、`machines.cartridgeRequired`、`machines.unsupported`、`machines.noBios`、`machines.biosInvalid`、`machines.biosAccessFailed`

**示例**

```text
ember-debug call machines.boot machine=<string>
```

```ts
await client.api.machines.boot({ machine: … });
```

## 事件

事件要先用 `debug.subscribe` 订阅才会收到，以 JSON-RPC 通知送达。

### machines.changed

整机机型的 BIOS 添加、替换或移除。

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `machine` | `string` | 是 | 机型标识 |

## 数据类型

### BiosFile

已添加的 BIOS 文件：文件名与大小。

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `fileName` | `string` | 是 | 文件名，不含路径 |
| `size` | `integer（0..）` | 是 | 文件大小（字节），含 iNES 文件头 |

### MachineInfo

一个机型：标识、显示名称、是否需要 BIOS、是否已支持与已添加的 BIOS。

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `id` | `string` | 是 | 机型标识：fc、sb97、sb2000、bbk10、bbk20 |
| `name` | `string` | 是 | 显示名称 |
| `needsBios` | `boolean` | 是 | 是否需要 BIOS；fc 为 false，四个整机机型为 true |
| `supported` | `boolean` | 是 | 是否已支持；未支持的机型不能添加 BIOS，也不能开机 |
| `bios` | [machines.BiosFile](machines.md#biosfile) | 否 | 已添加的 BIOS；没有添加、或不需要 BIOS 时没有这一项 |
