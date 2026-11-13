/*
 * ------------------------------------------------------------------------
 * 名称：electron-stream-ports.test.ts
 * 说明：消息端口 Electron 实现的测试：通道工厂每次创建新的 MessageChannelMain，窗口接收方把端口经 webContents.postMessage 交给页面、销毁时通知。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：用假的 electron 模块；真实的端口传递在 Electron 冒烟脚本里验证。
 * ------------------------------------------------------------------------
 */

import { describe, expect, it, vi } from 'vitest';

const electron = vi.hoisted(() => ({ channels: 0 }));

vi.mock('electron', () => ({
  MessageChannelMain: class {
    readonly port1 = { name: `port1-${++electron.channels}` };
    readonly port2 = { name: `port2-${electron.channels}` };
  }
}));

const { createElectronChannelFactory, createWebContentsStreamTarget } = await import('./electron-stream-ports');

describe('createElectronChannelFactory', () => {
  it('每次创建一条新的通道，两端不同', () => {
    const factory = createElectronChannelFactory();
    const first = factory.create();
    const second = factory.create();
    expect(first.first).not.toBe(first.second);
    expect(first.first).not.toBe(second.first);
  });
});

/** 假的页面：记录 postMessage，可以模拟销毁。 */
function createContents() {
  const destroyed: Array<() => void> = [];
  const state = { destroyed: false };
  const contents = {
    id: 3,
    isDestroyed: () => state.destroyed,
    postMessage: vi.fn(),
    once: vi.fn((event: string, listener: () => void) => {
      if (event === 'destroyed') destroyed.push(listener);
    })
  };
  return { contents, state, destroyed };
}

describe('createWebContentsStreamTarget', () => {
  it('把两个端口连同通道名与标记交给页面', () => {
    const { contents } = createContents();
    const target = createWebContentsStreamTarget(contents as never, 'streams:ports', 'ember.stream-ports');
    const frames = { name: 'frames' };
    const input = { name: 'input' };
    target.send({ frames, input });
    expect(contents.postMessage).toHaveBeenCalledWith('streams:ports', 'ember.stream-ports', [frames, input]);
    expect(target.id).toBe(3);
  });

  it('窗口销毁后 isAlive 为 false，销毁时通知监听者', () => {
    const { contents, state, destroyed } = createContents();
    const target = createWebContentsStreamTarget(contents as never, 'streams:ports', 'tag');
    const listener = vi.fn();
    target.onDestroyed(listener);
    expect(target.isAlive()).toBe(true);
    state.destroyed = true;
    destroyed.forEach((callback) => callback());
    expect(target.isAlive()).toBe(false);
    expect(listener).toHaveBeenCalledTimes(1);
  });
});
