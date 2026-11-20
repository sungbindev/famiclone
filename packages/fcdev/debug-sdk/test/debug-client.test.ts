/*
 * ------------------------------------------------------------------------
 * 名称：debug-client.test.ts
 * 说明：客户端的测试：握手（令牌、客户端名、协议版本）、类型化请求与按模块分组的接口、订阅、事件处理与等待、关闭。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import { afterEach, describe, expect, it, vi } from 'vitest';

import { connect } from '../src/entry/connect.js';
import { EmberDebugClient } from '../src/logic/debug-client.js';
import { EmberConnectionError, EmberRequestError } from '../src/shared/errors.js';
import { FakeTransport, HELLO_RESULT, type Responder } from './support/fake-transport.js';

afterEach(() => {
  vi.useRealTimers();
});

/** 连不上的传输层。 */
async function unreachable(): Promise<never> {
  throw new EmberConnectionError('unavailable', '连不上');
}

/** 握手之外的请求用 extra 应答。 */
function openClient(extra: Responder = () => undefined, token = 'secret') {
  const transport = new FakeTransport((request) => {
    if (request.method === 'debug.hello') return request.params?.token === 'secret' ? { result: HELLO_RESULT } : { error: { code: -32000, message: '令牌不对', data: { code: 'debug.unauthorized' } } };
    return extra(request);
  });
  const client = EmberDebugClient.open({ url: 'ws://127.0.0.1:5688/', token, clientName: '测试', transportFactory: async () => transport });
  return { transport, client };
}

describe('EmberDebugClient 握手', () => {
  it('握手请求带令牌、客户端名与协议版本，结果放在 info', async () => {
    const { transport, client } = openClient();
    const opened = await client;
    expect(transport.sent[0]).toMatchObject({ method: 'debug.hello', params: { client: '测试', token: 'secret', protocol: 1 } });
    expect(opened.info).toEqual(HELLO_RESULT);
  });

  it('令牌不对时以 EmberRequestError(debug.unauthorized) 失败，并关闭连接', async () => {
    const { transport, client } = openClient(undefined, 'wrong');
    await expect(client).rejects.toMatchObject({ code: 'debug.unauthorized' });
    expect(transport.closeArguments).toBeDefined();
  });

  it('握手没有应答时按连接超时失败', async () => {
    vi.useFakeTimers();
    const transport = new FakeTransport();
    const opened = EmberDebugClient.open({ url: 'ws://x/', token: 't', connectTimeoutMs: 500, transportFactory: async () => transport });
    const assertion = expect(opened).rejects.toMatchObject({ kind: 'timeout' });
    await vi.advanceTimersByTimeAsync(500);
    await assertion;
  });

  it('连不上时把传输层的错误原样抛出', async () => {
    await expect(connect({ url: 'ws://x/', token: 't', transportFactory: unreachable })).rejects.toMatchObject({ kind: 'unavailable' });
  });

  it('Ember 的协议版本与 SDK 不一致时以 protocol 错误失败，提示升级 fcdev，并关闭连接', async () => {
    const transport = new FakeTransport((request) => (request.method === 'debug.hello' ? { result: { ...HELLO_RESULT, protocol: 2, appVersion: '9.0.0' } } : undefined));
    const opened = EmberDebugClient.open({ url: 'ws://x/', token: 't', transportFactory: async () => transport });
    await expect(opened).rejects.toMatchObject({ kind: 'protocol', message: expect.stringContaining('升级 fcdev') });
    expect(transport.closeArguments).toBeDefined();
  });
});

