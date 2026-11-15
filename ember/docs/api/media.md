# media 模块

请求与事件的规则、连接与认证见 [README](README.md) 与 Ember 的《调试服务》文档。

## 请求

| 请求 | 简介 |
| --- | --- |
| [`media.floppyInsert`](#mediafloppyinsert) | 读取盘片镜像文件（上限 16 MB）放进驱动器，替换已有的盘片 |
| [`media.floppyEject`](#mediafloppyeject) | 取出盘片并丢弃镜像（没有保存的修改随之丢失） |
| [`media.floppySave`](#mediafloppysave) | 把驱动器里的镜像写回放入时的文件，并清除“已修改”标志 |
| [`media.floppyWriteProtect`](#mediafloppywriteprotect) | 设置或取消盘片的写保护 |
| [`media.floppyStatus`](#mediafloppystatus) | 全部软驱的状态 |
| [`media.floppyFault`](#mediafloppyfault) | 故障注入：取盘、写保护、无索引脉冲、磁头卡住、控制器无响应、指定扇区校验错、全部清除 |
| [`media.printerStatus`](#mediaprinterstatus) | 打印机的状态：空闲、打印中、打印完成，当前页是否有内容，已输出的页数与最近一页的文件名 |
| [`media.printerFlush`](#mediaprinterflush) | 立即输出当前页，等同打印机收到换页 |

### media.floppyInsert

> 界面里只能由主进程发起，调试服务可以调用

读取盘片镜像文件（上限 16 MB）放进驱动器，替换已有的盘片；盘片与电源无关，断电时也可以插入。镜像的格式由大小推断（160、180、320、360、720、1200、1440、2880 KB，其他大小按 1.44MB 处理）。

**参数**

| 名称 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `drive` | `integer（0..3）` | 是 | 驱动器编号 |
| `path` | `string（长度 1..）` | 是 | 镜像文件的路径 |
| `writeProtected` | `boolean` | 否 | 是否写保护，默认 false |

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `status` | [media.FloppyStatus](media.md#floppystatus) | 是 |  |

**错误码**：`media.noMachine`、`media.noDrive`、`media.fileNotFound`、`media.fileUnreadable`、`media.imageInvalid`

**示例**

```text
ember-debug call media.floppyInsert drive=<integer> path=<string>
```

```ts
await client.api.media.floppyInsert({ drive: …, path: … });
```

### media.floppyEject

取出盘片并丢弃镜像（没有保存的修改随之丢失）；没有盘片时没有效果。

**参数**

| 名称 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `drive` | `integer（0..3）` | 是 | 驱动器编号 |

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `status` | [media.FloppyStatus](media.md#floppystatus) | 是 |  |

**错误码**：`media.noMachine`、`media.noDrive`

**示例**

```text
ember-debug call media.floppyEject drive=<integer>
```

```ts
await client.api.media.floppyEject({ drive: … });
```

### media.floppySave

把驱动器里的镜像写回放入时的文件，并清除“已修改”标志；外部修改与学习机写盘都只改内存里的镜像，不自动落盘。

**参数**

| 名称 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `drive` | `integer（0..3）` | 是 | 驱动器编号 |

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `status` | [media.FloppyStatus](media.md#floppystatus) | 是 |  |

**错误码**：`media.noMachine`、`media.noDrive`、`media.noDisk`、`media.saveFailed`

**示例**

```text
ember-debug call media.floppySave drive=<integer>
```

```ts
await client.api.media.floppySave({ drive: … });
```

### media.floppyWriteProtect

设置或取消盘片的写保护；写保护的盘片拒绝学习机写入。

**参数**

| 名称 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `drive` | `integer（0..3）` | 是 | 驱动器编号 |
| `writeProtected` | `boolean` | 是 | 是否写保护 |

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `status` | [media.FloppyStatus](media.md#floppystatus) | 是 |  |

**错误码**：`media.noMachine`、`media.noDrive`

**示例**

```text
ember-debug call media.floppyWriteProtect drive=<integer> writeProtected=<boolean>
```

```ts
await client.api.media.floppyWriteProtect({ drive: …, writeProtected: … });
```

### media.floppyStatus

全部软驱的状态；还没有主机或主机没有软驱时 drives 为空。

**参数**

（无）

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `drives` | [media.FloppyStatus](media.md#floppystatus)[] | 是 | 各驱动器的状态，按编号排列 |

**错误码**：无专有错误码（参数不合格返回 `core.invalidParams`）

**示例**

```text
ember-debug call media.floppyStatus
```

```ts
await client.api.media.floppyStatus();
```

### media.floppyFault

> 只经调试服务开放（界面不能发起）

故障注入：取盘、写保护、无索引脉冲、磁头卡住、控制器无响应、指定扇区校验错、全部清除；用来检查学习机程序对软驱故障的处理。

**参数**

| 名称 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `drive` | `integer（0..3）` | 是 | 驱动器编号 |
| `fault` | `"ejected" \| "writeProtected" \| "noIndex" \| "seekFail" \| "unresponsive" \| "badSector" \| "clear"` | 是 | 故障：ejected 取盘（放回时内容不变），writeProtected 写保护，noIndex 无索引脉冲，seekFail 磁头卡住，unresponsive 控制器无响应，badSector 指定扇区校验错，clear 关闭全部故障、放回盘片、取消写保护 |
| `enabled` | `boolean` | 否 | 打开还是关闭这个故障，默认 true；clear 不用这一项 |
| `sector` | `integer（0..）` | 否 | badSector 的逻辑扇区号（磁道×每道扇区数×面数 + 磁头×每道扇区数 + 扇区号 - 1），badSector 必填 |

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `status` | [media.FloppyStatus](media.md#floppystatus) | 是 |  |

**错误码**：`media.noMachine`、`media.noDrive`、`media.sectorRequired`

**示例**

```text
ember-debug call media.floppyFault drive=<integer> fault=<"ejected" | "writeProtected" | "noIndex" | "seekFail" | "unresponsive" | "badSector" | "clear">
```

```ts
await client.api.media.floppyFault({ drive: …, fault: … });
```

### media.printerStatus

打印机的状态：空闲、打印中、打印完成，当前页是否有内容，已输出的页数与最近一页的文件名；还没有主机或主机没有打印机时为空闲。

**参数**

（无）

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `status` | [media.PrinterStatus](media.md#printerstatus) | 是 |  |

**错误码**：无专有错误码（参数不合格返回 `core.invalidParams`）

**示例**

```text
ember-debug call media.printerStatus
```

```ts
await client.api.media.printerStatus();
```

### media.printerFlush

立即输出当前页，等同打印机收到换页；页上的内容画成 PNG 保存到首选项“打印机”里设置的目录，成功后发 media.printed。

**参数**

（无）

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `status` | [media.PrinterStatus](media.md#printerstatus) | 是 |  |

**错误码**：`media.noMachine`、`media.noPrinter`、`media.printerEmpty`

**示例**

```text
ember-debug call media.printerFlush
```

```ts
await client.api.media.printerFlush();
```

## 事件

事件要先用 `debug.subscribe` 订阅才会收到，以 JSON-RPC 通知送达。

### media.changed

盘片的状态变化：插入、取出、保存或注入故障。

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `device` | `"floppy"` | 是 | 设备 |
| `drive` | `integer（0..）` | 是 | 驱动器编号 |

### media.printed

打印机输出了一页：已经保存成 PNG 文件。

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `fileName` | `string` | 是 | 文件名，不含路径 |
| `page` | `integer（1..）` | 是 | 本次运行以来的页序号 |

### media.printFailed

打印机输出的一页无法保存（目录无法写入等），这一页已丢弃。

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `message` | `string` | 是 | 原因，面向用户 |

## 数据类型

### FloppyFaults

软驱上注入的故障；全部关闭是正常状态。

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `ejected` | `boolean` | 是 | 取盘：盘片被暂时取出（镜像还在，放回后内容不变） |
| `noIndex` | `boolean` | 是 | 没有索引脉冲：读扇区编号、读写与格式化命令一直不结束 |
| `seekFail` | `boolean` | 是 | 磁头卡住：寻道与回零不移动 |
| `unresponsive` | `boolean` | 是 | 控制器无响应：收到命令字节后不再处理 |
| `badSectors` | `integer（0..）[]` | 是 | 读时数据校验出错的逻辑扇区号 |

### FloppyStatus

一个软驱的状态。

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `drive` | `integer（0..）` | 是 | 驱动器编号，从 0 开始 |
| `inserted` | `boolean` | 是 | 里面有没有盘片 |
| `fileName` | `string` | 否 | 盘片来源的文件名，不含路径；没有盘片时没有这一项 |
| `writeProtected` | `boolean` | 是 | 盘片是否写保护 |
| `modified` | `boolean` | 是 | 镜像自放入或上次保存以来是否被修改过（学习机写盘会改它） |
| `size` | `integer（0..）` | 是 | 镜像的大小（字节）；没有盘片时为 0 |
| `cylinder` | `integer（0..）` | 是 | 磁头所在的磁道 |
| `motor` | `boolean` | 是 | 马达是否在转 |
| `faults` | [media.FloppyFaults](media.md#floppyfaults) | 是 |  |

### PrinterStatus

并口打印机的状态。

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `state` | `"idle" \| "printing" \| "done"` | 是 | 状态：idle 空闲（机器没有运行，或还没有内容），printing 打印中（当前页上有内容），done 打印完成（输出过一页且下一页还没有内容） |
| `hasContent` | `boolean` | 是 | 当前页上是否已经有内容 |
| `pages` | `integer（0..）` | 是 | 本次运行以来已输出的页数 |
| `lastFile` | `string` | 否 | 最近一页的文件名，不含路径；还没有输出过时没有这一项 |
