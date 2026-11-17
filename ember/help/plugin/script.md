# 脚本接口

脚本是包里的 `main.js`，用来补充清单写不出的逻辑。这一页列出脚本能导出什么、Ember 给脚本什么对象、有哪些限制。

## 基本写法

脚本是单个 ES 模块，默认导出一个对象，对象上的函数就是回调：

```js
export default {
  reset(ctx, kind) {},
  read(ctx, bus, addr) {},
  write(ctx, bus, addr, value) {},
  onTimer(ctx, name) {},
  onCircuit(ctx, id) {},
  onA12(ctx) {},
  onKey(ctx, event) {},
  onMouse(ctx, event) {},
  onTapeEdge(ctx, level, samples) {},
  tables: { 表标识(ctx, ...各来源的值) {} }
};
```

- 每个导出都是可选的，只写用得到的。
- 脚本里不能 `import`，也不能引用别的文件；需要的辅助函数直接写在同一个文件里。
- 函数都是同步的，没有 `Promise` 与定时器，顶层不能 `await`。
- 第一个参数 `ctx` 是 Ember 给的宿主对象，脚本与硬件之间的一切都经它进行。

## 回调

| 回调 | 什么时候调用 |
| --- | --- |
| `reset(ctx, kind)` | 复位时。`kind` 是 `'power'`（上电）或 `'soft'`（软复位）。上电复位时，Ember 先把窗口回到 `initial`、状态清零、名称表回到初始连法、释放中断，再调用 `reset`；软复位只调用 `reset`，窗口与状态保持不变，要不要改由脚本自己决定 |
| `read(ctx, bus, addr)` | CPU 或 PPU 读一个声明为 `read: true` 的寄存器。返回 0 到 255 的整数；返回 `undefined` 表示不驱动总线 |
| `write(ctx, bus, addr, value)` | 写一个声明为 `write: true` 的寄存器 |
| `onTimer(ctx, name)` | `ctx.timer.after(name, 周期数)` 登记的计时到期 |
| `onCircuit(ctx, id)` | 声明式寄存器把写入交给电路之后，可以在这里按电路的新状态重算窗口 |
| `onA12(ctx)` | PPU 地址线 A12 出现一次上升沿；A12 先连续低过 4 次显存访问再变高才算一次（滤掉取图块时的短暂低电平）。脚本导出了它，卡带才会收到显存地址 |
| `onKey(ctx, event)` | 键盘事件，见下面的“键盘与鼠标” |
| `onMouse(ctx, event)` | 鼠标事件，见下面的“键盘与鼠标” |
| `onTapeEdge(ctx, level, samples)` | 磁带数据线翻转，见下面的“磁带机” |
| `tables.<表标识>` | 查表的一个表项，见[寄存器与状态](registers.md) |

- 清单里声明了 `read: true` 的寄存器，就必须实现 `read`；声明了 `write: true`，就必须实现 `write`。
- `bus` 是 `'cpu'` 或 `'ppu'`，`addr` 是访问的地址。
- 读回调只能返回整个字节：没有“只驱动其中几位，其余位保持总线上最近的值”的办法；芯片没有驱动的位按 0 返回。

## 宿主对象 ctx

