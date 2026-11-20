# 客户端工具

官方的客户端工具随 npm 包 `fcdev` 一起安装（不单独发布），包含两样东西。协议是开放的，也可以不用它，直接按[连接与报文](protocol.md)连接。

| 工具 | 说明 |
| --- | --- |
| SDK | 类型完整的 WebSocket 客户端：给定地址与令牌连接（浏览器页面也能用），或在 Node 里自动找到正在运行的 Ember；`client.api.runtime.run(...)` 这样按模块分组的类型化请求；`subscribe`、`on`、`waitFor` 处理事件；二进制结果是 `Uint8Array` |
| 终端客户端 `fcdev debug` | `endpoint`、`methods`、`describe`、`call`、`screenshot`、`watch` |

需要 Node 22.12 或更高版本，用 `npm install --save-dev fcdev` 安装，再用 `npx fcdev debug` 运行；要直接输入 `fcdev` 时用 `npm install -g fcdev` 全局安装。下面的命令省略 `npx`。

## 自动连接

Ember 开始监听后会写端点文件（见[连接与报文](protocol.md)），两种工具都读它自动连接。也可以用环境变量或命令行参数指定：

| 变量 | 说明 |
| --- | --- |
| `EMBER_DEBUG_URL`、`EMBER_DEBUG_TOKEN` | 直接指定调试服务的地址与令牌 |
| `EMBER_DEBUG_ENDPOINT` | 端点文件的路径 |
| `EMBER_USER_DATA` | Ember 的用户数据目录（用 `--user-data-dir` 启动了第二个实例时用它区分） |

查找顺序：显式参数、环境变量、端点文件。端点文件里的进程已退出时视为过期，不会连接。

## 在代码里使用

```ts
import { connectToEmber } from 'fcdev/dist/debug/node.js';

const client = await connectToEmber(); // 读端点文件；也可以传 { url, token }

await client.api.cartridges.load({ path: 'D:/roms/game.nes' });
const result = await client.api.runtime.run({ frames: 300, wait: true });
console.log(result.stopped, result.frames);

const shot = await client.api.output.screenshot(); // PNG，shot.data 是 Uint8Array
client.close();
```

要等事件：先 `subscribe`，再 `on`、`onAny` 或 `waitFor`，没有订阅不会收到任何事件。

```ts
await client.subscribe(['runtime.runFinished']);
const finished = client.waitFor('runtime.runFinished', { timeoutMs: 30_000 });
await client.api.runtime.run({ frames: 600 }); // 不等待，立即返回
console.log(await finished);
```

请求失败时抛出 `EmberRequestError`（`code` 是模块里登记的错误码，如 `runtime.noCartridge`），连接类错误抛出 `EmberConnectionError`。请求默认最多等 60 秒。

## 终端客户端

```text
fcdev debug endpoint                          显示正在运行的 Ember 的调试服务地址
fcdev debug methods [关键字]                  列出全部请求
fcdev debug describe runtime.run              显示参数、结果、错误码与示例（也可以查事件）
fcdev debug call cartridges.load path=D:/roms/game.nes
fcdev debug call runtime.run frames=300 wait=true
fcdev debug screenshot shot.png               截图保存为 PNG
fcdev debug watch runtime.* --timeout 5000    订阅事件并逐行打印
```

`call` 的参数写成 `key=value`（值按 JSON 解析，不是 JSON 就当字符串），或一个 JSON 对象；结果以 JSON 打印，二进制数据显示字节数，加 `--out 文件` 保存。退出码：0 成功，1 失败，2 用法错误。`fcdev debug --help` 查看全部用法。
