# preferences 模块

请求与事件的规则、连接与认证见 [README](README.md) 与 Ember 的《调试服务》文档。

## 请求

| 请求 | 简介 |
| --- | --- |
| [`preferences.list`](#preferenceslist) | 列出全部分组与设置项（含当前取值） |
| [`preferences.set`](#preferencesset) | 修改一个设置项：核心按登记的种类校验取值，保存到文件，并发出 preferences.changed |
| [`preferences.reset`](#preferencesreset) | 把一个设置项恢复为出厂默认值 |

### preferences.list

列出全部分组与设置项（含当前取值）；分组与设置项按登记的顺序排列。

**参数**

（无）

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `groups` | [preferences.PreferenceGroup](preferences.md#preferencegroup)[] | 是 | 分组 |
| `settings` | [preferences.PreferenceSetting](preferences.md#preferencesetting)[] | 是 | 设置项 |

**错误码**：无专有错误码（参数不合格返回 `core.invalidParams`）

**示例**

```text
ember-debug call preferences.list
```

```ts
await client.api.preferences.list();
```

### preferences.set

修改一个设置项：核心按登记的种类校验取值，保存到文件，并发出 preferences.changed；取值没有变化时不发事件。

**参数**

| 名称 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `id` | `string（长度 1..）` | 是 | 设置项标识 |
| `value` | `任意 JSON` | 是 | 新的取值：choice 为 options 里的字符串，switch 为布尔值，integer 为 minimum 到 maximum 之间的整数 |

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `id` | `string（长度 1..）` | 是 | 设置项标识 |
| `value` | `任意 JSON` | 是 | 修改之后的取值 |

**错误码**：`preferences.unknown`、`preferences.invalidValue`

**示例**

```text
ember-debug call preferences.set id=<string> value=<json>
```

```ts
await client.api.preferences.set({ id: …, value: … });
```

### preferences.reset

把一个设置项恢复为出厂默认值；取值没有变化时不发事件。

**参数**

| 名称 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `id` | `string（长度 1..）` | 是 | 设置项标识 |

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `id` | `string（长度 1..）` | 是 | 设置项标识 |
| `value` | `任意 JSON` | 是 | 恢复之后的取值 |

**错误码**：`preferences.unknown`

**示例**

```text
ember-debug call preferences.reset id=<string>
```

```ts
await client.api.preferences.reset({ id: … });
```

## 事件

事件要先用 `debug.subscribe` 订阅才会收到，以 JSON-RPC 通知送达。

### preferences.changed

一个设置项的取值变了（界面修改、恢复默认、核心里的模块修改都会发出）。

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `id` | `string（长度 1..）` | 是 | 设置项标识 |
| `value` | `任意 JSON` | 是 | 新的取值 |

## 数据类型

### PreferenceGroup

设置项的分组，首选项页左侧的一项。

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `id` | `string（长度 1..）` | 是 | 分组标识，如 appearance |
| `label` | `string（长度 1..）` | 是 | 分组的显示名称 |

### PreferenceOption

选择类设置项的一个可选值。

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `value` | `string（长度 1..）` | 是 | 保存的取值 |
| `label` | `string（长度 1..）` | 是 | 显示名称 |

### PreferenceSetting

一个设置项：登记的内容与当前的取值。

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `id` | `string（长度 1..）` | 是 | 设置项标识，分组标识加点再加名称，如 sound.buffer |
| `group` | `string（长度 1..）` | 是 | 所属分组的标识 |
| `label` | `string（长度 1..）` | 是 | 显示名称 |
| `description` | `string` | 是 | 一两句话的说明 |
| `kind` | `"choice" \| "switch" \| "integer" \| "text"` | 是 | 种类：choice 从 options 里选一个，switch 开关（布尔值），integer 范围内的整数，text 不超过 maxLength 个字节的文字 |
| `options` | [preferences.PreferenceOption](preferences.md#preferenceoption)[] | 否 | choice 的可选值，其他种类没有这一项 |
| `minimum` | `integer` | 否 | integer 的最小值，其他种类没有这一项 |
| `maximum` | `integer` | 否 | integer 的最大值，其他种类没有这一项 |
| `maxLength` | `integer（1..）` | 否 | text 的最大字节数（UTF-8），其他种类没有这一项 |
| `hidden` | `boolean` | 是 | 为 true 时首选项页不显示，只保存取值（如音量） |
| `defaultValue` | `任意 JSON` | 是 | 出厂默认值：choice 为字符串，switch 为布尔值，integer 为整数 |
| `value` | `任意 JSON` | 是 | 当前取值，类型同 defaultValue |
