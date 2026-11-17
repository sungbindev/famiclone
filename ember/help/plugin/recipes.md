# 常见写法

这一页收集写插件时常用的现成写法。标注“已验证”的示例都在 Ember 里真实加载并运行过，可以直接改了用。

## 一个寄存器同时切程序与图案窗口

用 `bank` 数组，每项取写入值的一段：

```json
"registers": [
  {
    "bus": "cpu",
    "range": "$8000-$FFFF",
    "write": {
      "state": "latch",
      "bank": [
        { "window": "prg-low", "shift": 0, "mask": 7 },
        { "window": "chr-0000", "shift": 4, "mask": 15 }
      ]
    }
  }
],
"state": [{ "name": "latch", "bits": 8, "description": "最近一次写入的值" }]
```

写入 `0x32`：低 3 位是 2，程序窗口切到第 2 个 bank；`(0x32 >> 4) & 15` 是 3，图案窗口切到第 3 个 bank。（已验证）

## 总线冲突

有些卡带的寄存器直接接在 ROM 的输出上，CPU 写入的数据会和该地址的 ROM 字节相与。清单写 `"busConflict": true` 就是默认有冲突；要按变体决定，在脚本的 `reset` 里设置：

```js
export default {
  reset(ctx) {
    ctx.busConflict.set(ctx.header.variant === 2);
  }
};
```

官方插件 `cnrom` 就是这样：寄存器是声明式的，脚本只在复位时决定有没有总线冲突。

## 带电池或不带电池的 PRG RAM

挂在 `$6000-$7FFF` 的 8K RAM，大小与是否带电池交给文件头决定：

```json
"storage": [
  { "id": "prg-ram", "kind": "ram", "size": { "header": "prgAny", "default": 8192 } }
],
"windows": [
  { "id": "prg-ram", "bus": "cpu", "range": "$6000-$7FFF", "storage": "prg-ram", "size": 8192, "initial": 0, "access": "rw" }
]
```

## 用寄存器改名称表连法

```js
write(ctx, bus, addr, value) {
  if ((addr & 0xE000) === 0xE000) ctx.nametable.set(value & 1 ? 'horizontal' : 'vertical');
}
```

清单里的 `nametable` 要先给一个初始连法（如 `vertical`）。（已验证）

## 读寄存器

只想读出一个状态，用声明：

```json
{ "bus": "cpu", "range": "$5000-$5FFF", "read": { "state": "status" } }
```

要按地址算出读到的值，用脚本，清单写 `"read": true`：

```js
read(ctx, bus, addr) {
  if (addr === 0x5000) return ctx.state.status;
  return undefined;
}
```

返回 `undefined` 表示这次不驱动总线，读到的是总线上最近的值。（已验证）

## 按 CPU 周期计时的中断

很多 mapper 有一个按 CPU 周期递减或递增的计数器，数到头就向 CPU 发中断。不要在脚本里逐周期计数：写寄存器时算出还要多少个周期，登记一个计时，到点在 `onTimer` 里拉低中断。

```js
export default {
  write(ctx, bus, addr, value) {
    switch (addr & 0xE000) {
      case 0xA000:
        ctx.state.status = value;
        ctx.timer.after('irq', (value + 1) * 1000); // 过 (value + 1) × 1000 个 CPU 周期后触发
        break;
      case 0xC000:
        ctx.timer.cancel('irq'); // 关闭计时
        ctx.irq.release(); // 应答：释放中断线
        break;
    }
  },
  onTimer(ctx, name) {
    ctx.irq.assert(); // 拉低中断线
  }
};
```

要点：

- 中断线被拉低后一直保持，直到脚本调用 `ctx.irq.release()`，所以要在应答的寄存器里释放。
- 想知道计数器“现在数到了哪”，用 `ctx.cycle()` 减去开始计时时记下的周期数，官方插件 `irem-h3001` 就是这样读回计数值的。
- 复位会取消全部计时。

示例中的 `ctx.state.status` 要在清单里声明成状态。（已验证）

## 按 PPU 地址线 A12 计数的中断

扫描线中断类的 mapper 靠 PPU 取图块时地址线 A12 的上升沿计数。简单的计数器用 `onA12`，每次上升沿调用一次：

```js
export default {
  write(ctx, bus, addr, value) {
    if (addr < 0xC000) {
      ctx.state.counter = value; // 设定计数值
    } else {
      ctx.irq.release(); // 应答
    }
  },
  onA12(ctx) {
    if (ctx.state.counter === 0) return;
    ctx.state.counter -= 1;
    if (ctx.state.counter === 0) ctx.irq.assert();
  }
};
```

Ember 已经把 A12 先低过 4 次显存访问再变高的才算一次上升沿，脚本不用再滤波。脚本导出了 `onA12`，卡带才会收到显存地址。（已验证）

MMC3 家族的完整计数规则（重载、使能、版本差异）不用自己写，用电路，见下一节。

## MMC3 与它的变体

MMC3 的寄存器、镜像、保护位和扫描线中断都由主机里的 `mmc3` 电路实现，清单声明一下就行：

```json
"registers": [
  { "bus": "cpu", "range": "$8000-$FFFF", "write": { "circuit": "mmc3" } }
],
"circuits": [
  { "id": "mmc3", "type": "mmc3", "wire": {
    "prg": ["prg-8000", "prg-a000", "prg-c000", "prg-e000"],
    "chr": ["chr-0000", "chr-0400", "chr-0800", "chr-0c00", "chr-1000", "chr-1400", "chr-1800", "chr-1c00"],
    "mirroring": true,
    "ram": "prg-ram"
  } }
]
```

很多 MMC3 变体多了一个外部寄存器，用来扩大 bank 范围。这时不写 `wire`，由脚本把写入交给电路，再按外部寄存器重算全部窗口：

```js
write(ctx, bus, addr, value) {
  if (addr < 0x8000) {
    // 外部寄存器：记下新的值，然后重算窗口
    block = value & 0x07;
    update(ctx);
    return;
  }
  ctx.circuit('mmc3').write(addr, value); // 交给电路
  update(ctx); // 再按外部寄存器重算窗口
}
```

`update` 里用 `ctx.circuit('mmc3').get('r6')` 之类读出电路的页寄存器，加工后调用各窗口的 `setBank`。完整写法见官方插件 `mmc3-37`；其他 `mmc3-*` 插件是同一类做法。

## 查表实现的逻辑方程

寄存器的输出是几个输入的逻辑函数时（如卡上的 PAL 芯片），用查表：清单声明输入的来源，脚本算每个输入组合的结果，Ember 缓存，游戏每次访问都是原生速度。完整的清单与脚本见[寄存器与状态](registers.md)里的“查表”一节，以及官方插件 `plq`。

## 键盘热键

见[脚本接口](script.md)里的“键盘与鼠标”。

## 参考官方插件

预装的官方插件覆盖了常见的各种写法，按需要找一个接近的改：

| 想做的 | 参考 |
| --- | --- |
| 最简单的整体切换 | `unrom`、`cnrom`（声明式寄存器） |
| 串行寄存器、多个窗口联动重算 | `mmc1` |
| 周期计数的中断 | `irem-h3001`、`front-fareast` |
| A12 计数的中断 | `mapper-117`、`mapper-222` |
| MMC3 电路与外部寄存器 | `mmc3`、`mmc3-37` |
| 图案页锁存 | `mmc2`、`mmc4` |
| 查表 | `plq` |
| 磁带 | `qls`、`plq` |
