# 清单：存储区、窗口与名称表

这一页描述卡带上的存储芯片，以及 CPU 与 PPU 的地址怎么落到存储芯片上。只能描述卡带上的东西：主板与芯片内部的存储不能在这里声明或修改。

## 存储区

`storage` 声明卡带上的存储芯片，最多 16 项。

| 字段 | 说明 |
| --- | --- |
| `id` | 存储区标识，同一插件内不重复，小写字母、数字与 `-`，不超过 32 个字符 |
| `kind` | `rom` 或 `ram` |
| `from` | `rom` 的数据来源：`file.prg`（文件里的程序 ROM）或 `file.chr`（文件里的图案 ROM） |
| `size` | `ram` 的字节数：整数（1 字节到 16 MiB），或 `{ "header": 头里的哪一项, "default": 默认字节数 }`，见下 |
| `battery` | `ram` 是否带电池 |
| `fallback` | 只有 `rom` 能写：来源为空时改用的存储区，写成 `{ "kind": "ram", "size": 字节数或 {header, default}, "battery": 可选 }`，如文件没有 CHR 时用 8K 的 CHR RAM |

示例：

```json
"storage": [
  { "id": "prg", "kind": "rom", "from": "file.prg" },
  { "id": "chr", "kind": "rom", "from": "file.chr", "fallback": { "kind": "ram", "size": 8192 } },
  { "id": "prg-ram", "kind": "ram", "size": { "header": "prgAny", "default": 8192 } }
]
```

### 容量取自文件头

`size` 写成 `{ "header": ..., "default": ... }` 时，容量由文件头决定。`header` 可以是：

| 取值 | 含义 |
| --- | --- |
| `prgRam` | 文件头里的 PRG RAM 大小 |
| `prgNvram` | 文件头里带电池的 PRG RAM 大小 |
| `prgAny` | 卡带默认挂在 `$6000` 的那块 RAM：有带电池的就用它（并带电池），否则用 PRG RAM（不带电池）；是否带电池由文件头决定，这种存储区不要写 `battery` |
| `chrRam` | 文件头里的 CHR RAM 大小 |
| `chrNvram` | 文件头里带电池的 CHR RAM 大小 |

NES 2.0 文件头写明了就用头里的，写明为 0 的存储区容量为 0。旧的 iNES 文件头没有写明，这时：

| `header` | 旧文件头下的结果 |
| --- | --- |
| `prgRam` | 文件带电池时为 0，否则取 `default` |
| `prgNvram` | 文件带电池时取 `default`，否则为 0 |
| `chrRam` | 取 `default` |
| `chrNvram` | 0 |

`default` 是 0 到 16 MiB 的整数。

### 存储区的行为

- ROM 拒绝写入：经窗口写入被丢弃，脚本经 `ctx.storage(id).write` 写入返回 `false`。
- 上电复位时不带电池的 RAM 清零，带电池的保持；软复位时所有 RAM 保持。
- 容量为 0 的存储区，窗口读到开放总线，写入被丢弃。
- 来源为空又没有 `fallback` 的 ROM，容量为 0。
- 带电池 RAM 的内容目前只在这次运行里保持，不会存成文件。
- 文件里的训练器内容目前不会载入存储区，脚本只能从 `ctx.header.trainer` 知道有没有。

## 窗口

`windows` 声明可以切换的地址窗口，最多 64 项。窗口把一段地址范围指向存储区里的一块（一个 bank）。寄存器写入时切换窗口，之后的读写由 Ember 直接按窗口访问，不再调用脚本，这和真机一样：译码输出只在寄存器写入时变化。

| 字段 | 说明 |
| --- | --- |
| `id` | 窗口标识 |
| `bus` | `cpu` 或 `ppu` |
| `range` | 窗口覆盖的地址范围，如 `$8000-$9FFF` |
| `storage` | 窗口访问的存储区标识；也可以写成存储区标识的数组（最多 4 个），第一个是复位后绑定的，脚本用 `select` 切换 |
| `size` | 一个 bank 的字节数，必须等于窗口的大小 |
| `initial` | 复位后指向的 bank：整数、`first`、`last`；负整数从末尾数，`-1` 是最后一个，`-2` 是倒数第二个；不写等于 `first` |
| `access` | `r` 或 `rw`；指向 ROM 的窗口只能是 `r` |

