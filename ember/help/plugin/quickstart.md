# 第一个插件

这一页从零写一个插件：`$8000-$FFFF` 写入的值选择程序 ROM 的 bank，用声明式寄存器实现，不需要脚本；最后再给出等价的脚本写法。示例里的 mapper 号 999 是随便写的，用的时候换成你要支持的卡带的 mapper 号。

## 第一步：确定卡带的信息

打开你要支持的 `.nes` 文件，提示“缺少 mapper 号 N、变体 V 的插件”时，N 和 V 就是要写进清单的匹配条件。再用 FC 资料弄清这块 mapper 的行为：哪个地址范围是寄存器，写入的值怎么决定切哪一段 ROM，固定的部分在哪里。

示例 mapper 的行为是：

- `$8000-$BFFF` 是 16K 可切换的程序窗口，往 `$8000-$FFFF` 写入的值就是 bank 号。
- `$C000-$FFFF` 固定为程序 ROM 的最后 16K。
- 图案使用 8K，文件没有 CHR ROM 时用 CHR RAM。

## 第二步：写清单

新建文件夹 `demo-bank`，在里面建 `manifest.json`：

```json
{
  "id": "demo-bank",
  "name": "示例：切换 bank",
  "version": "1.0.0",
  "apiVersion": 1,
  "publisher": "示例发布者",
  "description": "$8000-$FFFF 写入选择 $8000 起 16K 的程序 bank，$C000 起 16K 固定为最后一个 bank。",
  "host": ["fc"],
  "match": [{ "mapper": 999 }],
  "storage": [
    { "id": "prg", "kind": "rom", "from": "file.prg" },
    { "id": "chr", "kind": "rom", "from": "file.chr", "fallback": { "kind": "ram", "size": 8192 } }
  ],
  "windows": [
    { "id": "prg-low", "bus": "cpu", "range": "$8000-$BFFF", "storage": "prg", "size": 16384, "initial": 0, "access": "r" },
    { "id": "prg-high", "bus": "cpu", "range": "$C000-$FFFF", "storage": "prg", "size": 16384, "initial": "last", "access": "r" },
    { "id": "chr-0000", "bus": "ppu", "range": "$0000-$1FFF", "storage": "chr", "size": 8192, "initial": 0, "access": "rw" }
  ],
  "registers": [
    { "bus": "cpu", "range": "$8000-$FFFF", "write": { "state": "prgBank", "bank": [{ "window": "prg-low" }] } }
  ],
  "state": [
    { "name": "prgBank", "bits": 8, "description": "$8000 起 16K 窗口的 bank 号" }
  ],
  "nametable": "header"
}
```

逐段看：

| 字段 | 作用 |
| --- | --- |
| `id`、`name`、`version`、`apiVersion`、`publisher` | 插件的标识、显示名、版本、接口版本（目前只能是 `1`）与发布者 |
| `host` | 要求的主机型号，卡带目前只有 `fc` |
| `match` | 匹配条件：mapper 号等于 999 |
| `storage` | 两块存储区：`prg` 是文件里的程序 ROM；`chr` 是文件里的图案 ROM，文件没有时改用 8K 的 RAM |
| `windows` | 三个窗口：CPU 的 `$8000-$BFFF` 指向 `prg`，复位后指向第 0 个 16K；`$C000-$FFFF` 指向最后一个 16K（`last`）；PPU 的 `$0000-$1FFF` 指向 `chr` |
| `registers` | 寄存器：CPU 写 `$8000-$FFFF` 时，把写入的值存进状态 `prgBank`，并把窗口 `prg-low` 切到这个 bank |
| `state` | 声明状态 `prgBank`，8 位 |
| `nametable` | 名称表的连法按文件头里的镜像方式 |

bank 号超过实际的 bank 数时自动取模，所以 64K 的 PRG（4 个 16K）写入 7 等于选第 3 个。

## 第三步：打包

插件包是扩展名为 `.emap` 的 zip 文件，根目录里直接放 `manifest.json`（有脚本时还有 `main.js`）。

在 Windows 的资源管理器里打开 `demo-bank` 文件夹，选中里面的文件（不是文件夹本身），右键“发送到→压缩(zipped)文件夹”，再把生成的文件改名为 `demo-bank.emap`。也可以在 PowerShell 里：

```powershell
Compress-Archive -Path .\demo-bank\* -DestinationPath demo-bank.zip
Rename-Item demo-bank.zip demo-bank.emap
```

其他打包方式和包里的限制见[插件包与安装](package.md)。

## 第四步：安装并运行

1. 点“设置→mapper 插件”，点“安装插件”，选择 `demo-bank.emap`。安装失败时通知会写出原因。
2. 在表格里能看到“示例：切换 bank”，状态是“已启用”。
3. 打开 mapper 号为 999 的 `.nes` 文件，系统栏会显示卡带名和用到的插件。

## 改写成脚本

同样的 mapper 也可以用脚本实现。把清单里的 `registers` 改成回调脚本：

```json
"registers": [
  { "bus": "cpu", "range": "$8000-$FFFF", "write": true }
]
```

再在 `demo-bank` 里加一个 `main.js`：

```js
export default {
  reset(ctx) {
    ctx.state.prgBank = 0;
    ctx.window('prg-low').setBank(0);
  },
  write(ctx, bus, addr, value) {
    ctx.state.prgBank = value;
    ctx.window('prg-low').setBank(value);
  }
};
```

打包时把 `manifest.json` 和 `main.js` 一起放进去。

- `reset` 在复位时调用，参数 `kind` 是 `'power'`（上电）或 `'soft'`（软复位），这里没有用到。
- `write` 在 CPU 写入 `$8000-$FFFF` 时调用，`addr` 是写入的地址，`value` 是写入的值。
- 脚本里没有 `import`，直接 `export default` 一个对象。

声明式和脚本两种写法的区别：声明式由 Ember 直接执行，快，适合每帧写很多次的寄存器；脚本能写任意逻辑，但每次回调都有开销，所以能声明的就声明，见[寄存器与状态](registers.md)与[脚本接口](script.md)。

## 下一步

- 卡带出了问题：[测试与排错](debugging.md)。
- 想知道某种写法怎么做：[常见写法](recipes.md)。
