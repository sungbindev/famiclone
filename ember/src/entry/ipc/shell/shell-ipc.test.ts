/*
 * ------------------------------------------------------------------------
 * 名称：shell-ipc.test.ts
 * 说明：外壳 IPC 入口的测试：不可信的发送方一律不处理（读取状态只得到默认值）、窗口动作只接受白名单、布局修改整条校验后才保存、窗口状态的推送。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import type { IpcMain, IpcMainEvent, IpcMainInvokeEvent, WebContents } from 'electron';
import { describe, expect, it, vi } from 'vitest';
import { SHELL_CHANNELS } from '../../../contracts/shell/shell-bridge';
import type { ShellStateService } from '../../../contracts/shell/shell-state-service';
import { WINDOW_ACTIONS } from '../../../contracts/shell/window-control';
import { DEFAULT_SHELL_STATE } from '../../../shared/shell-state/shell-state';
import { forwardWindowState, registerShellIpc } from './shell-ipc';

type Listener = (event: IpcMainEvent | IpcMainInvokeEvent, ...args: unknown[]) => unknown;

/** 假的 ipcMain：记录各通道的处理函数。 */
function setup(trusted = true) {
  const handlers = new Map<string, Listener>();
  const listeners = new Map<string, Listener>();
  const ipc = {
    handle: (channel: string, listener: Listener) => handlers.set(channel, listener),
    on: (channel: string, listener: Listener) => listeners.set(channel, listener)
  } as unknown as IpcMain;
  const savedState = { ...DEFAULT_SHELL_STATE, layout: { bottomPanelHeight: 220 } };
  const state: ShellStateService = { get: vi.fn(() => savedState), updateLayout: vi.fn(), updateWindow: vi.fn(), flush: vi.fn() };
  const window = { perform: vi.fn(), state: vi.fn(() => ({ maximized: true, fullScreen: false })), setPanelExtents: vi.fn(), panelExtents: vi.fn(() => ({ left: 0, right: 0, bottom: 0 })) };
  registerShellIpc({ ipc, isTrustedSender: () => trusted, state, window });
  const event = {} as IpcMainEvent;
  return {
    state,
    window,
    savedState,
    invoke: (channel: string, ...args: unknown[]) => handlers.get(channel)!(event as unknown as IpcMainInvokeEvent, ...args),
    send: (channel: string, ...args: unknown[]) => listeners.get(channel)!(event, ...args)
  };
}

describe('读取', () => {
  it('可信的页面读到保存的状态与窗口状态', () => {
    const { invoke, savedState } = setup();
    expect(invoke(SHELL_CHANNELS.getState)).toBe(savedState);
    expect(invoke(SHELL_CHANNELS.getWindowState)).toEqual({ maximized: true, fullScreen: false });
  });

  it('不可信的发送方只得到默认值', () => {
    const { invoke } = setup(false);
    expect(invoke(SHELL_CHANNELS.getState)).toEqual(DEFAULT_SHELL_STATE);
    expect(invoke(SHELL_CHANNELS.getWindowState)).toEqual({ maximized: false, fullScreen: false });
  });
});

describe('窗口动作', () => {
  it('白名单里的动作都交给窗口控制器', () => {
    const { send, window } = setup();
    for (const action of WINDOW_ACTIONS) send(SHELL_CHANNELS.windowAction, action);
    expect(window.perform.mock.calls.map(([action]) => action)).toEqual([...WINDOW_ACTIONS]);
  });

  it('不在白名单里的动作、不是字符串的参数一律丢弃', () => {
    const { send, window } = setup();
    for (const action of ['destroy', 'MINIMIZE', '', 'minimize ', 'toString', undefined, null, 5, {}, ['close'], { toString: () => 'close' }]) {
      send(SHELL_CHANNELS.windowAction, action);
    }
    expect(window.perform).not.toHaveBeenCalled();
  });

  it('不可信的发送方发来的动作不执行', () => {
    const { send, window } = setup(false);
    send(SHELL_CHANNELS.windowAction, 'close');
    expect(window.perform).not.toHaveBeenCalled();
  });
});

describe('布局修改', () => {
  it('合格的修改交给状态服务', () => {
    const { send, state } = setup();
    send(SHELL_CHANNELS.setLayout, { bottomPanelHeight: 220 });
    expect(state.updateLayout).toHaveBeenCalledWith({ bottomPanelHeight: 220 });
  });

  it('含未知字段、值不合格、不是对象的修改整条丢弃', () => {
    const { send, state } = setup();
    for (const patch of [{ rightBarOpen: true }, { bottomPanelHeight: 200, window: {} }, { bottomPanelHeight: -3 }, {}, undefined, null, 'x', []]) {
      send(SHELL_CHANNELS.setLayout, patch);
    }
    expect(state.updateLayout).not.toHaveBeenCalled();
  });

  it('不可信的发送方发来的修改不保存', () => {
    const { send, state } = setup(false);
    send(SHELL_CHANNELS.setLayout, { bottomPanelHeight: 220 });
    expect(state.updateLayout).not.toHaveBeenCalled();
  });
});

describe('面板占用尺寸', () => {
  it('合格的尺寸交给窗口控制器，请求在窗口调整完成后才返回', async () => {
    const { invoke, window } = setup();
    await invoke(SHELL_CHANNELS.setPanelExtents, { left: 240, right: 0, bottom: 160 });
    expect(window.setPanelExtents).toHaveBeenCalledWith({ left: 240, right: 0, bottom: 160 });
  });

  it('不合格的尺寸、不可信的发送方一律丢弃', async () => {
    const { invoke, window } = setup();
    for (const raw of [undefined, null, 'x', [], {}, { left: -1, right: 0, bottom: 0 }, { left: 1.5, right: 0, bottom: 0 }, { left: 0, right: 0, bottom: 99_999 }, { left: 0, right: 0, bottom: 0, extra: 1 }]) {
      await invoke(SHELL_CHANNELS.setPanelExtents, raw);
    }
    const untrusted = setup(false);
    await untrusted.invoke(SHELL_CHANNELS.setPanelExtents, { left: 1, right: 0, bottom: 0 });
    expect(window.setPanelExtents).not.toHaveBeenCalled();
    expect(untrusted.window.setPanelExtents).not.toHaveBeenCalled();
  });
});

describe('forwardWindowState', () => {
  it('把窗口当前的状态推送给页面', () => {
    const send = vi.fn();
    const contents = { isDestroyed: () => false, send } as unknown as WebContents;
    forwardWindowState(contents, () => ({ maximized: false, fullScreen: true }))();
    expect(send).toHaveBeenCalledWith(SHELL_CHANNELS.windowStateChanged, { maximized: false, fullScreen: true });
  });

  it('页面已销毁时不推送', () => {
    const send = vi.fn();
    const contents = { isDestroyed: () => true, send } as unknown as WebContents;
    forwardWindowState(contents, () => ({ maximized: false, fullScreen: false }))();
    expect(send).not.toHaveBeenCalled();
  });
});
