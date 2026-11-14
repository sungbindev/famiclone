# rpc 模块

请求与事件的规则、连接与认证见 [README](README.md) 与 Ember 的《调试服务》文档。

## 请求

| 请求 | 简介 |
| --- | --- |
| [`rpc.discover`](#rpcdiscover) | 返回本文档 |

### rpc.discover

返回描述全部请求与事件的 OpenRPC 文档，工具可据此生成客户端与界面。

**参数**

（无）

**结果**

`object`

**错误码**：无专有错误码（参数不合格返回 `core.invalidParams`）

**示例**

```text
ember-debug call rpc.discover
```

```ts
await client.api.rpc.discover();
```