| 成员 | 说明 |
| --- | --- |
| `ctx.header` | 只读的卡带描述（mapper 号、变体、大小、镜像、校验值等），字段见[清单：基本字段与匹配](manifest.md) |
| `ctx.window(id)` | 窗口，见下 |
| `ctx.state` | 已声明状态的读写，如 `ctx.state.prgBank = 5` |
| `ctx.storage(id)` | 存储区：`size`、`read(offset)`、`write(offset, value)`；ROM 拒绝写入，返回 `false` |
| `ctx.peek(address)` | 按卡带自己的 CPU 侧窗口读一个字节，不经过寄存器，没有副作用；窗口没有映射或不驱动数据线时返回 `undefined`；地址不在 0 到 65535 之间抛出 `RangeError` |
| `ctx.nametable` | `set(mode)`：`horizontal`、`vertical`、`single0`、`single1`、`four`；`map(slot, source, bank)`：清单为 `custom` 时逐个槽映射 |
| `ctx.irq` | 卡带的中断引脚：`assert()` 拉低，`release()` 释放 |
| `ctx.busConflict.set(enabled)` | 设置写寄存器时是否发生总线冲突 |
| `ctx.cycle()` | 自上电起的 CPU 周期数 |
| `ctx.timer` | `after(name, cpuCycles)` 登记计时，`cancel(name)` 取消，见下 |
| `ctx.circuit(id)` | 清单里的电路：`write(addr, value)`、`get(name)`、`bank('program' 或 'character', addr)` |
| `ctx.settings.get(key)` | 读设置项，目前只有清单里的默认值 |
| `ctx.random(max)` | 返回 0 到 `max - 1` 的整数，`max` 为 1 到 2147483648 |
| `ctx.log.warn(message)` | 记一条警告，目前不会显示在任何地方 |
| `ctx.keys.isDown(code)` | 物理键现在是否按着，见“键盘与鼠标” |
| `ctx.pad(port)` | 手柄口：`set(button, down)`、`release()`，见“键盘与鼠标” |
| `ctx.device('tape')` | 磁带机，见“磁带机” |

### 窗口 ctx.window(id)

| 成员 | 说明 |
| --- | --- |
| `setBank(n)` | 切换 bank，`n` 按窗口的规则取模 |
| `setOffset(bytes)` | 按字节偏移映射，按存储区容量取模 |
| `unmap()` | 取消映射：读到开放总线，写被丢弃；再调用 `setBank` 或 `setOffset` 重新映射 |
| `bankCount` | 只读：当前绑定的存储区里有几个窗口大小的 bank，至少 1，用来写“倒数第 n 个 bank” |
| `select(存储区标识, bank)` | 切到清单里该窗口列出的另一个存储区并选 bank，`bank` 默认 0；不在列表里抛出 `RangeError` |
| `setAccess('r' 或 'rw')` | 写保护或允许写入，上电复位回到清单的 `access` |

### 计时器 ctx.timer

- `after(name, cpuCycles)`：登记一个计时，`cpuCycles` 个 CPU 周期之后调用 `onTimer(ctx, name)`；周期数为 1 到 2 的 40 次方。同名的计时还在等待时先取消再重新登记。
- `cancel(name)`：取消。
- 时间单位是 CPU 周期，由 Ember 的调度器预测到期时刻，脚本不逐周期运行；复位取消全部计时；同时最多 32 个不同的名字。
- 要实现 CPU 周期计数的中断：写寄存器时用 `ctx.cycle()` 算出到期周期，登记计时，到点在 `onTimer` 里拉低中断，见[常见写法](recipes.md)。

## 键盘与鼠标

手柄按键不由脚本产生：游戏手柄的事件与键盘按键，由手柄设置里的绑定换算成手柄口的按键，所以脚本不需要写通用的键位。没有被手柄绑定截下的键盘事件（没有绑定的键，或键盘属于机器时）与鼠标事件才交给卡带的脚本，用来实现这张卡带自己的外设或热键。没有导出对应回调的卡带忽略这类事件。

| 回调 | 事件 | 说明 |
| --- | --- | --- |
| `onKey(ctx, event)` | `{ code, down }` | `code` 是物理键位（`KeyboardEvent.code`，如 `KeyW`、`Space`、`Numpad1`），`down` 为按下或松开；按住不放的重复按下不上报；已被手柄绑定截下的按下事件不会到这里，松开事件总是同时交给脚本 |
| `onMouse(ctx, event)` | `{ kind, x, y, dx, dy, buttons, wheel }` | `kind` 是 `move`、`down`、`up`、`wheel`、`leave`；`x`、`y` 是指针在标准画面（256×240）上的坐标，指针在画面之外时可以超出范围；`dx`、`dy` 是相对上一次事件的位移；`buttons` 是事件之后按住的按键位（1 左键、2 右键、4 中键）；`wheel` 向下为正，约一格为 1，只有 `wheel` 事件不为 0 |

