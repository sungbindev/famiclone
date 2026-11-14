/*
 * ------------------------------------------------------------------------
 * 名称：use-window-extents.test.tsx
 * 说明：窗口随面板伸缩的界面侧处理的测试：启动时告知一次、相同尺寸不重复告知、拖动与全屏时不告知、调整窗口期间钉住页面区并在主进程返回后放开、连续告知时只由最后一次放开。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：用假的 ResizeObserver 与可手动完成的 setPanelExtents；两帧的等待用假的 requestAnimationFrame 手动推进。
 * ------------------------------------------------------------------------
 */

import { act, renderHook } from '@testing-library/react';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import type { EmberBridge } from '../../contracts/shell/shell-bridge';
import type { PanelExtents } from '../../contracts/shell/window-control';
import { useWindowExtents } from './use-window-extents';

const NONE: PanelExtents = { left: 0, right: 0, bottom: 0 };
const SIDE: PanelExtents = { left: 240, right: 0, bottom: 0 };
const BOTH: PanelExtents = { left: 240, right: 0, bottom: 160 };

/** 页面区容器的假尺寸。 */
const PAGES_SIZE = { width: 800, height: 600 };

/** 可手动完成的 setPanelExtents：每次调用都记录，replyAll 让全部未完成的调用返回。 */
function createBridge() {
  const pending: Array<() => void> = [];
  const setPanelExtents = vi.fn(() => new Promise<void>((resolve) => pending.push(resolve)));
  return {
    bridge: { shell: { setPanelExtents } } as unknown as EmberBridge,
    setPanelExtents,
    replyAll: async (): Promise<void> => {
      await act(async () => {
        for (const resolve of pending.splice(0)) resolve();
        await Promise.resolve();
      });
    }
  };
}

/** 渲染钩子并把容器挂到引用上（容器的 clientWidth 与 clientHeight 是假的）。 */
function renderExtents(bridge: EmberBridge, fullScreen = false) {
  const hook = renderHook(({ full }) => useWindowExtents(bridge, full), { initialProps: { full: fullScreen } });
  const element = document.createElement('div');
  Object.defineProperty(element, 'clientWidth', { value: PAGES_SIZE.width });
  Object.defineProperty(element, 'clientHeight', { value: PAGES_SIZE.height });
  let detach: void | (() => void);
  act(() => {
    detach = hook.result.current.pagesRef(element);
  });
  return { ...hook, detach: () => (typeof detach === 'function' ? detach() : undefined) };
}

let frames: Array<() => void> = [];

/** 推进两帧：每帧执行当时已登记的回调。 */
async function runFrames(count: number): Promise<void> {
  for (let index = 0; index < count; index += 1) {
    const current = frames;
    frames = [];
    await act(async () => {
      for (const callback of current) callback();
      await Promise.resolve();
    });
  }
}

beforeEach(() => {
  frames = [];
  vi.stubGlobal('ResizeObserver', class {
    observe = vi.fn();
    disconnect = vi.fn();
  });
  vi.stubGlobal('requestAnimationFrame', (callback: () => void) => frames.push(callback));
});

afterEach(() => {
  vi.unstubAllGlobals();
});

describe('useWindowExtents', () => {
  it('第一次通知直接告知主进程，不钉住页面区', () => {
    const { bridge, setPanelExtents } = createBridge();
    const { result } = renderExtents(bridge);
    act(() => result.current.onPanelExtentsChange(NONE, false));
    expect(setPanelExtents.mock.calls).toEqual([[NONE]]);
    expect(result.current.pinnedStyle).toBeUndefined();
  });

  it('尺寸变化时告知并把页面区钉在稳定尺寸上，主进程返回再过两帧后放开', async () => {
    const { bridge, setPanelExtents, replyAll } = createBridge();
    const { result } = renderExtents(bridge);
    act(() => result.current.onPanelExtentsChange(NONE, false));
    await replyAll();
    act(() => result.current.onPanelExtentsChange(SIDE, false));
    expect(setPanelExtents).toHaveBeenLastCalledWith(SIDE);
    expect(result.current.pinnedStyle).toEqual({ width: 800, height: 600 });
    await replyAll();
    expect(result.current.pinnedStyle).toEqual({ width: 800, height: 600 });
    await runFrames(1);
    expect(result.current.pinnedStyle).toEqual({ width: 800, height: 600 });
    await runFrames(1);
    expect(result.current.pinnedStyle).toBeUndefined();
  });

  it('尺寸与上次告知的相同时不再告知', () => {
    const { bridge, setPanelExtents } = createBridge();
    const { result } = renderExtents(bridge);
    act(() => result.current.onPanelExtentsChange(SIDE, false));
    act(() => result.current.onPanelExtentsChange({ ...SIDE }, false));
    expect(setPanelExtents).toHaveBeenCalledTimes(1);
  });

  it('拖动分隔条期间不告知，拖动结束后告知最终尺寸', () => {
    const { bridge, setPanelExtents } = createBridge();
    const { result } = renderExtents(bridge);
    act(() => result.current.onPanelExtentsChange(NONE, false));
    act(() => result.current.onPanelExtentsChange(SIDE, true));
    act(() => result.current.onPanelExtentsChange({ ...SIDE, left: 300 }, true));
    expect(setPanelExtents).toHaveBeenCalledTimes(1);
    act(() => result.current.onPanelExtentsChange({ ...SIDE, left: 300 }, false));
    expect(setPanelExtents).toHaveBeenLastCalledWith({ left: 300, right: 0, bottom: 0 });
  });

  it('全屏时不告知', () => {
    const { bridge, setPanelExtents } = createBridge();
    const { result } = renderExtents(bridge, true);
    act(() => result.current.onPanelExtentsChange(SIDE, false));
    expect(setPanelExtents).not.toHaveBeenCalled();
  });

  it('连续告知时只有最后一次返回后才放开', async () => {
    const { bridge, replyAll } = createBridge();
    const { result } = renderExtents(bridge);
    act(() => result.current.onPanelExtentsChange(NONE, false));
    await replyAll();
    act(() => result.current.onPanelExtentsChange(SIDE, false));
    await replyAll();
    act(() => result.current.onPanelExtentsChange(BOTH, false));
    await runFrames(2);
    expect(result.current.pinnedStyle).toEqual({ width: 800, height: 600 });
    await replyAll();
    await runFrames(2);
    expect(result.current.pinnedStyle).toBeUndefined();
  });

  it('页面区的容器卸载时断开尺寸监听', () => {
    const { bridge } = createBridge();
    const { detach } = renderExtents(bridge);
    expect(() => detach()).not.toThrow();
  });
});
