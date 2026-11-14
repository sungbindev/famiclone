# system 模块

请求与事件的规则、连接与认证见 [README](README.md) 与 Ember 的《调试服务》文档。

## 请求

| 请求 | 简介 |
| --- | --- |
| [`system.version`](#systemversion) | 返回应用版本与核心版本 |

### system.version

返回应用版本与核心版本。

**参数**

（无）

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `appVersion` | `string（长度 1..）` | 是 | 应用版本，取自安装包的版本号 |
| `coreVersion` | `string（长度 1..）` | 是 | 核心版本，取自核心的构建参数 |

**错误码**：无专有错误码（参数不合格返回 `core.invalidParams`）

**示例**

```text
ember-debug call system.version
```

```ts
await client.api.system.version();
```
