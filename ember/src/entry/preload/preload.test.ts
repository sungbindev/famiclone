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
    expect(Object.keys(bridge).toSorted()).toEqual(['core', 'getCoreStatus', 'main', 'onCoreEvent', 'onCoreStatusChange', 'shell', 'streams']);
    expect(JSON.stringify(Object.keys(bridge.streams))).toBe('["requestPorts"]');
    expect(Object.keys(bridge.shell).toSorted()).toEqual(['getState', 'getWindowState', 'onKeyboardReleased', 'onNativeKey', 'onWindowStateChange', 'setKeyboardCapture', 'setLayout', 'windowAction']);
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
    expect(Object.keys(bridge.main)).toEqual(['cartridges', 'machines', 'mappers', 'media', 'preferences', 'system']);
    expect(Object.keys(bridge.main.cartridges).toSorted()).toEqual(['open', 'openRecent', 'recent']);
    expect(Object.keys(bridge.main.machines)).toEqual(['openBios']);
    expect(Object.keys(bridge.main.mappers)).toEqual(['openPackage']);
    expect(Object.keys(bridge.main.media)).toEqual(['openFloppy']);
    expect(Object.keys(bridge.main.preferences)).toEqual(['pickDirectory']);
    expect(Object.keys(bridge.main.system)).toEqual(['openLogDirectory']);
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

  it('外壳能力走各自的通道：读取状态与窗口状态用 invoke，保存布局与窗口动作用 send', async () => {
    electron.invoke.mockResolvedValue('状态');
    await expect(bridge.shell.getState()).resolves.toBe('状态');
    await bridge.shell.getWindowState();
    bridge.shell.setLayout({ bottomPanelHeight: 200 });
    bridge.shell.windowAction('toggleFullScreen');
    expect(electron.invoke.mock.calls).toEqual([[SHELL_CHANNELS.getState], [SHELL_CHANNELS.getWindowState]]);
    expect(electron.send.mock.calls).toEqual([
      [SHELL_CHANNELS.setLayout, { bottomPanelHeight: 200 }],
      [SHELL_CHANNELS.windowAction, 'toggleFullScreen']
    ]);
  });

  it('窗口状态的变化只把数据交给回调，不交出 IPC 事件对象', () => {
    const received: unknown[] = [];
    bridge.shell.onWindowStateChange((state) => received.push(state));
    const listener = electron.listeners.get(SHELL_CHANNELS.windowStateChanged);
    expect(listener).toBeDefined();
    listener?.({ sender: '事件对象' }, { maximized: true, fullScreen: false });
    expect(received).toEqual([{ maximized: true, fullScreen: false }]);
  });

  it('键盘独占的开关用 invoke，独占期间的按键与“已关闭”只把数据交给回调，不交出 IPC 事件对象', async () => {
    electron.invoke.mockResolvedValue(true);
    await expect(bridge.shell.setKeyboardCapture(true)).resolves.toBe(true);
    expect(electron.invoke).toHaveBeenLastCalledWith(SHELL_CHANNELS.setKeyboardCapture, true);
    const keys: unknown[] = [];
    let released = 0;
    bridge.shell.onNativeKey((event) => keys.push(event));
    bridge.shell.onKeyboardReleased(() => (released += 1));
    electron.listeners.get(SHELL_CHANNELS.nativeKey)?.({ sender: '事件对象' }, { code: 'Escape', down: true });
    electron.listeners.get(SHELL_CHANNELS.keyboardReleased)?.({ sender: '事件对象' });
    expect(keys).toEqual([{ code: 'Escape', down: true }]);
    expect(released).toBe(1);
  });

  it('核心运行状态的变化只把数据交给回调，不交出 IPC 事件对象', () => {
    const received: unknown[] = [];
    bridge.onCoreStatusChange((status) => received.push(status));
    const listener = electron.listeners.get(SHELL_CHANNELS.coreStatusChanged);
    expect(listener).toBeDefined();
    listener?.({ sender: '事件对象' }, { state: 'ready', coreVersion: '0.1.0' });
    expect(received).toEqual([{ state: 'ready', coreVersion: '0.1.0' }]);
  });

  it('最近打开的请求走主进程请求通道', async () => {
    electron.invoke.mockResolvedValue({ ok: true, result: { items: [] } });
    await expect(bridge.main.cartridges.recent({})).resolves.toEqual({ items: [] });
    expect(electron.invoke).toHaveBeenCalledWith(SHELL_CHANNELS.mainRequest, 'cartridges.recent', {});
    await bridge.main.cartridges.openRecent({ id: 3 });
    expect(electron.invoke).toHaveBeenLastCalledWith(SHELL_CHANNELS.mainRequest, 'cartridges.openRecent', { id: 3 });
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
