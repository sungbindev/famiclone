/*
 * ------------------------------------------------------------------------
 * 名称：cli.test.ts
 * 说明：终端客户端的测试：参数解析、各命令的输出、二进制数据的保存、错误与退出码。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import { describe, expect, it } from 'vitest';

import { parseCallParams, printable, UsageError } from '../src/cli/arguments.js';
import { runCli, type CliContext } from '../src/cli/commands.js';
import { OPENRPC_DOCUMENT } from '../src/entry/document.js';
import { EmberDebugClient } from '../src/logic/debug-client.js';
import { EmberConnectionError } from '../src/shared/errors.js';
import { FakeTransport, HELLO_RESULT, type Responder } from './support/fake-transport.js';

/** 假的运行环境：记录输出与写出的文件，连接用假传输层。 */
function createContext(responder: Responder = () => undefined, overrides: Partial<CliContext> = {}) {
  const out: string[] = [];
  const err: string[] = [];
  const files = new Map<string, Uint8Array>();
  const targets: unknown[] = [];
  const transport = new FakeTransport((request) => (request.method === 'debug.hello' ? { result: HELLO_RESULT } : responder(request)));
  const ctx: CliContext = {
    document: OPENRPC_DOCUMENT,
    version: '9.9.9',
    out: (text) => out.push(text),
    err: (text) => err.push(text),
    readEndpoint: () => ({ url: 'ws://127.0.0.1:5688/', port: 5688, token: 'abcdef123456', pid: 1, coreVersion: '0.1.0', appVersion: '0.1.0', startedAt: 'now' }),
    connect: (target) => {
      targets.push(target);
      return EmberDebugClient.open({ url: 'ws://x/', token: 't', transportFactory: async () => transport });
    },
    writeFile: (path, bytes) => files.set(path, bytes),
    now: () => new Date(2026, 10, 14, 8, 5, 9),
    ...overrides
  };
  return { ctx, out, err, files, transport, targets };
}

describe('parseCallParams', () => {
  it('没有参数是空对象；key=value 的值按 JSON 解析，不是 JSON 就当字符串', () => {
    expect(parseCallParams([])).toEqual({});
    expect(parseCallParams(['frames=60', 'wait=true', 'path=D:/roms/a b.nes', 'name="x"', 'list=[1,2]'])).toEqual({ frames: 60, wait: true, path: 'D:/roms/a b.nes', name: 'x', list: [1, 2] });
    // 十六进制整数：0x 或 $ 开头；加引号当字符串
    expect(parseCallParams(['address=0x8000', 'start=$C000', 'last=0XFF', 'file="0x10"', 'name=0xZZ'])).toEqual({ address: 0x8000, start: 0xc000, last: 0xff, file: '0x10', name: '0xZZ' });
  });

  it('一个 JSON 对象也可以', () => {
    expect(parseCallParams(['{"frames":3}'])).toEqual({ frames: 3 });
  });

  it('写法不对时抛出 UsageError', () => {
    expect(() => parseCallParams(['frames'])).toThrow(UsageError);
    expect(() => parseCallParams(['=1'])).toThrow(UsageError);
    expect(() => parseCallParams(['{bad'])).toThrow('JSON');
    expect(() => parseCallParams(['{"a":1}', 'b=2'])).toThrow('一个参数');
  });
});

describe('printable', () => {
  it('把二进制换成说明文字', () => {
    expect(printable({ a: [new Uint8Array(3)], b: 1 })).toEqual({ a: ['<二进制 3 字节>'], b: 1 });
  });
});

