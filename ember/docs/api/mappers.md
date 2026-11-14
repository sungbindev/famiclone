# mappers 模块

请求与事件的规则、连接与认证见 [README](README.md) 与 Ember 的《调试服务》文档。

## 请求

| 请求 | 简介 |
| --- | --- |
| [`mappers.list`](#mapperslist) | 列出生效的插件：同一标识有多个来源时只列版本最高的，版本相同时用户安装的优先 |
| [`mappers.info`](#mappersinfo) | 返回一个插件的摘要与清单详情 |
| [`mappers.install`](#mappersinstall) | 安装一个 .emap 插件包：检查通过后放进用户插件目录 |
| [`mappers.uninstall`](#mappersuninstall) | 卸载用户安装的插件 |
| [`mappers.setEnabled`](#mapperssetenabled) | 启用或停用一个插件 |
| [`mappers.reload`](#mappersreload) | 重新读取预装目录与用户目录里的插件包，下次装配或硬复位时生效 |

### mappers.list

列出生效的插件：同一标识有多个来源时只列版本最高的，版本相同时用户安装的优先。

**参数**

（无）

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `plugins` | [mappers.MapperSummary](mappers.md#mappersummary)[] | 是 | 插件摘要，按标识排序 |

**错误码**：无专有错误码（参数不合格返回 `core.invalidParams`）

**示例**

```text
ember-debug call mappers.list
```

```ts
await client.api.mappers.list();
```

### mappers.info

返回一个插件的摘要与清单详情。

**参数**

| 名称 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `id` | `string（长度 1..40）` | 是 | 插件标识 |

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `plugin` | [mappers.MapperDetail](mappers.md#mapperdetail) | 是 |  |

**错误码**：`mappers.notFound`

**示例**

```text
ember-debug call mappers.info id=<string>
```

```ts
await client.api.mappers.info({ id: … });
```

### mappers.install

> 界面里只能由主进程发起，调试服务可以调用

安装一个 .emap 插件包：检查通过后放进用户插件目录；已有同标识的用户插件时替换。

**参数**

| 名称 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `path` | `string（长度 1..）` | 是 | 插件包的文件路径，由主进程根据用户选择的文件给出 |

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `plugin` | [mappers.MapperSummary](mappers.md#mappersummary) | 是 |  |

**错误码**：`mappers.invalidPackage`、`mappers.hostUnknown`、`mappers.limitExceeded`

**示例**

```text
ember-debug call mappers.install path=<string>
```

```ts
await client.api.mappers.install({ path: … });
```

### mappers.uninstall

卸载用户安装的插件；预装的插件只能停用。

**参数**

| 名称 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `id` | `string（长度 1..40）` | 是 | 插件标识 |

**结果**

（无）

**错误码**：`mappers.notFound`、`mappers.bundledReadonly`

**示例**

```text
ember-debug call mappers.uninstall id=<string>
```

```ts
await client.api.mappers.uninstall({ id: … });
```

### mappers.setEnabled

启用或停用一个插件；停用的插件不参与匹配。

**参数**

| 名称 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `id` | `string（长度 1..40）` | 是 | 插件标识 |
| `enabled` | `boolean` | 是 | 是否启用 |

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `plugin` | [mappers.MapperSummary](mappers.md#mappersummary) | 是 |  |

**错误码**：`mappers.notFound`

**示例**

```text
ember-debug call mappers.setEnabled id=<string> enabled=<boolean>
```

```ts
await client.api.mappers.setEnabled({ id: …, enabled: … });
```

### mappers.reload

重新读取预装目录与用户目录里的插件包，下次装配或硬复位时生效；不合格的包被跳过并写入错误日志。

**参数**

（无）

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `plugins` | [mappers.MapperSummary](mappers.md#mappersummary)[] | 是 | 重新加载后生效的插件摘要 |

**错误码**：无专有错误码（参数不合格返回 `core.invalidParams`）

**示例**

```text
ember-debug call mappers.reload
```

```ts
await client.api.mappers.reload();
```

## 事件

事件要先用 `debug.subscribe` 订阅才会收到，以 JSON-RPC 通知送达。

### mappers.changed

插件安装、卸载、启停或重新加载。

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `id` | `string \| null` | 是 | 变化的插件标识；重新加载时为 null |

## 数据类型

### MapperMatchRule

一项匹配条件：mapper 号必写，其余可选；写出的条件必须全部满足。

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `mapper` | `integer（0..4095）` | 是 | mapper 号 |
| `variant` | `integer（0..255）` | 否 | 变体 |
| `prgSize` | `integer（0..）` | 否 | PRG 的字节数 |
| `chrSize` | `integer（0..）` | 否 | CHR 的字节数 |
| `crc32Prg` | `string（长度 8..8）` | 否 | PRG 的 CRC-32，8 位十六进制 |
| `crc32All` | `string（长度 8..8）` | 否 | PRG 加 CHR 的 CRC-32，8 位十六进制 |

### MapperSummary

插件摘要，列表里的一行。

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `id` | `string（长度 3..40）` | 是 | 插件标识 |
| `name` | `string（长度 1..40）` | 是 | 显示名称 |
| `version` | `string（长度 5..32）` | 是 | 插件版本，主.次.修订 |
| `publisher` | `string（长度 1..40）` | 是 | 发布者 |
| `description` | `string（长度 ..200）` | 否 | 说明 |
| `host` | `string[]` | 是 | 要求的主机型号 |
| `match` | [mappers.MapperMatchRule](mappers.md#mappermatchrule)[] | 是 | 匹配条件，任一项满足即匹配 |
| `source` | `"bundled" \| "user"` | 是 | 来源：bundled 预装，user 用户安装 |
| `enabled` | `boolean` | 是 | 是否启用 |

### MapperStorageInfo

清单里的一个存储区。

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `id` | `string` | 是 | 存储区标识 |
| `kind` | `"rom" \| "ram"` | 是 | 种类 |
| `from` | `"file.prg" \| "file.chr"` | 否 | rom 的数据来源 |
| `size` | `integer（0..）` | 否 | ram 的字节数 |
| `battery` | `boolean` | 是 | ram 是否带电池 |
| `fallbackSize` | `integer（0..）` | 否 | 来源为空时改用的 ram 的字节数 |

### MapperWindowInfo

清单里的一个窗口。

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `id` | `string` | 是 | 窗口标识 |
| `bus` | `"cpu" \| "ppu"` | 是 | 所在总线 |
| `range` | `string` | 是 | 地址范围，如 $8000-$9FFF |
| `storage` | `string` | 是 | 访问的存储区 |
| `size` | `integer（1..）` | 是 | 一个 bank 的字节数 |
| `initial` | `string` | 是 | 复位后指向的 bank：整数、first 或 last |
| `access` | `"r" \| "rw"` | 是 | 访问方式 |

### MapperRegisterInfo

清单里的一段寄存器范围。

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `bus` | `"cpu" \| "ppu"` | 是 | 所在总线 |
| `range` | `string` | 是 | 地址范围 |
| `read` | `boolean` | 是 | 是否回调读 |
| `write` | `boolean` | 是 | 是否回调写 |

### MapperStateInfo

清单里声明的一项脚本状态。

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `name` | `string` | 是 | 状态名 |
| `bits` | `integer（1..32）` | 是 | 位宽 |
| `description` | `string` | 是 | 说明 |

### MapperSettingOption

choice 设置项的一个选项。

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `value` | `string` | 是 | 取值 |
| `label` | `string` | 是 | 显示文字 |

### MapperSettingInfo

清单里的一个设置项。

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `key` | `string` | 是 | 设置键 |
| `label` | `string` | 是 | 显示文字 |
| `kind` | `"text" \| "choice" \| "switch"` | 是 | 种类 |
| `defaultValue` | `任意 JSON` | 是 | 默认值：text 与 choice 为字符串，switch 为布尔值 |
| `description` | `string` | 否 | 说明 |
| `options` | [mappers.MapperSettingOption](mappers.md#mappersettingoption)[] | 否 | choice 的选项 |

### MapperDetail

插件详情：摘要加清单的各部分。

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `summary` | [mappers.MapperSummary](mappers.md#mappersummary) | 是 |  |
| `storage` | [mappers.MapperStorageInfo](mappers.md#mapperstorageinfo)[] | 是 | 存储区 |
| `windows` | [mappers.MapperWindowInfo](mappers.md#mapperwindowinfo)[] | 是 | 窗口 |
| `nametable` | `string` | 是 | 名称表的初始连法 |
| `registers` | [mappers.MapperRegisterInfo](mappers.md#mapperregisterinfo)[] | 是 | 寄存器范围 |
| `state` | [mappers.MapperStateInfo](mappers.md#mapperstateinfo)[] | 是 | 脚本状态 |
| `settings` | [mappers.MapperSettingInfo](mappers.md#mappersettinginfo)[] | 是 | 设置项 |
