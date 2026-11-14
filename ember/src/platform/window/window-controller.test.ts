/*
 * ------------------------------------------------------------------------
 * 名称：window-controller.test.ts
 * 说明：窗口控制器的测试：各动作调用窗口的对应方法、最大化与还原、全屏时不响应最大化、没有窗口或窗口已销毁时不动作、退出应用不需要窗口、状态读取、面板尺寸变化时伸缩窗口并更新最小尺寸。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

// @vitest-environment node

import { describe, expect, it, vi } from 'vitest';
import { BASE_MIN_HEIGHT, BASE_MIN_WIDTH, type Rect } from './panel-bounds';
import { createWindowController, type ControlledWindow, type WindowControllerEnvironment } from './window-controller';

const WORK_AREA: Rect = { x: 0, y: 0, width: 1920, height: 1040 };

/** 假环境：工作区固定，设置边界时直接改假窗口的边界，并记下调用顺序。 */
function createEnvironment(calls: string[] = []): WindowControllerEnvironment {
  return {
    workAreaOf: () => WORK_AREA,
    applyBounds: (window, rect) => {
      calls.push('applyBounds');
      (window as ReturnType<typeof createWindow>).bounds = rect;
    }
  };
}

/** 假窗口：最大化与全屏状态可调，边界与最小尺寸可读。 */
function createWindow(initial: { maximized?: boolean; fullScreen?: boolean; destroyed?: boolean; bounds?: Rect; calls?: string[] } = {}) {
  const state = { maximized: initial.maximized ?? false, fullScreen: initial.fullScreen ?? false };
  const window = {
    bounds: initial.bounds ?? { x: 400, y: 100, width: 1000, height: 700 },
    minimum: { width: BASE_MIN_WIDTH, height: BASE_MIN_HEIGHT },
    getBounds() {
      return window.bounds;
    },
    getNormalBounds() {
      return window.bounds;
    },
    setBounds: vi.fn((bounds: Rect) => {
      window.bounds = bounds;
    }),
    setMinimumSize: vi.fn((width: number, height: number) => {
      initial.calls?.push(`min:${String(width)}x${String(height)}`);
      window.minimum = { width, height };
    }),
    isDestroyed: () => initial.destroyed ?? false,
    minimize: vi.fn(),
    isMaximized: () => state.maximized,
    maximize: vi.fn(() => {
      state.maximized = true;
    }),
    unmaximize: vi.fn(() => {
      state.maximized = false;
    }),
    isFullScreen: () => state.fullScreen,
    setFullScreen: vi.fn((flag: boolean) => {
      state.fullScreen = flag;
    }),
    close: vi.fn()
  };
  return window satisfies ControlledWindow;
}

describe('createWindowController', () => {
  it('最小化与关闭调用窗口的对应方法', () => {
    const window = createWindow();
    const controller = createWindowController(() => window, vi.fn(), createEnvironment());
    controller.perform('minimize');
    controller.perform('close');
    expect(window.minimize).toHaveBeenCalledTimes(1);
    expect(window.close).toHaveBeenCalledTimes(1);
  });

  it('最大化与还原交替', () => {
    const window = createWindow();
    const controller = createWindowController(() => window, vi.fn(), createEnvironment());
    controller.perform('toggleMaximize');
    expect(window.maximize).toHaveBeenCalledTimes(1);
    expect(controller.state().maximized).toBe(true);
    controller.perform('toggleMaximize');
    expect(window.unmaximize).toHaveBeenCalledTimes(1);
    expect(controller.state().maximized).toBe(false);
  });

  it('全屏时不响应最大化', () => {
    const window = createWindow({ fullScreen: true });
    createWindowController(() => window, vi.fn(), createEnvironment()).perform('toggleMaximize');
    expect(window.maximize).not.toHaveBeenCalled();
    expect(window.unmaximize).not.toHaveBeenCalled();
  });

  it('全屏与退出全屏交替', () => {
    const window = createWindow();
    const controller = createWindowController(() => window, vi.fn(), createEnvironment());
    controller.perform('toggleFullScreen');
    expect(window.setFullScreen).toHaveBeenLastCalledWith(true);
    expect(controller.state().fullScreen).toBe(true);
    controller.perform('toggleFullScreen');
    expect(window.setFullScreen).toHaveBeenLastCalledWith(false);
  });

  it('退出应用不需要窗口', () => {
    const quit = vi.fn();
    createWindowController(() => undefined, quit, createEnvironment()).perform('quit');
    expect(quit).toHaveBeenCalledTimes(1);
  });

  it('没有窗口时其余动作什么都不做，状态都是 false', () => {
    const controller = createWindowController(() => undefined, vi.fn(), createEnvironment());
    expect(() => {
      controller.perform('minimize');
      controller.perform('toggleMaximize');
      controller.perform('toggleFullScreen');
      controller.perform('close');
    }).not.toThrow();
    expect(controller.state()).toEqual({ maximized: false, fullScreen: false });
  });

  it('窗口已销毁时不动作', () => {
    const window = createWindow({ destroyed: true, maximized: true });
    const controller = createWindowController(() => window, vi.fn(), createEnvironment());
    controller.perform('minimize');
    controller.perform('close');
    expect(window.minimize).not.toHaveBeenCalled();
    expect(window.close).not.toHaveBeenCalled();
    expect(controller.state()).toEqual({ maximized: false, fullScreen: false });
  });
});