describe('EmberDebugClient 请求', () => {
  it('request 按方法名发请求，返回结果', async () => {
    const { transport, client } = openClient((request) => ({ result: { power: 'on', mode: 'running', echo: request.method } }));
    const opened = await client;
    const status = await opened.request('runtime.status');
    expect(status).toMatchObject({ power: 'on' });
    expect(transport.sent.at(-1)).toMatchObject({ method: 'runtime.status' });
  });

  it('api 按模块分组：api.runtime.run(...) 等同于 request("runtime.run", ...)', async () => {
    const { transport, client } = openClient(() => ({ result: { frames: 3, cycles: 9, stopped: 'completed', state: { power: 'on', mode: 'paused' } } }));
    const opened = await client;
    const result = await opened.api.runtime.run({ frames: 3, wait: true });
    expect(result.stopped).toBe('completed');
    expect(transport.sent.at(-1)).toMatchObject({ method: 'runtime.run', params: { frames: 3, wait: true } });
  });

  it('api 的 then 属性是 undefined，所以可以被 await 而不被当成 Promise', async () => {
    const { client } = openClient();
    const opened = await client;
    expect((opened.api as unknown as { then?: unknown }).then).toBeUndefined();
    expect((opened.api.runtime as unknown as { then?: unknown }).then).toBeUndefined();
  });

  it('服务端的错误响应变成 EmberRequestError', async () => {
    const { client } = openClient(() => ({ error: { code: -32000, message: '主机没有通电', data: { code: 'runtime.notPowered' } } }));
    const opened = await client;
    await expect(opened.api.runtime.pause()).rejects.toBeInstanceOf(EmberRequestError);
  });
});

describe('EmberDebugClient 事件', () => {
  it('subscribe 与 unsubscribe 返回订阅后的全部写法', async () => {
    const { transport, client } = openClient((request) => ({ result: { events: request.method === 'debug.subscribe' ? ['runtime.*'] : [] } }));
    const opened = await client;
    expect(await opened.subscribe(['runtime.*'])).toEqual(['runtime.*']);
    expect(transport.sent.at(-1)).toMatchObject({ method: 'debug.subscribe', params: { events: ['runtime.*'] } });
    expect(await opened.unsubscribe(['runtime.*'])).toEqual([]);
  });

  it('on 只收指定事件，onAny 收全部，取消注册后不再收到', async () => {
    const { transport, client } = openClient();
    const opened = await client;
    const finished: unknown[] = [];
    const all: string[] = [];
    const off = opened.on('runtime.runFinished', (data) => finished.push(data));
    opened.onAny((event) => all.push(event));
    transport.notify('runtime.runFinished', { frames: 5, cycles: 1, stopped: 'completed' });
    transport.notify('runtime.stateChanged', { power: 'on', mode: 'paused', reason: 'pause' });
    off();
    transport.notify('runtime.runFinished', { frames: 6, cycles: 1, stopped: 'completed' });
    expect(finished).toEqual([{ frames: 5, cycles: 1, stopped: 'completed' }]);
    expect(all).toEqual(['runtime.runFinished', 'runtime.stateChanged', 'runtime.runFinished']);
  });

  it('waitFor 等到满足条件的事件', async () => {
    const { transport, client } = openClient();
    const opened = await client;
    const waiting = opened.waitFor('runtime.runFinished', { predicate: (data) => data.frames === 2 });
    transport.notify('runtime.runFinished', { frames: 1, cycles: 1, stopped: 'completed' });
    transport.notify('runtime.runFinished', { frames: 2, cycles: 2, stopped: 'completed' });
    await expect(waiting).resolves.toMatchObject({ frames: 2 });
  });

  it('waitFor 超时或连接关闭时失败', async () => {
    vi.useFakeTimers();
    const { transport, client } = openClient();
    const opened = await client;
    const timedOut = expect(opened.waitFor('runtime.runFinished', { timeoutMs: 100 })).rejects.toMatchObject({ kind: 'timeout' });
    await vi.advanceTimersByTimeAsync(100);
    await timedOut;
    const closed = expect(opened.waitFor('runtime.runFinished', { timeoutMs: 0 })).rejects.toMatchObject({ kind: 'closed' });
    transport.closeFromServer(1001);
    await closed;
  });

  it('close 关闭连接，closed 兑现', async () => {
    const { client } = openClient();
    const opened = await client;
    opened.close();
    await expect(opened.closed).resolves.toMatchObject({ code: 1000 });
  });
});
