/*
 * ------------------------------------------------------------------------
 * 名称：preload.test.ts
 * 说明：预加载脚本的测试：暴露的接口只有具名方法（没有 ipcRenderer）、mainOnly 请求不暴露给界面、主进程请求与端口请求走各自的通道、收到端口时校验标记与数量后才交给页面。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：用假的 electron 模块捕获 exposeInMainWorld 与 ipcRenderer 的调用；jsdom 提供 window。
 * ------------------------------------------------------------------------
 */

import { afterEach, beforeAll, describe, expect, it, vi } from 'vitest';
import { SHELL_CHANNELS, type EmberBridge } from '../../contracts/shell/shell-bridge';
import { STREAM_PORTS_WINDOW_MESSAGE } from '../../contracts/stream-ports/stream-messages';
import { STREAM_CHANNELS } from '../../contracts/stream-ports/stream-ports';

const electron = vi.hoisted(() => ({
  exposed: new Map<string, unknown>(),
  listeners: new Map<string, (event: unknown, ...args: unknown[]) => void>(),
  invoke: vi.fn(),
  send: vi.fn()
}));

vi.mock('electron', () => ({
  contextBridge: { exposeInMainWorld: (name: string, value: unknown) => electron.exposed.set(name, value) },
  ipcRenderer: {
    on: (channel: string, listener: (event: unknown, ...args: unknown[]) => void) => electron.listeners.set(channel, listener),
    removeListener: vi.fn(),
    invoke: electron.invoke,
    send: electron.send
  }
}));

let bridge: EmberBridge;

beforeAll(async () => {
  await import('./preload');
  bridge = electron.exposed.get('ember') as EmberBridge;
});

afterEach(() => {
  vi.restoreAllMocks();
  electron.invoke.mockReset();
  electron.send.mockReset();
});

describe('window.ember', () => {
  it('只暴露具名方法，不暴露 ipcRenderer 或通用的调用入口', () => {
    expect(Object.keys(bridge).toSorted()).toEqual(['core', 'getCoreStatus', 'main', 'onCoreEvent', 'streams']);
    expect(JSON.stringify(Object.keys(bridge.streams))).toBe('["requestPorts"]');
    expect('ipcRenderer' in bridge).toBe(false);
    expect('invoke' in bridge).toBe(false);
  });

  it('界面能调用的核心请求按 Schema 生成，只能由主进程发起的 cartridges.load 不暴露', () => {
    expect(typeof bridge.core.runtime.status).toBe('function');
    expect(typeof bridge.core.runtime.pause).toBe('function');
    expect(typeof bridge.core.cartridges.eject).toBe('function');
    expect(typeof bridge.core.cartridges.info).toBe('function');
    expect((bridge.core.cartridges as Record<string, unknown>).load).toBeUndefined();
    expect((bridge.core.mappers as Record<string, unknown>).install).toBeUndefined();
  });

  it('主进程请求暴露为 main.cartridges.open，走主进程请求通道', async () => {
    expect(Object.keys(bridge.main)).toEqual(['cartridges']);
    expect(Object.keys(bridge.main.cartridges)).toEqual(['open']);
    electron.invoke.mockResolvedValue({ ok: true, result: { canceled: true } });
    await expect(bridge.main.cartridges.open({})).resolves.toEqual({ canceled: true });
    expect(electron.invoke).toHaveBeenCalledWith(SHELL_CHANNELS.mainRequest, 'cartridges.open', {});
  });

  it('主进程请求失败时以 {code, message} 拒绝', async () => {
    electron.invoke.mockResolvedValue({ ok: false, error: { code: 'cartridges.notInes', message: '不是 NES 文件' } });
    await expect(bridge.main.cartridges.open({})).rejects.toEqual({ code: 'cartridges.notInes', message: '不是 NES 文件' });
  });

  it('核心请求走核心请求通道', async () => {
    electron.invoke.mockResolvedValue({ ok: true, result: { power: 'off' } });
    await bridge.core.runtime.status({});
    expect(electron.invoke).toHaveBeenCalledWith(SHELL_CHANNELS.coreRequest, 'runtime.status', {});
  });

  it('streams.requestPorts 向主进程请求端口', () => {
    bridge.streams.requestPorts();
    expect(electron.send).toHaveBeenCalledWith(STREAM_CHANNELS.connect);
  });
});

/** 模拟主进程把端口发给预加载脚本，返回对 window.postMessage 的监视。 */
function deliver(tag: unknown, ports: unknown[]): ReturnType<typeof vi.spyOn> {
  const post = vi.spyOn(window, 'postMessage').mockImplementation(() => undefined);
  const listener = electron.listeners.get(STREAM_CHANNELS.ports);
  expect(listener).toBeDefined();
  listener?.({ ports }, tag);
  return post;
}

describe('收到端口', () => {
  it('标记与端口数量都对时用窗口消息把端口交给页面', () => {
    const frames = { name: 'frames' };
    const input = { name: 'input' };
    const post = deliver(STREAM_PORTS_WINDOW_MESSAGE, [frames, input]);
    expect(post).toHaveBeenCalledTimes(1);
    const [message, origin, transfer] = post.mock.calls[0] as [unknown, string, unknown[]];
    expect(message).toEqual({ type: STREAM_PORTS_WINDOW_MESSAGE });
    expect(origin).toBe(window.location.origin);
    expect(transfer).toEqual([frames, input]);
  });

  it('标记不对时丢弃', () => {
    expect(deliver('别的消息', [{}, {}])).not.toHaveBeenCalled();
    expect(deliver(undefined, [{}, {}])).not.toHaveBeenCalled();
  });

  it('端口数量不对时丢弃', () => {
    expect(deliver(STREAM_PORTS_WINDOW_MESSAGE, [])).not.toHaveBeenCalled();
    expect(deliver(STREAM_PORTS_WINDOW_MESSAGE, [{}])).not.toHaveBeenCalled();
    expect(deliver(STREAM_PORTS_WINDOW_MESSAGE, [{}, {}, {}])).not.toHaveBeenCalled();
  });
});