规则：

- CPU 侧的窗口只能在卡带的地址 `$4020-$FFFF`；PPU 侧的窗口只能在图案表地址 `$0000-$1FFF`。名称表地址的连法见下面的“名称表”。
- 同一条总线上的窗口地址范围不能重叠；没有被窗口或寄存器覆盖的卡槽地址读到开放总线。
- bank 号按存储区的 bank 数取模（bank 数是存储区容量除以 `size` 向下取整，至少为 1），所以超出范围的 bank 号自动回绕，bank 数不是 2 的幂也成立。按字节偏移设置时（`setOffset`）同样按存储区容量取模。
- 窗口访问越过存储区末尾时从头回绕（镜像）：16K 的程序 ROM 放进两个 16K 的窗口，就是同一块 ROM 映射两次。
- `access` 为 `rw` 的窗口必须指向 `ram`，或指向带 `fallback` 的 `rom`（文件没有这部分数据时它是 RAM，有数据时是 ROM，写入被 ROM 拒绝）。`storage` 是数组时，其中有一项满足即可。
- `access` 只是上限：脚本可以用 `setAccess('r')` 临时写保护，上电复位回到清单的值。

示例（4 个 8K 程序窗口加 8K 带电池 RAM）：

```json
"windows": [
  { "id": "prg-8000", "bus": "cpu", "range": "$8000-$9FFF", "storage": "prg", "size": 8192, "initial": 0, "access": "r" },
  { "id": "prg-a000", "bus": "cpu", "range": "$A000-$BFFF", "storage": "prg", "size": 8192, "initial": 1, "access": "r" },
  { "id": "prg-c000", "bus": "cpu", "range": "$C000-$DFFF", "storage": "prg", "size": 8192, "initial": -2, "access": "r" },
  { "id": "prg-e000", "bus": "cpu", "range": "$E000-$FFFF", "storage": "prg", "size": 8192, "initial": -1, "access": "r" },
  { "id": "prg-ram", "bus": "cpu", "range": "$6000-$7FFF", "storage": "prg-ram", "size": 8192, "initial": 0, "access": "rw" }
]
```

## 名称表

`nametable` 声明名称表的初始连法，值是下面之一：

| 取值 | 含义 |
| --- | --- |
| `header` | 按文件头里的镜像方式 |
| `horizontal`、`vertical` | 水平、垂直镜像 |
| `single0`、`single1` | 单屏，用第 0 页或第 1 页 |
| `four` | 四屏：卡带自带 4K 的名称表显存，`$3000-$3EFF` 是 `$2000-$2EFF` 的镜像 |
| `custom` | 逐个名称表映射，由脚本用 `ctx.nametable.map` 指定 |

其中除 `four` 之外都是用主板上的 2K 显存，只是连法不同。

脚本可以在运行中用 `ctx.nametable.set('horizontal')` 之类改连法，上电复位回到清单里的初始连法。

### 逐个映射

`custom` 把名称表地址 `$2000-$2FFF` 分成 4 个 1K 的槽（`$3000-$3EFF` 是它们的镜像），脚本用 `ctx.nametable.map(槽, 来源, bank)` 给每个槽指定来源：

- `'ciram0'`、`'ciram1'`：主板显存的两页，`bank` 被忽略。
- 清单里的存储区标识：卡带自己响应，`bank` 是 1K 页号，按存储区容量取模；ROM 来源只读，RAM 来源可写。

脚本没有给过的槽按 `ciram0`；`ctx.nametable.set` 会让名称表回到一种固定连法，之后再 `map` 又回到逐槽映射；清单不是 `custom` 时调用 `map` 会抛出 `RangeError`。上电复位清除全部映射。
