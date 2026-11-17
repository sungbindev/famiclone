# 清单：基本字段与匹配

清单是包里的 `manifest.json`，一个 JSON 对象。清单里不认识的字段一律拒绝安装。本页讲基本字段与匹配，其余内容分在[存储区、窗口与名称表](memory.md)和[寄存器与状态](registers.md)。

## 基本字段

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `id` | 字符串 | 是 | 插件标识，小写字母、数字与 `-`，3 到 40 个字符，不能以 `-` 开头；安装后不能改，改了就是另一个插件 |
| `name` | 字符串 | 是 | 显示名称，1 到 40 个字符 |
| `version` | 字符串 | 是 | 插件版本，`主.次.修订`，如 `1.0.0` |
| `apiVersion` | 整数 | 是 | 插件接口版本，目前只能写 `1` |
| `publisher` | 字符串 | 是 | 发布者名称，1 到 40 个字符 |
| `description` | 字符串 | 否 | 一两句话的说明，不超过 200 个字符，显示在插件列表里 |
| `host` | 字符串数组 | 是 | 要求的主机型号，目前只有 `["fc"]` |
| `devices` | 字符串数组 | 否 | 卡带用到的外设，目前只有 `mouse`，最多一项，不重复 |
| `busConflict` | 布尔 | 否 | 写寄存器时是否发生总线冲突，默认 `false` |
| `match` | 数组 | 是 | 匹配条件，1 到 32 项，见下 |

说明：

- `devices`：经典 FC 只有声明了 `mouse` 的卡带，点击画面才会捕获鼠标，没有声明的点不进去；鼠标事件用脚本的 `onMouse` 接收，见[脚本接口](script.md)。
- `busConflict`：为 `true` 时，CPU 写入的数据先与该地址当前读到的 ROM 字节按位相与，再交给寄存器；该地址没有映射 ROM 时不相与。每次复位回到清单的值，脚本可以用 `ctx.busConflict.set(true 或 false)` 按变体改。
- 其余部分的字段（`storage`、`windows`、`nametable`、`registers`、`state`、`settings`、`tables`、`tape`、`circuits`）见后面两页。

## 匹配

打开 `.nes` 文件时，Ember 先解析文件头，得到下面这些信息，再拿它们和每个插件的 `match` 比较：

| 字段 | 含义 |
| --- | --- |
| `mapper` | mapper 号 |
| `variant` | 变体：NES 2.0 文件头取 submapper；旧的 iNES 文件头取头部第 9 字节（从 0 数起）右移 1 位 |
| `prgSize`、`chrSize` | 程序 ROM 与图案 ROM 的字节数 |
| `mirroring`、`battery`、`trainer` | 头里的镜像方式、电池标志、训练器标志 |
| `nes20` | 是不是 NES 2.0 文件头 |
| `prgRamSize`、`prgNvramSize`、`chrRamSize`、`chrNvramSize` | 头里写明的 PRG RAM、带电池的 PRG RAM、CHR RAM、带电池的 CHR RAM 字节数，只有 NES 2.0 头写明，旧头里是 `null` |
| `crc32Prg`、`crc32All` | 程序 ROM 的校验值、程序加图案 ROM 的校验值 |

这些信息在脚本里就是只读的 `ctx.header`。

`match` 是一个数组，每一项是一组条件：

| 条件 | 写法 | 含义 |
| --- | --- | --- |
| `mapper` | 整数 | 必须写，mapper 号 |
| `variant` | 整数 | 变体 |
| `prgSize`、`chrSize` | 整数 | 程序 ROM、图案 ROM 的字节数 |
| `chr` | `"present"` 或 `"none"` | 文件里有 CHR ROM，或没有 |
| `crc32Prg`、`crc32All` | 8 位十六进制字符串，如 `"A1B2C3D4"` | 校验值，大小写都行 |

- 一项里写出的条件必须全部满足；数组里任意一项满足，插件就匹配。
- 同一个 mapper 号对应多种卡带时，用变体、大小或校验值区分，可以写成一个插件的多项匹配，也可以每种卡带一个插件。
- 同一块硬件用在多个 mapper 号上时，写多项 `match`，脚本里按 `ctx.header.mapper` 分支。

示例：

```json
"match": [
  { "mapper": 4 },
  { "mapper": 118, "chr": "present" },
  { "mapper": 172, "prgSize": 32768 }
]
```

## 匹配的优先级

- 多个插件同时匹配同一个文件时，写出的条件更多的优先。一个插件有多项匹配时，取它满足的那些项里条件最多的一项来比较。
- 条件数相同时提示冲突，说明里列出同样匹配的插件，把不需要的那个停用即可。
- 没有任何已启用的插件匹配时，提示缺少哪个 mapper 号和变体的插件；匹配的插件都被停用时，改为提示去启用。

想为已有插件的 mapper 做自己的版本，有两条路：停用官方插件，或者在 `match` 里多写一个条件（如 `prgSize`）让自己的插件优先。

mapper 171（学习机 BIOS 文件）没有匹配的插件时同样提示找不到，Ember 不会把它识别成学习机，也不允许改 mapper 号强行运行。学习机的 BIOS 要走“机型与 BIOS”，见[机型与 BIOS](../manual/machines.md)。