describe('createWindowController 的面板尺寸', () => {
  const SIDE = { left: 240, right: 0, bottom: 0 };

  it('展开边栏时窗口向左伸，最小尺寸加上边栏宽度', () => {
    const window = createWindow();
    const controller = createWindowController(() => window, vi.fn(), createEnvironment());
    controller.setPanelExtents(SIDE);
    expect(window.bounds).toEqual({ x: 160, y: 100, width: 1240, height: 700 });
    expect(window.minimum).toEqual({ width: BASE_MIN_WIDTH + 240, height: BASE_MIN_HEIGHT });
    expect(controller.panelExtents()).toEqual(SIDE);
  });

  it('折叠后窗口缩回原来的边界与最小尺寸', () => {
    const window = createWindow();
    const controller = createWindowController(() => window, vi.fn(), createEnvironment());
    controller.setPanelExtents(SIDE);
    controller.setPanelExtents({ left: 0, right: 0, bottom: 0 });
    expect(window.bounds).toEqual({ x: 400, y: 100, width: 1000, height: 700 });
    expect(window.minimum).toEqual({ width: BASE_MIN_WIDTH, height: BASE_MIN_HEIGHT });
  });

  it('缩小窗口前先放宽最小尺寸，边界设好后再收紧', () => {
    const calls: string[] = [];
    const window = createWindow({ calls });
    const controller = createWindowController(() => window, vi.fn(), createEnvironment(calls));
    controller.setPanelExtents(SIDE);
    calls.length = 0;
    controller.setPanelExtents({ left: 0, right: 0, bottom: 0 });
    expect(calls).toEqual([`min:${String(BASE_MIN_WIDTH)}x${String(BASE_MIN_HEIGHT)}`, 'applyBounds', `min:${String(BASE_MIN_WIDTH)}x${String(BASE_MIN_HEIGHT)}`]);
  });

  it('最大化时只记录尺寸并设置最小尺寸，不改窗口边界', () => {
    const environment = createEnvironment();
    const apply = vi.spyOn(environment, 'applyBounds');
    const window = createWindow({ maximized: true });
    const controller = createWindowController(() => window, vi.fn(), environment);
    controller.setPanelExtents(SIDE);
    expect(apply).not.toHaveBeenCalled();
    expect(window.minimum).toEqual({ width: BASE_MIN_WIDTH + 240, height: BASE_MIN_HEIGHT });
    expect(controller.panelExtents()).toEqual(SIDE);
  });

  it('窗口换了一个后面板尺寸重新从全部折叠算起', () => {
    let current = createWindow();
    const controller = createWindowController(() => current, vi.fn(), createEnvironment());
    controller.setPanelExtents(SIDE);
    current = createWindow();
    expect(controller.panelExtents()).toEqual({ left: 0, right: 0, bottom: 0 });
  });

  it('没有窗口或窗口已销毁时不动作', () => {
    const none = createWindowController(() => undefined, vi.fn(), createEnvironment());
    expect(() => none.setPanelExtents(SIDE)).not.toThrow();
    const window = createWindow({ destroyed: true });
    createWindowController(() => window, vi.fn(), createEnvironment()).setPanelExtents(SIDE);
    expect(window.setMinimumSize).not.toHaveBeenCalled();
  });
});
