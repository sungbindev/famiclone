# ember-debug-sdk

Ember（FC 与学习机模拟器）调试服务的 SDK：类型完整的 WebSocket 客户端，终端客户端 `ember-debug`，以及给 Copilot 等智能体用的 MCP 服务 `ember-debug-mcp`。

调试服务是 Ember 核心里的一个可开关的 WebSocket 服务（JSON-RPC 2.0），模拟器的全部功能都有接口。本 SDK 是它的官方客户端；协议是开放的，也可以用任何语言直接连接，见 [协议参考](../../ember/docs/api/README.md)。

## 开启调试服务

1. 在 Ember 的“首选项”里打开“调试服务”分组，启用调试服务（端口默认 5688，令牌自动生成）；或者用启动参数 `--debug-port=5688` 只在本次运行开启。
2. Ember 开始监听后，会在用户数据目录写端点文件 `run/debug-endpoint.json`（地址与令牌）。SDK、终端客户端与 MCP 服务都读它自动连接，不用手抄端口与令牌。

只监听 `127.0.0.1`，每条连接都要先用令牌握手。

## 安装

需要 Node 22.12 或更高版本。用 npm 安装：`npm install ember-debug-sdk`；要全局使用终端客户端与 MCP 服务时加 `-g`。从 famiclone 仓库源码构建的方法：先在仓库根目录运行 `npm install`，再在 `packages/ember-debug-sdk` 目录里运行：

```text
npm run schema     # 生成接口类型与接口文档（等于在 ember 目录运行 npm run schema）
npm run build
npm pack        # 生成 .tgz，再在你的项目里 npm install 它
```

## 在代码里使用

```ts
import { connectToEmber } from 'ember-debug-sdk/node';

const client = await connectToEmber(); // 读端点文件；也可以传 { url, token }

await client.api.cartridges.load({ path: 'D:/roms/game.nes' });
await client.subscribe(['runtime.*']);
const result = await client.api.runtime.run({ frames: 300, wait: true }); // 不限速跑 300 帧后暂停
console.log(result.stopped, result.frames);

const shot = await client.api.output.screenshot(); // PNG，shot.data 是 Uint8Array
await client.api.input.pad({ port: 0, buttons: 0b0000_1000 }); // 按下开始键

client.close();
```

- 在浏览器或 Electron 页面里用 `import { connect } from 'ember-debug-sdk'`，给出 `url` 与 `token`；这个入口不引用 Node 内置模块。
- 请求的参数与结果、事件的数据都有完整的 TypeScript 类型，由 Ember 的接口定义生成；`client.request('runtime.run', { frames: 1 })` 与 `client.api.runtime.run({ frames: 1 })` 等价。
- 结果里的二进制数据（截图、以后的内存内容）是 `Uint8Array`。
- 要等事件：先 `subscribe`，再 `on` / `onAny` / `waitFor`。没有订阅不会收到任何事件。

```ts
await client.subscribe(['runtime.runFinished']);
const finished = client.waitFor('runtime.runFinished', { timeoutMs: 30_000 });
await client.api.runtime.run({ frames: 600 }); // 不等待，立即返回
console.log(await finished);
```

### 错误

| 错误类型 | 含义 |
| --- | --- |
| `EmberRequestError` | 服务端返回了错误。`code` 是模块里登记的错误码（如 `runtime.noCartridge`），`rpcCode` 是 JSON-RPC 错误码 |
| `EmberConnectionError` | 连接类错误，`kind` 为 `unavailable`（连不上）、`closed`（连接关闭，`closeCode` 是关闭码）、`timeout`、`protocol`、`endpoint`（找不到或读不了端点文件） |

请求默认等 60 秒（`requestTimeoutMs`，0 表示一直等）；单个请求可以用第三个参数 `{ timeoutMs }` 指定。超时只是客户端不再等，服务端仍会把请求执行完。同一条连接上的请求在服务端按到达顺序逐个处理，要并行就开多条连接。

## 终端客户端

```text
ember-debug endpoint                          显示正在运行的 Ember 的调试服务地址
ember-debug methods [关键字]                  列出全部请求
ember-debug describe runtime.run              显示参数、结果、错误码与示例（也可以查事件）
ember-debug call cartridges.load path=D:/roms/game.nes
ember-debug call runtime.run frames=300 wait=true
ember-debug call input.key '{"code":"Enter","down":true}'
ember-debug screenshot shot.png               截图保存为 PNG
ember-debug watch runtime.* --timeout 5000    订阅事件并逐行打印
```

`call` 的参数写成 `key=value`（值按 JSON 解析，不是 JSON 就当字符串；`0x8000`、`$8000` 是十六进制数，要当字符串就加引号，如 `file='"0x10"'`），或一个 JSON 对象；结果以 JSON 打印，二进制数据显示字节数，加 `--out 文件` 保存。选项 `--url`、`--token`、`--endpoint-file`、`--user-data-dir` 指定连接目标，`--compact` 不缩进输出。退出码：0 成功，1 失败，2 用法错误。`ember-debug --help` 看全部用法。

## MCP 服务（给 Copilot 等智能体）

`ember-debug-mcp` 是标准输入输出的 MCP 服务，把调试服务的每个请求做成一个工具（名称 `ember_<模块>_<操作>`，如 `ember_runtime_run`、`ember_output_screenshot`），输入 Schema 由接口定义生成；`ember_events` 读取 Ember 发出的事件；PNG 截图作为图像返回，智能体可以直接看画面。第一次调用工具时才连接 Ember，连接断了下次调用自动重连。

VS Code 的 `.vscode/mcp.json`：

```json
{
  "servers": {
    "ember": {
      "type": "stdio",
      "command": "node",
      "args": ["D:/GitCode/famiclone/packages/ember-debug-sdk/dist/mcp.js"]
    }
  }
}
```

安装了本包之后也可以把 `command` 写成 `ember-debug-mcp`。

## 环境变量

| 变量 | 说明 |
| --- | --- |
| `EMBER_DEBUG_URL`、`EMBER_DEBUG_TOKEN` | 直接指定调试服务的地址与令牌 |
| `EMBER_DEBUG_ENDPOINT` | 端点文件的路径 |
| `EMBER_USER_DATA` | Ember 的用户数据目录（用 `--user-data-dir` 启动了第二个实例时用它区分） |
| `EMBER_MCP_TIMEOUT_MS` | MCP 服务里每个请求等响应的毫秒数，默认 120000 |

查找顺序：显式参数，环境变量，端点文件（默认在 Windows 的 `%APPDATA%\Ember`、macOS 的 `~/Library/Application Support/Ember`、Linux 的 `~/.config/Ember` 下的 `run` 目录）。端点文件里的进程已退出时视为过期，不会连接。

## 接口文档

- [协议参考](../../ember/docs/api/README.md)：每个请求的参数、结果、错误码与事件，由接口定义生成；也可以用终端客户端的 `describe` 查看。
- SDK 的接口参考：`npm run docs` 生成到 `docs/api`（在 `packages/ember-debug-sdk` 目录里，不提交）。
- 接口定义本身是 OpenRPC 1.3 文档（[openrpc.json](generated/openrpc.json)），连接后用 `rpc.discover` 也能取到，可以据此生成其他语言的客户端。

## 版本

协议版本在握手时校验（目前是 1）。请求与事件的类型来自 `ember/schema/`，与 Ember 版本同步生成；Ember 新增请求后，重新生成并发布 SDK 即可使用新的类型和工具。老版本的 SDK 不认识新请求，其余请求照常可用。