- 要接收鼠标事件，清单里要声明 `"devices": ["mouse"]`，否则经典 FC 不会把鼠标交给这张卡带。
- `ctx.keys.isDown(code)` 读到的是这次事件之后的状态，写组合键用它。
- `ctx.pad(port).set(button, down)`：按下或松开手柄口 `port` 的一个键。经典 FC 的 `port` 是 0 和 1，`button` 是 `a`、`b`、`select`、`start`、`up`、`down`、`left`、`right`；`ctx.pad(port).release()` 松开这个口的全部按键。参数不合法按脚本错误处理。
- 事件按到达的顺序在模拟线程里执行，最迟在下一帧开始前生效。
- Windows 上画面区获得焦点期间，所有物理按键（包括 Esc、Tab、F 键与 Ctrl、Alt 组合）都上报；其他平台不上报 F1 到 F24、Tab、Escape 与带 Ctrl、Alt、Meta 的组合键，修饰键本身照常上报。

示例：按住右 Shift 时，F 键当作手柄 2 的开始键：

```js
export default {
  onKey(ctx, event) {
    if (event.code === 'KeyF' && ctx.keys.isDown('ShiftRight')) ctx.pad(1).set('start', event.down);
  }
};
```

## 磁带机

声像磁带的卡带把磁带机的数据线接在卡上。磁带机是独立于主机的设备，主机通电时才能播放，播放时按模拟时间推进。

| 接口 | 说明 |
| --- | --- |
| `ctx.device('tape').level()` | 数据线现在的电平，0 或 1；没有播放时是“静音”对应的电平 |
| `ctx.device('tape').setComparator(low, high, invert)` | 设置比较器：数据线取磁带右声道，采样不低于 `high` 为 1、不高于 `low` 为 0，之间保持原值；`invert` 为 `true` 时输出取反；`low`、`high` 是 -32768 到 32767 的整数且 `low` 小于 `high`，否则抛出 `RangeError`；默认是 -3072、3072、不取反；通常在 `reset` 里设置 |
| `onTapeEdge(ctx, level, samples)` | 数据线翻转时调用：`level` 是翻转后的电平，`samples` 是距上一次翻转的采样数（96 kHz；播放刚开始时是距开始播放的采样数）；只在播放时发生 |

- 磁带的接线用清单的 `tape` 就能写出来（见[寄存器与状态](registers.md)）；`ctx.device('tape')` 与 `onTapeEdge` 用于清单写不出的磁带逻辑。
- 主机没有磁带机时，`ctx.device('tape')` 抛出 `RangeError`。
- `level()` 只给当前电平；需要精确的边沿时刻用 `onTapeEdge`。

## 沙箱与限制

- 脚本不能访问文件、网络、进程、系统时间。没有 `Date`、`console`、`performance`、`setTimeout`、`fetch`、`process`、`require`，`Math.random` 被删除；用到它们会得到 `ReferenceError` 或 `TypeError`。需要随机数用 `ctx.random`，它由卡带固定的种子决定，每次上电复位后从头开始，所以同样的操作得到同样的结果。
- `ctx` 与其中的窗口、存储区对象都是冻结的，`ctx.state` 不能增加属性。
- 参数不合法（bank 不是整数或为负、标识不存在、存储区偏移越界、写入值不在 0 到 255、名称表连法不认识）会抛出 `RangeError` 或 `TypeError`，按脚本错误处理。
- 读返回的值必须是 0 到 255 的整数或 `undefined`，否则按脚本错误处理。

| 项 | 限制 |
| --- | --- |
| 运行时内存 | 32 MB |
| 栈 | 256 KB |
| 一次回调的时长 | 超过 50 毫秒即终止 |
| 同时登记的计时名字 | 最多 32 个 |

超出限制或脚本抛出异常时，模拟器暂停并报告：超时、内存或栈超限是 `mappers.limitExceeded`，其他异常与不合法的返回值是 `mappers.scriptError`，说明里带插件标识与脚本位置。脚本顶层代码（加载阶段）同样受时长与内存限制。出错后卡带不再调用脚本，复位后才恢复。处理办法见[测试与排错](debugging.md)。

脚本只在寄存器访问、查表首次计算、复位、计时、输入与磁带翻转时被调用，所以不会拖慢普通的程序执行；但每次回调仍有开销（约几微秒到十几微秒），访问特别频繁的寄存器要写成声明。
