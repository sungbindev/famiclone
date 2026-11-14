/*
 * ------------------------------------------------------------------------
 * 名称：live-check.mjs
 * 说明：对正在运行的 Ember 做真实联调：SDK 连接与握手、类型化请求、截图二进制帧、事件订阅，以及终端客户端与 MCP 服务。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：先 npm run build；要有开启了调试服务的 Ember 在运行，地址与令牌取环境变量 EMBER_DEBUG_URL、EMBER_DEBUG_TOKEN，
 *       或端点文件（EMBER_USER_DATA 指向它的用户数据目录）；EMBER_LIVE_ROM 指定要装入的 NES 文件。
 * ------------------------------------------------------------------------
 */

import { execFileSync, spawn } from 'node:child_process';
import { existsSync, mkdtempSync, readFileSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { dirname, join } from 'node:path';
import { createInterface } from 'node:readline';
import { fileURLToPath, pathToFileURL } from 'node:url';

const dist = join(dirname(fileURLToPath(import.meta.url)), '..', 'dist');
const rom = process.env.EMBER_LIVE_ROM;
if (rom === undefined || !existsSync(rom)) {
  console.error('请用环境变量 EMBER_LIVE_ROM 指定一个存在的 NES 文件');
  process.exit(2);
}

const { connectToEmber } = await import(pathToFileURL(join(dist, 'node.js')).href);
let failed = 0;

/** 记录一项检查。 */
function check(name, ok, detail = '') {
  console.log(`${ok ? '通过' : '失败'}：${name}${detail === '' ? '' : `（${detail}）`}`);
  if (!ok) failed += 1;
}

const client = await connectToEmber({ clientName: 'live-check' });
check('连接并握手', client.info.protocol === 1 && typeof client.info.appVersion === 'string', JSON.stringify(client.info));

const status = await client.api.runtime.status();
check('api.runtime.status 返回运行状态', typeof status.power === 'string', `${status.power}/${status.mode}`);

const loaded = await client.api.cartridges.load({ path: rom });
check('api.cartridges.load 装入卡带并上电', loaded.state.power === 'on', loaded.cartridge.pluginId);

await client.subscribe(['runtime.*']);
const finished = client.waitFor('runtime.runFinished', { timeoutMs: 20_000 });
const run = await client.api.runtime.run({ frames: 120, wait: true });
const event = await finished;
check('runtime.run 等待跑完 120 帧', run.stopped === 'completed' && run.frames === 120, JSON.stringify(run));
check('收到 runtime.runFinished 事件', event.frames === 120 && event.stopped === 'completed');

const raw = await client.api.output.screenshot({ format: 'rgba' });
check('截图（RGBA）以二进制帧到达，data 是 Uint8Array', raw.data instanceof Uint8Array && raw.data.byteLength === 256 * 240 * 4 && raw.width === 256, `${raw.data?.byteLength}`);
const png = await client.api.output.screenshot();
check('截图（PNG）的数据以 PNG 签名开头', png.data instanceof Uint8Array && png.data[0] === 0x89 && png.data[1] === 0x50 && png.format === 'png', `${png.byteLength} 字节`);
const nonBlack = raw.data.some((value, index) => index % 4 !== 3 && value !== 0);
check('画面不是全黑', nonBlack);

await client.api.input.pad({ port: 0, buttons: 8 });
await client.api.input.releaseAll();
check('input.pad 与 input.releaseAll 成功', true);
await expectRequestError(() => client.api.input.pad({ port: 7, buttons: 1 }), 'input.invalidPort');
await expectRequestError(() => client.api.runtime.run({ frames: 0 }), 'core.invalidParams');

/** 请求应以给定错误码失败。 */
async function expectRequestError(action, code) {
  try {
    await action();
    check(`错误码 ${code}`, false, '没有失败');
  } catch (error) {
    check(`错误码 ${code}`, error?.code === code, `${error?.code}`);
  }
}

const discovered = await client.api.rpc.discover();
check('rpc.discover 返回 OpenRPC 文档', discovered.openrpc === '1.3.2' && discovered.methods.length > 30, `${discovered.methods.length} 个方法`);
client.close();
await client.closed;
check('关闭后 closed 兑现', true);

// 终端客户端：读端点文件，不给地址与令牌。
const cli = (...args) => execFileSync(process.execPath, [join(dist, 'cli.js'), ...args], { encoding: 'utf8', env: process.env });
check('ember-debug endpoint 显示地址', JSON.parse(cli('endpoint', '--compact')).url.startsWith('ws://127.0.0.1:'));
check('ember-debug call runtime.status', JSON.parse(cli('call', 'runtime.status', '--compact')).power === 'on');
const work = mkdtempSync(join(tmpdir(), 'ember-live-'));
const shot = join(work, 'shot.png');
cli('screenshot', shot);
check('ember-debug screenshot 保存 PNG', readFileSync(shot).subarray(1, 4).toString() === 'PNG');
rmSync(work, { recursive: true, force: true });

// MCP 服务：经标准输入输出走一遍协议。
const server = spawn(process.execPath, [join(dist, 'mcp.js')], { env: process.env, stdio: ['pipe', 'pipe', 'inherit'] });
const lines = createInterface({ input: server.stdout });
const waiting = new Map();
lines.on('line', (line) => {
  const message = JSON.parse(line);
  waiting.get(message.id)?.(message);
});
let nextId = 1;
/** 发一个 MCP 请求并等回复。 */
function mcp(method, params = {}) {
  const id = nextId++;
  return new Promise((resolve) => {
    waiting.set(id, resolve);
    server.stdin.write(`${JSON.stringify({ jsonrpc: '2.0', id, method, params })}\n`);
  });
}
const init = await mcp('initialize', { protocolVersion: '2025-06-18', capabilities: {}, clientInfo: { name: 'live-check', version: '1' } });
check('MCP initialize', init.result?.serverInfo?.name === 'ember-debug-mcp');
const tools = await mcp('tools/list');
check('MCP tools/list 含 ember_runtime_run 与 ember_events', ['ember_runtime_run', 'ember_events'].every((name) => tools.result.tools.some((tool) => tool.name === name)), `${tools.result.tools.length} 个工具`);
const mcpStatus = await mcp('tools/call', { name: 'ember_runtime_status', arguments: {} });
check('MCP 调用 ember_runtime_status', JSON.parse(mcpStatus.result.content[0].text).power === 'on');
const mcpShot = await mcp('tools/call', { name: 'ember_output_screenshot', arguments: {} });
check('MCP 截图返回 PNG 图像块', mcpShot.result.content.some((block) => block.type === 'image' && block.mimeType === 'image/png' && block.data.startsWith('iVBOR')));
const mcpError = await mcp('tools/call', { name: 'ember_input_pad', arguments: { port: 7, buttons: 1 } });
check('MCP 把请求错误作为工具错误返回', mcpError.result.isError === true && mcpError.result.content[0].text.includes('input.invalidPort'));
await mcp('tools/call', { name: 'ember_events', arguments: { patterns: ['runtime.*'] } });
await mcp('tools/call', { name: 'ember_runtime_run', arguments: { frames: 5, wait: true } });
const mcpEvents = await mcp('tools/call', { name: 'ember_events', arguments: {} });
check('MCP ember_events 读到运行事件', JSON.parse(mcpEvents.result.content[0].text).events.some((item) => item.event === 'runtime.runFinished'));
server.stdin.end();

console.log(failed === 0 ? '联调全部通过' : `联调有 ${failed} 项失败`);
process.exit(failed === 0 ? 0 : 1);
