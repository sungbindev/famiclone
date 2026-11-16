# 客户端工具

官方的客户端工具是 npm 包 `ember-debug-sdk`，包含三样东西。协议是开放的，也可以不用它，直接按[连接与报文](protocol.md)连接。

| 工具 | 说明 |
| --- | --- |
| SDK | 类型完整的 WebSocket 客户端：给定地址与令牌连接（浏览器与 Electron 页面也能用），或在 Node 里自动找到正在运行的 Ember；`client.api.runtime.run(...)` 这样按模块分组的类型化请求；`subscribe`、`on`、`waitFor` 处理事件；二进制结果是 `Uint8Array` |
| 终端客户端 `ember-debug` | `endpoint`、`methods`、`describe`、`call`、`screenshot`、`watch` |
| MCP 服务 `ember-debug-mcp` | 给 Copilot 等智能体用：每个请求一个工具（`ember_<模块>_<操作>`），`ember_events` 读事件，PNG 截图作为图像返回 |

需要 Node 22.12 或更高版本，用 `npm install ember-debug-sdk` 安装；要全局使用终端客户端与 MCP 服务时加 `-g`。

## 自动连接

Ember 开始监听后会写端点文件（见[连接与报文](protocol.md)），三种工具都读它自动连接。也可以用环境变量或命令行参数指定：

| 变量 | 说明 |
| --- | --- |
| `EMBER_DEBUG_URL`、`EMBER_DEBUG_TOKEN` | 直接指定调试服务的地址与令牌 |
| `EMBER_DEBUG_ENDPOINT` | 端点文件的路径 |
| `EMBER_USER_DATA` | Ember 的用户数据目录（用 `--user-data-dir` 启动了第二个实例时用它区分） |
| `EMBER_MCP_TIMEOUT_MS` | MCP 服务里每个请求等响应的毫秒数，默认 120000 |

查找顺序：显式参数、环境变量、端点文件。端点文件里的进程已退出时视为过期，不会连接。

## 在代码里使用

```ts
import { connectToEmber } from 'ember-debug-sdk/node';

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
ember-debug endpoint                          显示正在运行的 Ember 的调试服务地址
ember-debug methods [关键字]                  列出全部请求
ember-debug describe runtime.run              显示参数、结果、错误码与示例（也可以查事件）
ember-debug call cartridges.load path=D:/roms/game.nes
ember-debug call runtime.run frames=300 wait=true
ember-debug screenshot shot.png               截图保存为 PNG
ember-debug watch runtime.* --timeout 5000    订阅事件并逐行打印
```

`call` 的参数写成 `key=value`（值按 JSON 解析，不是 JSON 就当字符串），或一个 JSON 对象；结果以 JSON 打印，二进制数据显示字节数，加 `--out 文件` 保存。退出码：0 成功，1 失败，2 用法错误。`ember-debug --help` 查看全部用法。

## MCP 服务

`ember-debug-mcp` 是标准输入输出的 MCP 服务，把调试服务的每个请求做成一个工具，输入参数由接口定义生成；第一次调用工具时才连接 Ember，连接断了下次调用自动重连。VS Code 里在 `.vscode/mcp.json` 配置：

```json
{
  "servers": {
    "ember": {
      "type": "stdio",
      "command": "ember-debug-mcp"
    }
  }
}
```
