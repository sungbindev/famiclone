# output 模块

请求与事件的规则、连接与认证见 [README](README.md) 与 Ember 的《调试服务》文档。

## 请求

| 请求 | 简介 |
| --- | --- |
| [`output.setVolume`](#outputsetvolume) | 设置声音的音量与静音 |
| [`output.screenshot`](#outputscreenshot) | 截取最近一次完成的画面（256x240，暂停与断电后也有上一帧） |

### output.setVolume

设置声音的音量与静音；两个参数都可以省略，省略的保持不变。音量保存到首选项的 sound.volume，核心重启后保持；静音不保存，每次启动由 sound.startMuted 决定；变化平滑过渡，不会产生爆音。

**参数**

| 名称 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `volume` | `integer（0..100）` | 否 | 音量百分比，0 到 100 |
| `muted` | `boolean` | 否 | 是否静音 |

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `volume` | `integer（0..100）` | 是 | 当前的音量百分比 |
| `muted` | `boolean` | 是 | 当前是否静音 |

**错误码**：无专有错误码（参数不合格返回 `core.invalidParams`）

**示例**

```text
ember-debug call output.setVolume
```

```ts
await client.api.output.setVolume();
```

### output.screenshot

> 只经调试服务开放（界面不能发起）

截取最近一次完成的画面（256x240，暂停与断电后也有上一帧）。不带 file 时图像以二进制帧返回（结果里的 data，见 docs/04-调试服务.md 第 2.3 节）；带 file 时保存到调试输出目录下，结果里的 file 是保存后的完整路径，不再返回 data。

**参数**

| 名称 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `format` | `"png" \| "rgba"` | 否 | 图像格式，默认 png；rgba 是未压缩的像素，每像素 4 字节，按行从上到下 |
| `file` | `string（长度 1..100）` | 否 | 保存的文件名（不含目录，只能是字母、数字、点、下划线、短横线与空格）；png 没有扩展名时自动补 .png |

**结果**

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `format` | `"png" \| "rgba"` | 是 | 图像格式 |
| `width` | `integer（1..）` | 是 | 图像宽度（像素） |
| `height` | `integer（1..）` | 是 | 图像高度（像素） |
| `byteLength` | `integer（1..）` | 是 | 图像数据的字节数 |
| `file` | `string` | 否 | 保存后的完整路径；没有带 file 参数时没有这一项 |
| `data` | `二进制` | 否 | 图像数据（二进制，以二进制帧送达）；带 file 参数时没有这一项 |

**错误码**：`output.noPicture`、`output.invalidFileName`、`output.fileFailed`

**示例**

```text
ember-debug call output.screenshot
```

```ts
await client.api.output.screenshot();
```
