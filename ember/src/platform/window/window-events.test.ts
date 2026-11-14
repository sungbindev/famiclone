/*
 * ------------------------------------------------------------------------
 * 名称：window-events.test.ts
 * 说明：窗口事件监听的测试：移动、改变大小、最大化、还原与关闭时报告位置（含是否最大化），全屏时不报告位置，窗口已销毁时不报告，状态变化事件通知接收方。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

// @vitest-environment node

import { EventEmitter } from 'node:events';
import { describe, expect, it, vi } from 'vitest';
import { watchWindow, type WatchedWindow } from './window-events';

/** 假窗口：用事件发射器触发事件，状态可调。 */
function createWindow(initial: { fullScreen?: boolean; maximized?: boolean; destroyed?: boolean } = {}) {
  const emitter = new EventEmitter();
  const state = { fullScreen: initial.fullScreen ?? false, maximized: initial.maximized ?? false, destroyed: initial.destroyed ?? false };
  const window: WatchedWindow = {
    on: (event, listener) => emitter.on(event, listener),
    isDestroyed: () => state.destroyed,
    isFullScreen: () => state.fullScreen,
    isMaximized: () => state.maximized,
    getNormalBounds: () => ({ x: 10, y: 20, width: 1100, height: 700 })
  };
  return { window, emit: (event: string) => emitter.emit(event), state };
}

describe('watchWindow', () => {
  it('移动、改变大小、最大化、还原与关闭时报告位置与是否最大化', () => {
    const { window, emit, state } = createWindow();
    const onPlacement = vi.fn();
    watchWindow(window, { onPlacement, onState: vi.fn() });
    for (const event of ['move', 'resize', 'unmaximize', 'close']) emit(event);
    expect(onPlacement).toHaveBeenCalledTimes(4);
    expect(onPlacement).toHaveBeenLastCalledWith({ x: 10, y: 20, width: 1100, height: 700, maximized: false });
    state.maximized = true;
    emit('maximize');
    expect(onPlacement).toHaveBeenLastCalledWith({ x: 10, y: 20, width: 1100, height: 700, maximized: true });
  });

  it('全屏时不报告位置，退出全屏后再报告', () => {
    const { window, emit, state } = createWindow({ fullScreen: true });
    const onPlacement = vi.fn();
    watchWindow(window, { onPlacement, onState: vi.fn() });
    emit('resize');
    emit('move');
    expect(onPlacement).not.toHaveBeenCalled();
    state.fullScreen = false;
    emit('leave-full-screen');
    expect(onPlacement).toHaveBeenCalledTimes(1);
  });

  it('窗口已销毁时不报告', () => {
    const { window, emit } = createWindow({ destroyed: true });
    const onPlacement = vi.fn();
    watchWindow(window, { onPlacement, onState: vi.fn() });
    emit('close');
    expect(onPlacement).not.toHaveBeenCalled();
  });

  it('最大化、还原、进入与退出全屏时通知状态变了，移动与改变大小不通知', () => {
    const { window, emit } = createWindow();
    const onState = vi.fn();
    watchWindow(window, { onPlacement: vi.fn(), onState });
    emit('move');
    emit('resize');
    expect(onState).not.toHaveBeenCalled();
    for (const event of ['maximize', 'unmaximize', 'enter-full-screen', 'leave-full-screen']) emit(event);
    expect(onState).toHaveBeenCalledTimes(4);
  });
});
