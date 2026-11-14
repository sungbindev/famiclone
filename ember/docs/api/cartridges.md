# cartridges 模块

请求与事件的规则、连接与认证见 [README](README.md) 与 Ember 的《调试服务》文档。

## 请求

| 请求 | 简介 |
| --- | --- |
| [`cartridges.load`](#cartridgesload) | 读取 NES 文件，解析文件头，按 mapper 号与变体匹配插件，装配主机并插入卡槽 |
| [`cartridges.eject`](#cartridgeseject) | 取出卡带：主机随之拆除，电源状态变为断电 |
| [`cartridges.info`](#cartridgesinfo) | 当前卡带的描述、匹配到的插件与存储区清单 |

### cartridges.load

> 界面里只能由主进程发起，调试服务可以调用

读取 NES 文件，解析文件头，按 mapper 号与变体匹配插件，装配主机并插入卡槽；已有的卡带与主机被替换，失败时原有的保持不变。匹配失败的错误码是 mappers.noMatch、mappers.conflict、mappers.hostUnknown，插件加载失败的是 mappers.notFound、mappers.invalidPackage、mappers.limitExceeded。

**参数**

| 名称 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `path` | `string（长度 1..）` | 是 | NES 文件的路径，由主进程根据用户选择的文件给出 |
| `powerOn` | `boolean` | 否 | 装入后是否上电，默认 true |
| `resume` | `boolean` | 否 | 上电后是否直接运行，默认 true；powerOn 为 false 时无效 |

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `cartridge` | [runtime.CartridgeSummary](runtime.md#cartridgesummary) | 是 |  |
| `state` | [runtime.RuntimeState](runtime.md#runtimestate) | 是 |  |

**错误码**：`cartridges.fileNotFound`、`cartridges.fileUnreadable`、`cartridges.notInes`、`cartridges.truncated`、`cartridges.unsupportedFormat`、`cartridges.invalidHeader`

**示例**

```text
ember-debug call cartridges.load path=<string>
```

```ts
await client.api.cartridges.load({ path: … });
```

### cartridges.eject

取出卡带：主机随之拆除，电源状态变为断电；没有卡带时没有效果。

**参数**

（无）

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `state` | [runtime.RuntimeState](runtime.md#runtimestate) | 是 |  |

**错误码**：无专有错误码（参数不合格返回 `core.invalidParams`）

**示例**

```text
ember-debug call cartridges.eject
```

```ts
await client.api.cartridges.eject();
```

### cartridges.info

当前卡带的描述、匹配到的插件与存储区清单。

**参数**

（无）

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `cartridge` | [runtime.CartridgeSummary](runtime.md#cartridgesummary) | 是 |  |

**错误码**：`cartridges.empty`

**示例**

```text
ember-debug call cartridges.info
```

```ts
await client.api.cartridges.info();
```

## 事件

事件要先用 `debug.subscribe` 订阅才会收到，以 JSON-RPC 通知送达。

### cartridges.loaded

卡带装配完成。

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `cartridge` | [runtime.CartridgeSummary](runtime.md#cartridgesummary) | 是 |  |

## 数据类型

### RecentCartridge

最近打开的清单里的一项：给界面看的显示名与编号，不含路径。

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `id` | `integer（1..）` | 是 | 编号：本次运行内有效，用于 openRecent |
| `name` | `string` | 是 | 显示名：文件名 |

### LoadedCartridge

加载完成的卡带与加载后的运行状态。

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `cartridge` | [runtime.CartridgeSummary](runtime.md#cartridgesummary) | 是 |  |
| `state` | [runtime.RuntimeState](runtime.md#runtimestate) | 是 |  |
