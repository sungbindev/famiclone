/*
 * ------------------------------------------------------------------------
 * 名称：rpc-session.test.ts
 * 说明：JSON-RPC 会话的测试：按编号匹配响应、错误响应的映射、通知转发、二进制结果、超时、连接关闭时等待中的请求失败。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import { afterEach, describe, expect, it, vi } from 'vitest';

import { RpcSession } from '../src/logic/rpc-session.js';
import { EmberConnectionError, EmberRequestError } from '../src/shared/errors.js';
import { binaryFrame, FakeTransport } from './support/fake-transport.js';

afterEach(() => {
  vi.useRealTimers();
});

describe('RpcSession', () => {
  it('发出 JSON-RPC 请求，按编号匹配响应，请求之间互不串', async () => {
    const transport = new FakeTransport((request) => ({ result: { echo: request.method } }));
    const session = new RpcSession(transport);
    const [first, second] = await Promise.all([session.request('runtime.status'), session.request('system.version', { a: 1 })]);
    expect(first).toEqual({ echo: 'runtime.status' });
    expect(second).toEqual({ echo: 'system.version' });
    expect(transport.sent.map((request) => request.id)).toEqual([1, 2]);
    expect(transport.sent[0]).toEqual({ jsonrpc: '2.0', id: 1, method: 'runtime.status' });
    expect(transport.sent[1]?.params).toEqual({ a: 1 });
  });

  it('错误响应变成 EmberRequestError，带 JSON-RPC 码与模块错误码', async () => {
    const transport = new FakeTransport(() => ({ error: { code: -32000, message: '没有卡带', data: { code: 'runtime.noCartridge' } } }));
    const error = await new RpcSession(transport).request('runtime.powerOn').catch((reason: unknown) => reason);
    expect(error).toBeInstanceOf(EmberRequestError);
    expect(error).toMatchObject({ rpcCode: -32000, code: 'runtime.noCartridge', message: '没有卡带' });
  });

  it('错误响应没有模块错误码时用 core.unknown', async () => {
    const transport = new FakeTransport(() => ({ error: { code: -32700, message: '不是 JSON' } }));
    await expect(new RpcSession(transport).request('x.y')).rejects.toMatchObject({ code: 'core.unknown', rpcCode: -32700 });
  });

  it('把通知转给处理函数，取消注册后不再收到', () => {
    const transport = new FakeTransport();
    const session = new RpcSession(transport);
    const received: unknown[] = [];
    const off = session.onNotification((method, params) => received.push([method, params]));
    transport.notify('runtime.stateChanged', { power: 'on' });
    off();
    transport.notify('runtime.stateChanged', { power: 'off' });
    expect(received).toEqual([['runtime.stateChanged', { power: 'on' }]]);
  });

  it('二进制帧的响应带 Uint8Array 的 data', async () => {
    const transport = new FakeTransport();
    const session = new RpcSession(transport);
    const pending = session.request('output.screenshot');
    transport.receive(binaryFrame({ jsonrpc: '2.0', id: 1, result: { format: 'rgba' } }, new Uint8Array([5, 6])));
    await expect(pending).resolves.toEqual({ format: 'rgba', data: new Uint8Array([5, 6]) });
  });

  it('等待超时后以 timeout 失败；迟到的响应被忽略', async () => {
    vi.useFakeTimers();
    const transport = new FakeTransport();
    const session = new RpcSession(transport, 1000);
    const pending = session.request('runtime.run', { frames: 1 });
    const assertion = expect(pending).rejects.toMatchObject({ kind: 'timeout' });
    await vi.advanceTimersByTimeAsync(1000);
    await assertion;
    expect(() => transport.receive(JSON.stringify({ jsonrpc: '2.0', id: 1, result: {} }))).not.toThrow();
  });

  it('timeoutMs 为 0 时一直等', async () => {
    vi.useFakeTimers();
    const transport = new FakeTransport();
    const session = new RpcSession(transport, 10);
    const pending = session.request('runtime.run', {}, { timeoutMs: 0 });
    await vi.advanceTimersByTimeAsync(60_000);
    transport.receive(JSON.stringify({ jsonrpc: '2.0', id: 1, result: { ok: true } }));
    await expect(pending).resolves.toEqual({ ok: true });
  });

  it('连接关闭时等待中的请求以 closed 失败，之后的请求立即失败，closed 兑现关闭信息', async () => {
    const transport = new FakeTransport();
    const session = new RpcSession(transport);
    const pending = session.request('runtime.status');
    transport.closeFromServer(4002, '令牌已更换');
    await expect(pending).rejects.toMatchObject({ kind: 'closed', closeCode: 4002 });
    await expect(session.request('runtime.status')).rejects.toBeInstanceOf(EmberConnectionError);
    await expect(session.closed).resolves.toEqual({ code: 4002, reason: '令牌已更换' });
  });

  it('收到不合法的报文时让等待中的请求以 protocol 失败，并以 1002 关闭连接', async () => {
    const transport = new FakeTransport();
    const session = new RpcSession(transport);
    const pending = session.request('runtime.status');
    transport.receive('不是 JSON');
    await expect(pending).rejects.toMatchObject({ kind: 'protocol' });
    expect(transport.closeArguments?.code).toBe(1002);
  });
});