describe('runCli', () => {
  it('没有命令显示帮助并返回 2，--help 返回 0，--version 显示版本', async () => {
    const none = createContext();
    expect(await runCli([], none.ctx)).toBe(2);
    expect(none.out[0]).toContain('用法：ember-debug');
    expect(await runCli(['--help'], createContext().ctx)).toBe(0);
    const version = createContext();
    expect(await runCli(['--version'], version.ctx)).toBe(0);
    expect(version.out).toEqual(['9.9.9']);
  });

  it('未知命令与未知选项返回 2', async () => {
    const unknown = createContext();
    expect(await runCli(['fly'], unknown.ctx)).toBe(2);
    expect(unknown.err[0]).toContain('没有这个命令');
    const option = createContext();
    expect(await runCli(['methods', '--nope'], option.ctx)).toBe(2);
  });

  it('endpoint 默认只显示令牌前 4 位，--show-token 显示完整', async () => {
    const masked = createContext();
    expect(await runCli(['endpoint', '--compact'], masked.ctx)).toBe(0);
    expect(JSON.parse(masked.out[0] ?? '').token).toBe('abcd…（共 12 个字符）');
    const shown = createContext();
    await runCli(['endpoint', '--show-token', '--compact'], shown.ctx);
    expect(JSON.parse(shown.out[0] ?? '').token).toBe('abcdef123456');
  });

  it('连接参数经上下文传入', async () => {
    const { ctx, targets } = createContext(() => ({ result: {} }));
    await runCli(['call', 'runtime.status', '--url', 'ws://z/', '--token', 'tk', '--endpoint-file', 'e.json', '--user-data-dir', 'ud'], ctx);
    expect(targets[0]).toEqual({ url: 'ws://z/', token: 'tk', endpointFile: 'e.json', userDataDirectory: 'ud' });
  });

  it('methods 列出全部请求并标出调试专用，可按关键字过滤', async () => {
    const all = createContext();
    expect(await runCli(['methods'], all.ctx)).toBe(0);
    expect(all.out.some((line) => line.startsWith('runtime.run') && line.includes('[调试]'))).toBe(true);
    const filtered = createContext();
    await runCli(['methods', 'screenshot'], filtered.ctx);
    expect(filtered.out).toHaveLength(1);
    expect(await runCli(['methods', 'xyz没有'], createContext().ctx)).toBe(2);
  });

  it('describe 显示请求与事件，不存在时返回 2', async () => {
    const method = createContext();
    expect(await runCli(['describe', 'runtime.run'], method.ctx)).toBe(0);
    expect(method.out[0]).toContain('frames: integer');
    const event = createContext();
    expect(await runCli(['describe', 'runtime.runFinished'], event.ctx)).toBe(0);
    expect(event.out[0]).toContain('事件，需要先订阅');
    expect(await runCli(['describe', 'nope.nothing'], createContext().ctx)).toBe(2);
    expect(await runCli(['describe'], createContext().ctx)).toBe(2);
  });

  it('call 发请求并打印结果，二进制 data 只显示字节数，--out 保存到文件', async () => {
    const plain = createContext(() => ({ result: { power: 'on' } }));
    expect(await runCli(['call', 'runtime.status', '--compact'], plain.ctx)).toBe(0);
    expect(JSON.parse(plain.out[0] ?? '')).toEqual({ power: 'on' });
    expect(plain.transport.sent.at(-1)).toMatchObject({ method: 'runtime.status', params: {} });

    const binary = createContext(() => ({ result: { format: 'rgba', byteLength: 4 } }));
    const patched = withBinaryData(binary.ctx, new Uint8Array([1, 2, 3, 4]));
    expect(await runCli(['call', 'output.screenshot', 'format=rgba', '--out', 'x.rgba'], patched)).toBe(0);
    expect(binary.files.get('x.rgba')).toEqual(new Uint8Array([1, 2, 3, 4]));
    expect(binary.err[0]).toContain('4 字节');
    expect(binary.out[0]).toContain('<二进制 4 字节>');
  });

  it('call 没有请求名返回 2；服务端的错误返回 1 并显示错误码', async () => {
    expect(await runCli(['call'], createContext().ctx)).toBe(2);
    const failing = createContext(() => ({ error: { code: -32000, message: '主机没有通电', data: { code: 'runtime.notPowered' } } }));
    expect(await runCli(['call', 'runtime.pause'], failing.ctx)).toBe(1);
    expect(failing.err[0]).toBe('错误 runtime.notPowered：主机没有通电');
  });

  it('连接失败返回 1 并显示原因', async () => {
    const { ctx, err } = createContext(undefined, {
      connect: async () => {
        throw new EmberConnectionError('endpoint', '找不到端点文件');
      }
    });
    expect(await runCli(['call', 'runtime.status'], ctx)).toBe(1);
    expect(err).toEqual(['找不到端点文件']);
  });

  it('screenshot 保存 PNG，默认文件名带时间，格式不对返回 2', async () => {
    const { ctx, out, files, transport } = createContext((request) => ({ result: { format: request.params?.format, width: 256, height: 240, byteLength: 3 } }));
    const patched = withBinaryData(ctx, new Uint8Array([7, 8, 9]));
    expect(await runCli(['screenshot', '--compact'], patched)).toBe(0);
    expect([...files.keys()]).toEqual(['ember-screenshot-20261114-080509.png']);
    expect(files.get('ember-screenshot-20261114-080509.png')).toEqual(new Uint8Array([7, 8, 9]));
    expect(transport.sent.at(-1)).toMatchObject({ method: 'output.screenshot', params: { format: 'png' } });
    expect(JSON.parse(out[0] ?? '')).toMatchObject({ file: 'ember-screenshot-20261114-080509.png', width: 256 });
    expect(await runCli(['screenshot', 'a.png', '--format', 'bmp'], patched)).toBe(2);
  });

  it('watch 订阅事件并逐行打印，收到 --count 条后结束', async () => {
    const { ctx, out, transport } = createContext((request) => ({ result: { events: request.params?.events } }));
    const running = runCli(['watch', 'runtime.*', '--count', '2'], ctx);
    await waitUntil(() => transport.sent.some((request) => request.method === 'debug.subscribe'));
    transport.notify('runtime.stateChanged', { power: 'on' });
    transport.notify('runtime.runFinished', { frames: 1 });
    expect(await running).toBe(0);
    expect(transport.sent.find((request) => request.method === 'debug.subscribe')?.params).toEqual({ events: ['runtime.*'] });
    expect(out.map((line) => JSON.parse(line).event)).toEqual(['runtime.stateChanged', 'runtime.runFinished']);
  });

  it('watch 默认订阅全部事件；--count 不是数字返回 2', async () => {
    const { ctx, transport } = createContext((request) => ({ result: { events: request.params?.events } }));
    const running = runCli(['watch', '--count', '1'], ctx);
    await waitUntil(() => transport.sent.some((request) => request.method === 'debug.subscribe'));
    expect(transport.sent.find((request) => request.method === 'debug.subscribe')?.params).toEqual({ events: ['*'] });
    transport.notify('a.b', {});
    expect(await running).toBe(0);
    expect(await runCli(['watch', '--count', 'x'], createContext().ctx)).toBe(2);
  });
});

/** 轮询等条件成立。 */
async function waitUntil(condition: () => boolean): Promise<void> {
  for (let attempt = 0; attempt < 200 && !condition(); attempt += 1) await new Promise((resolve) => setTimeout(resolve, 5));
  expect(condition()).toBe(true);
}

/** 让所有请求的结果都带上给定的二进制 data（真实服务端以二进制帧送达，客户端解码后就是这样）。 */
function withBinaryData(ctx: CliContext, data: Uint8Array): CliContext {
  return {
    ...ctx,
    connect: async (target) => {
      const client = await ctx.connect(target);
      const request = client.request.bind(client);
      client.request = (async (...args: Parameters<typeof request>) => ({ ...(await request(...args)), data })) as typeof client.request;
      return client;
    }
  };
}
