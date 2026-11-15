/*
 * ------------------------------------------------------------------------
 * 名称：shell-ipc.test.ts
 * 说明：外壳 IPC 入口的测试：不可信的发送方一律不处理（读取状态只得到默认值）、窗口动作只接受白名单、布局修改整条校验后才保存、键盘独占只接受可信页面的布尔值、窗口状态与独占期间按键的推送。
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
import type { KeyboardCaptureService, NativeKeyEvent } from '../../../contracts/shell/keyboard-capture';
import { forwardKeyboardToWindow, forwardWindowState, registerShellIpc } from './shell-ipc';

type Listener = (event: IpcMainEvent | IpcMainInvokeEvent, ...args: unknown[]) => unknown;

/** 假的 ipcMain：记录各通道的处理函数。 */
function setup(trusted = true) {
  const handlers = new Map<string, Listener>();
  const listeners = new Map<string, Listener>();
  const ipc = {
    handle: (channel: string, listener: Listener) => handlers.set(channel, listener),
    on: (channel: string, listener: Listener) => listeners.set(channel, listener)
  } as unknown as IpcMain;
  const savedState = { ...DEFAULT_SHELL_STATE, layout: { bottomPanelHeight: 220, selectedMachine: null } };
  const state: ShellStateService = { get: vi.fn(() => savedState), updateLayout: vi.fn(), updateWindow: vi.fn(), flush: vi.fn() };
  const window = { perform: vi.fn(), state: vi.fn(() => ({ maximized: true, fullScreen: false })) };
  const keyboard = { setCapture: vi.fn((enabled: boolean) => enabled), onKey: vi.fn(), onReleased: vi.fn() } satisfies KeyboardCaptureService;
  registerShellIpc({ ipc, isTrustedSender: () => trusted, state, window, keyboard });
  const event = {} as IpcMainEvent;
  return {
    state,
    window,
    keyboard,
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

describe('键盘独占', () => {
  it('可信的页面要求开启或关闭，结果是服务的返回值', () => {
    const { invoke, keyboard } = setup();
    expect(invoke(SHELL_CHANNELS.setKeyboardCapture, true)).toBe(true);
    expect(invoke(SHELL_CHANNELS.setKeyboardCapture, false)).toBe(false);
    expect(keyboard.setCapture.mock.calls).toEqual([[true], [false]]);
  });

  it('参数不是 true 的一律按关闭处理', () => {
    const { invoke, keyboard } = setup();
    for (const value of ['true', 1, {}, null, undefined]) invoke(SHELL_CHANNELS.setKeyboardCapture, value);
    expect(keyboard.setCapture.mock.calls.every(([enabled]) => enabled === false)).toBe(true);
  });

  it('不可信的发送方不能开启独占', () => {
    const { invoke, keyboard } = setup(false);
    expect(invoke(SHELL_CHANNELS.setKeyboardCapture, true)).toBe(false);
    expect(keyboard.setCapture).not.toHaveBeenCalled();
  });
});

/** 可以手动触发的键盘独占服务。 */
function fakeKeyboard() {
  let onKey: ((event: NativeKeyEvent) => void) | undefined;
  let onReleased: (() => void) | undefined;
  const stopKey = vi.fn();
  const stopReleased = vi.fn();
  const service: KeyboardCaptureService = {
    setCapture: () => true,
    onKey: (listener) => {
      onKey = listener;
      return stopKey;
    },
    onReleased: (listener) => {
      onReleased = listener;
      return stopReleased;
    }
  };
  return { service, key: (event: NativeKeyEvent) => onKey?.(event), released: () => onReleased?.(), stopKey, stopReleased };
}

describe('forwardKeyboardToWindow', () => {
  it('把独占期间的按键与“已关闭”推送给页面，取消转发时两个订阅都取消', () => {
    const send = vi.fn();
    const contents = { isDestroyed: () => false, send } as unknown as WebContents;
    const keyboard = fakeKeyboard();
    const stop = forwardKeyboardToWindow(keyboard.service, contents);
    keyboard.key({ code: 'Escape', down: true });
    keyboard.released();
    expect(send.mock.calls).toEqual([[SHELL_CHANNELS.nativeKey, { code: 'Escape', down: true }], [SHELL_CHANNELS.keyboardReleased]]);
    stop();
    expect(keyboard.stopKey).toHaveBeenCalledTimes(1);
    expect(keyboard.stopReleased).toHaveBeenCalledTimes(1);
  });

  it('页面已销毁时不推送', () => {
    const send = vi.fn();
    const contents = { isDestroyed: () => true, send } as unknown as WebContents;
    const keyboard = fakeKeyboard();
    forwardKeyboardToWindow(keyboard.service, contents);
    keyboard.key({ code: 'KeyA', down: true });
    keyboard.released();
    expect(send).not.toHaveBeenCalled();
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
