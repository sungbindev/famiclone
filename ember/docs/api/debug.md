# debug 模块

请求与事件的规则、连接与认证见 [README](README.md) 与 Ember 的《调试服务》文档。

## 请求

| 请求 | 简介 |
| --- | --- |
| [`debug.hello`](#debughello) | 握手：每条连接上第一个请求必须是它，令牌不对时连接被关闭（关闭码 4001） |
| [`debug.subscribe`](#debugsubscribe) | 订阅事件 |
| [`debug.unsubscribe`](#debugunsubscribe) | 取消订阅：写法要与订阅时一致 |
| [`debug.status`](#debugstatus) | 调试服务的状态：是否开启、是否在监听、端口与当前的连接 |

### debug.hello

> 只经调试服务开放（界面不能发起）

握手：每条连接上第一个请求必须是它，令牌不对时连接被关闭（关闭码 4001）。成功后才能调用其他请求。

**参数**

| 名称 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `client` | `string（长度 1..64）` | 是 | 客户端名称，如 copilot、ember-debug-cli |
| `token` | `string（长度 1..128）` | 是 | 调试服务的令牌 |
| `protocol` | `integer（1..1）` | 否 | 客户端使用的协议版本，目前只有 1；省略时按 1 |

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `protocol` | `integer（1..）` | 是 | 服务端的协议版本 |
| `coreVersion` | `string` | 是 | 核心版本 |
| `appVersion` | `string` | 是 | 应用版本 |
| `session` | `integer（1..）` | 是 | 本连接的编号 |

**错误码**：`debug.unauthorized`、`debug.unsupportedProtocol`

**示例**

```text
ember-debug call debug.hello client=<string> token=<string>
```

```ts
await client.api.debug.hello({ client: …, token: … });
```

### debug.subscribe

> 只经调试服务开放（界面不能发起）

订阅事件。事件名写成“模块.事件”，也可以写“模块.*”订阅一个模块的全部事件，或“*”订阅全部；已经订阅的重复订阅没有效果。高频事件默认不推送，要订阅才有。

**参数**

| 名称 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `events` | `string（长度 1..64）[]` | 是 | 事件名或通配写法 |

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `events` | `string[]` | 是 | 订阅之后本连接订阅的全部事件名或通配写法 |

**错误码**：无专有错误码（参数不合格返回 `core.invalidParams`）

**示例**

```text
ember-debug call debug.subscribe events=<string>
```

```ts
await client.api.debug.subscribe({ events: … });
```

### debug.unsubscribe

> 只经调试服务开放（界面不能发起）

取消订阅：写法要与订阅时一致；没有订阅过的忽略。

**参数**

| 名称 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `events` | `string（长度 1..64）[]` | 是 | 事件名或通配写法 |

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `events` | `string[]` | 是 | 取消之后本连接订阅的全部事件名或通配写法 |

**错误码**：无专有错误码（参数不合格返回 `core.invalidParams`）

**示例**

```text
ember-debug call debug.unsubscribe events=<string>
```

```ts
await client.api.debug.unsubscribe({ events: … });
```

### debug.status

调试服务的状态：是否开启、是否在监听、端口与当前的连接。

**参数**

（无）

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `enabled` | `boolean` | 是 | 是否开启（首选项里打开，或启动参数 --debug-port 指定） |
| `listening` | `boolean` | 是 | 是否正在监听；开启但端口被占用时为 false |
| `port` | `integer（0..65535）` | 是 | 监听的端口，没有监听时为 0 |
| `error` | `string` | 否 | 开启了但没有监听时的原因；其他情况没有这一项 |
| `connections` | [debug.DebugConnection](debug.md#debugconnection)[] | 是 | 当前的连接 |

**错误码**：无专有错误码（参数不合格返回 `core.invalidParams`）

**示例**

```text
ember-debug call debug.status
```

```ts
await client.api.debug.status();
```

## 事件

事件要先用 `debug.subscribe` 订阅才会收到，以 JSON-RPC 通知送达。

### debug.statusChanged

服务的状态变了：开始或停止监听、有连接建立或断开。

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `enabled` | `boolean` | 是 | 是否开启 |
| `listening` | `boolean` | 是 | 是否正在监听 |
| `port` | `integer（0..65535）` | 是 | 监听的端口，没有监听时为 0 |
| `connections` | `integer（0..）` | 是 | 当前的连接数 |

### debug.eventsDropped

只发给订阅事件的那条连接：客户端读得太慢，事件队列已满，丢弃了若干条事件；响应不会被丢弃。

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `count` | `integer（1..）` | 是 | 自上一次通知以来丢弃的事件条数 |

## 数据类型

### DebugConnection

一个已经通过握手的调试连接。

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `id` | `integer（1..）` | 是 | 连接编号，本次运行内唯一 |
| `client` | `string` | 是 | 客户端在 hello 里自报的名称 |
| `connectedAt` | `string` | 是 | 握手完成的时间，UTC，ISO 8601 |
