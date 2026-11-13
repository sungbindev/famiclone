/*
 * ------------------------------------------------------------------------
 * 名称：streams-ipc.test.ts
 * 说明：端口请求入口的测试：只有应用自己的页面、主窗口、主框架的请求才建立端口，其余一律忽略。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import type { IpcMain, IpcMainEvent, WebContents } from 'electron';
import { describe, expect, it, vi } from 'vitest';
import { STREAM_CHANNELS, type StreamTarget } from '../../../contracts/stream-ports/stream-ports';
import { handleStreamsConnect, registerStreamsIpc } from './streams-ipc';

/** 假的页面：主框架是 frame。 */
function createContents(destroyed = false) {
  const mainFrame = { name: 'main' };
  const contents = { isDestroyed: () => destroyed, mainFrame } as unknown as WebContents;
  return { contents, mainFrame };
}

function setup(overrides: { trusted?: boolean; streamWindow?: boolean } = {}) {
  const connect = vi.fn();
  const target = { id: 7 } as StreamTarget;
  const createTarget = vi.fn().mockReturnValue(target);
  const options = {
    connector: { connect },
    isTrustedSender: () => overrides.trusted ?? true,
    isStreamWindow: () => overrides.streamWindow ?? true,
    createTarget
  };
  return { options, connect, createTarget, target };
}

describe('handleStreamsConnect', () => {
  it('可信的主窗口主框架请求时建立端口', () => {
    const { options, connect, createTarget, target } = setup();
    const { contents, mainFrame } = createContents();
    handleStreamsConnect(options, { sender: contents, senderFrame: mainFrame } as unknown as IpcMainEvent);
    expect(createTarget).toHaveBeenCalledWith(contents);
    expect(connect).toHaveBeenCalledWith(target);
  });

  it('不可信的发送方被忽略', () => {
    const { options, connect } = setup({ trusted: false });
    const { contents, mainFrame } = createContents();
    handleStreamsConnect(options, { sender: contents, senderFrame: mainFrame } as unknown as IpcMainEvent);
    expect(connect).not.toHaveBeenCalled();
  });

  it('不是主窗口的页面被忽略', () => {
    const { options, connect } = setup({ streamWindow: false });
    const { contents, mainFrame } = createContents();
    handleStreamsConnect(options, { sender: contents, senderFrame: mainFrame } as unknown as IpcMainEvent);
    expect(connect).not.toHaveBeenCalled();
  });

  it('子框架的请求被忽略，只有主框架能收到端口', () => {
    const { options, connect } = setup();
    const { contents } = createContents();
    handleStreamsConnect(options, { sender: contents, senderFrame: { name: 'child' } } as unknown as IpcMainEvent);
    expect(connect).not.toHaveBeenCalled();
  });

  it('页面已销毁时被忽略', () => {
    const { options, connect } = setup();
    const { contents, mainFrame } = createContents(true);
    handleStreamsConnect(options, { sender: contents, senderFrame: mainFrame } as unknown as IpcMainEvent);
    expect(connect).not.toHaveBeenCalled();
  });
});

describe('registerStreamsIpc', () => {
  it('在端口请求通道上登记', () => {
    const on = vi.fn();
    const { options, connect } = setup();
    registerStreamsIpc({ ...options, ipc: { on } as unknown as IpcMain });
    const [channel, listener] = on.mock.calls[0] as [string, (event: IpcMainEvent) => void];
    expect(channel).toBe(STREAM_CHANNELS.connect);
    const { contents, mainFrame } = createContents();
    listener({ sender: contents, senderFrame: mainFrame } as unknown as IpcMainEvent);
    expect(connect).toHaveBeenCalledTimes(1);
  });
});
