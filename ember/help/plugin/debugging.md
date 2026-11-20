# 测试与排错

## 出错时的表现

| 阶段 | 表现 |
| --- | --- |
| 安装 | 通知里写出被拒绝的原因，指出是哪个字段或哪个文件有问题，见[插件包与安装](package.md) |
| 打开卡带 | 没有匹配的插件时提示缺少哪个 mapper 号和变体；多个插件同样匹配时提示冲突并列出候选，见[清单：基本字段与匹配](manifest.md) |
| 运行 | 脚本抛出异常、返回不合法的值或超出限制时，模拟器暂停，原因（插件标识与脚本位置）写进错误日志；调试服务开着时还会推送 `runtime.error` 事件。点“帮助→打开日志目录”可以找到日志 |

脚本出错后，卡带不再调用脚本：对寄存器的读按开放总线处理，写被丢弃。复位之后错误被清除，用新的插件重新运行要重新打开卡带。

脚本错误的两类错误码：

| 错误码 | 含义 |
| --- | --- |
| `mappers.scriptError` | 脚本异常（如访问了不存在的窗口、写了未声明的状态）或返回了不合法的值 |
| `mappers.limitExceeded` | 超时（一次回调超过 50 毫秒）、内存或栈超限 |

## 常见问题

| 现象 | 可能的原因 |
| --- | --- |
| 打开卡带提示缺少插件 | 插件没装或被停用；`match` 里的 mapper 号或变体与文件头不符；`match` 的条件写得太严（如多写了 `prgSize`） |
| 提示冲突 | 另一个插件的匹配条件和你的一样多，停用不需要的那个，或在你的 `match` 里多写一个条件 |
| 安装被拒绝 | 清单里有不认识的字段、窗口地址范围重叠、窗口 `size` 与地址范围大小不符、引用了不存在的状态或窗口、`access` 与存储区种类不符、声明了查表但脚本没有导出对应函数 |
| 黑屏或一开机就跑飞 | 复位向量 `$FFFC` 所在的窗口复位后没有指向正确的 bank，通常是 `initial` 写错，如最后一个 bank 要写 `"last"` 或 `-1` |
| 切 bank 后内容不对 | 窗口的 `size` 与实际 bank 大小不一致；写入值的取位（`shift`、`mask`）不对；bank 号被取模回绕了 |
| 中断一直触发 | 没有在应答的寄存器里调用 `ctx.irq.release()`，或声明式读寄存器没写 `releaseIrq` |
| 中断根本不触发 | 用 A12 计数的卡带脚本没有导出 `onA12`（卡带不会收到显存地址）；计时用了 `ctx.timer.after` 但周期数算错 |
| 复位后行为和预期不同 | 脚本里用 `let` 存的变量不会随复位清零，也看不到，改成声明状态；软复位不会重置窗口与状态，要在 `reset` 里自己处理 |
| 脚本访问 `console`、`Date` 报错 | 沙箱里没有这些对象，见[脚本接口](script.md) |

## 用调试服务验证

Ember 内置的调试服务可以不靠真实游戏就验证插件：安装插件、装入卡带、读写寄存器、看窗口落在哪里、看状态值。打开方式与连接见[调试服务概述](../debug/index.md)，下面用官方的终端客户端 `fcdev debug`（省略 `npx`），见[客户端工具](../debug/clients.md)。

### 做一个最小的测试 ROM

文件头的 mapper 号和变体决定匹配哪个插件，所以测试时可以自己做一个空的 ROM：程序区每个 16K 的 bank 开头填上 bank 号，之后读 `$8000` 就知道现在映射的是哪一块。用 Node 生成（NES 2.0 文件头，mapper 号可以超过 255）：

```js
import { writeFileSync } from 'node:fs';

const mapper = 999; // 换成你的 mapper 号
const variant = 0; // 换成你的变体
const prgBanks = 4; // 程序区有几个 16K
const chrBanks = 0; // 图案区有几个 8K，0 表示没有 CHR ROM

const header = Buffer.alloc(16);
header.write('NES\x1a', 0, 'latin1');
header[4] = prgBanks;
header[5] = chrBanks;
header[6] = (mapper & 0x0f) << 4;
header[7] = 0x08 | (mapper & 0xf0); // 位 2-3 为 2 表示 NES 2.0
header[8] = ((variant & 0x0f) << 4) | ((mapper >> 8) & 0x0f);

const prg = Buffer.alloc(prgBanks * 16384);
for (let bank = 0; bank < prgBanks; bank += 1) prg[bank * 16384] = bank;
writeFileSync('test.nes', Buffer.concat([header, prg, Buffer.alloc(chrBanks * 8192)]));
```

### 一次验证

先在“设置→首选项”里打开调试服务，再在终端里：

```text
fcdev debug call mappers.install path=D:/work/demo-bank.emap
fcdev debug call cartridges.load path=D:/work/test.nes
fcdev debug call inspector.resolve space=cpu address=32768
fcdev debug call inspector.writeMemory space=cpu address=32768 bytes=[2]
fcdev debug call inspector.resolve space=cpu address=32768
fcdev debug call inspector.getState module=mapper
```

- `mappers.install` 装入（或替换）插件。
- `cartridges.load` 装入测试 ROM，成功就说明匹配到了你的插件，`cartridges.info` 能看到匹配的插件和存储区清单。
- `inspector.resolve` 解析一个总线地址，告诉你它落在哪块存储区的哪个偏移：32768 是 `$8000`。切 bank 前后各查一次，偏移应该变化。
- `inspector.writeMemory` 经真实的总线写入，和 CPU 写一样会触发寄存器，所以可以直接模拟游戏对寄存器的写入。
- `inspector.getState` 的 `mapper` 模块列出卡带上声明的全部状态当前的值。

`inspector.readMemory` 读内存不会触发寄存器的回调，需要看卡带存储区的内容时，先用 `inspector.listSpaces` 查空间标识（卡带的存储区是 `cart.<存储区标识>`）。

### 监视状态的变化

```text
fcdev debug call inspector.watch path=mapper.prgBank
fcdev debug watch inspector.* --timeout 5000
```

`mapper.` 后面是清单里声明的状态名；取值变化时发 `inspector.stateChanged` 事件。运行游戏时用它观察寄存器被写成了什么。

### 改了插件之后

1. 重新打包，用 `mappers.install` 再装一次（同标识的插件直接替换）。
2. 用 `cartridges.load` 重新装入卡带，新的插件才会生效。
3. 脚本出过错的话，错误在重新装入后清除。

每个请求的参数与结果见[请求与事件总览](../api/index.md)，运行控制见[运行控制与典型流程](../debug/control.md)。

## 写插件时的几个习惯

- 能声明的寄存器就声明，脚本只写声明表达不了的部分。
- 脚本里要观察、要随复位变化的数据放进 `state`，不用模块里的 `let` 变量。
- 查表的计算函数写成纯函数，不读也不改任何会变的东西。
- 每改一次清单就安装一次，让 Ember 的检查尽早指出问题，而不是写完一大堆再装。
- 没有真实游戏可测时，用上面的测试 ROM 加 `inspector.writeMemory`、`inspector.resolve` 逐个寄存器检查窗口落点。
