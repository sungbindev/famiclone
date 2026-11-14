# input 模块

请求与事件的规则、连接与认证见 [README](README.md) 与 Ember 的《调试服务》文档。

## 请求

| 请求 | 简介 |
| --- | --- |
| [`input.key`](#inputkey) | 按下或松开一个物理键，交给卡带的 mapper 脚本 |
| [`input.mouse`](#inputmouse) | 鼠标事件，交给卡带的 mapper 脚本 |
| [`input.gamepad`](#inputgamepad) | 游戏手柄一个手柄的当前全部状态，交给卡带的 mapper 脚本 |
| [`input.pad`](#inputpad) | 直接设置主机某个手柄口上标准手柄的按键，不经 mapper 脚本 |
| [`input.releaseAll`](#inputreleaseall) | 松开经 key 按下的全部键、把经 pad 设置过的手柄口清零、并让经 gamepad 注入的手柄断开 |

### input.key

> 只经调试服务开放（界面不能发起）

按下或松开一个物理键，交给卡带的 mapper 脚本；没有开机时被丢弃。

**参数**

| 名称 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `code` | `string（长度 1..32）` | 是 | 物理键位，取 KeyboardEvent.code，如 KeyW、Space、Numpad1、Enter |
| `down` | `boolean` | 是 | true 按下，false 松开 |

**结果**

（无）

**错误码**：无专有错误码（参数不合格返回 `core.invalidParams`）

**示例**

```text
ember-debug call input.key code=<string> down=<boolean>
```

```ts
await client.api.input.key({ code: …, down: … });
```

### input.mouse

> 只经调试服务开放（界面不能发起）

鼠标事件，交给卡带的 mapper 脚本；坐标单位是标准帧（256x240）的像素，省略的数值按 0 处理。

**参数**

| 名称 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `kind` | `"move" \| "down" \| "up" \| "wheel" \| "leave"` | 是 | 事件种类 |
| `x` | `number` | 否 | 指针在标准帧上的横坐标，可以超出画面范围 |
| `y` | `number` | 否 | 指针在标准帧上的纵坐标，可以超出画面范围 |
| `dx` | `number` | 否 | 相对上一次事件的横向位移 |
| `dy` | `number` | 否 | 相对上一次事件的纵向位移 |
| `buttons` | `integer（0..7）` | 否 | 当前按住的按键位：1 左键、2 右键、4 中键 |
| `wheel` | `number` | 否 | 滚轮的滚动量，向下为正，只有 wheel 事件用 |

**结果**

（无）

**错误码**：无专有错误码（参数不合格返回 `core.invalidParams`）

**示例**

```text
ember-debug call input.mouse kind=<"move" | "down" | "up" | "wheel" | "leave">
```

```ts
await client.api.input.mouse({ kind: … });
```

### input.gamepad

> 只经调试服务开放（界面不能发起）

游戏手柄一个手柄的当前全部状态，交给卡带的 mapper 脚本；拔掉时 connected 为 false，按键与轴省略。

**参数**

| 名称 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `index` | `integer（0..7）` | 是 | 手柄编号，同浏览器 Gamepad API 的 index |
| `connected` | `boolean` | 是 | 是否连着 |
| `buttons` | `boolean[]` | 否 | 各按键是否按下，顺序同浏览器的标准映射 |
| `axes` | `number[]` | 否 | 各轴的值，范围 -1 到 1，顺序同浏览器的标准映射 |

**结果**

（无）

**错误码**：无专有错误码（参数不合格返回 `core.invalidParams`）

**示例**

```text
ember-debug call input.gamepad index=<integer> connected=<boolean>
```

```ts
await client.api.input.gamepad({ index: …, connected: … });
```

### input.pad

> 只经调试服务开放（界面不能发起）

直接设置主机某个手柄口上标准手柄的按键，不经 mapper 脚本；位：1 A、2 B、4 选择、8 开始、16 上、32 下、64 左、128 右。

**参数**

| 名称 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `port` | `integer（0..7）` | 是 | 手柄口编号，从 0 开始 |
| `buttons` | `integer（0..255）` | 是 | 按键掩码，1 表示按下 |

**结果**

（无）

**错误码**：`input.invalidPort`

**示例**

```text
ember-debug call input.pad port=<integer> buttons=<integer>
```

```ts
await client.api.input.pad({ port: …, buttons: … });
```

### input.releaseAll

> 只经调试服务开放（界面不能发起）

松开经 key 按下的全部键、把经 pad 设置过的手柄口清零、并让经 gamepad 注入的手柄断开。

**参数**

（无）

**结果**

（无）

**错误码**：无专有错误码（参数不合格返回 `core.invalidParams`）

**示例**

```text
ember-debug call input.releaseAll
```

```ts
await client.api.input.releaseAll();
```
