/*
 * ------------------------------------------------------------------------
 * 名称：window-controller.test.ts
 * 说明：窗口控制器的测试：各动作调用窗口的对应方法、最大化与还原、全屏时不响应最大化、没有窗口或窗口已销毁时不动作、退出应用不需要窗口、状态读取。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

// @vitest-environment node

import { describe, expect, it, vi } from 'vitest';
import { createWindowController, type ControlledWindow } from './window-controller';

/** 假窗口：最大化与全屏状态可调。 */
function createWindow(initial: { maximized?: boolean; fullScreen?: boolean; destroyed?: boolean } = {}) {
  const state = { maximized: initial.maximized ?? false, fullScreen: initial.fullScreen ?? false };
  const window = {
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
    const controller = createWindowController(() => window, vi.fn());
    controller.perform('minimize');
    controller.perform('close');
    expect(window.minimize).toHaveBeenCalledTimes(1);
    expect(window.close).toHaveBeenCalledTimes(1);
  });

  it('最大化与还原交替', () => {
    const window = createWindow();
    const controller = createWindowController(() => window, vi.fn());
    controller.perform('toggleMaximize');
    expect(window.maximize).toHaveBeenCalledTimes(1);
    expect(controller.state().maximized).toBe(true);
    controller.perform('toggleMaximize');
    expect(window.unmaximize).toHaveBeenCalledTimes(1);
    expect(controller.state().maximized).toBe(false);
  });

  it('全屏时不响应最大化', () => {
    const window = createWindow({ fullScreen: true });
    createWindowController(() => window, vi.fn()).perform('toggleMaximize');
    expect(window.maximize).not.toHaveBeenCalled();
    expect(window.unmaximize).not.toHaveBeenCalled();
  });

  it('全屏与退出全屏交替', () => {
    const window = createWindow();
    const controller = createWindowController(() => window, vi.fn());
    controller.perform('toggleFullScreen');
    expect(window.setFullScreen).toHaveBeenLastCalledWith(true);
    expect(controller.state().fullScreen).toBe(true);
    controller.perform('toggleFullScreen');
    expect(window.setFullScreen).toHaveBeenLastCalledWith(false);
  });

  it('退出应用不需要窗口', () => {
    const quit = vi.fn();
    createWindowController(() => undefined, quit).perform('quit');
    expect(quit).toHaveBeenCalledTimes(1);
  });

  it('没有窗口时其余动作什么都不做，状态都是 false', () => {
    const controller = createWindowController(() => undefined, vi.fn());
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
    const controller = createWindowController(() => window, vi.fn());
    controller.perform('minimize');
    controller.perform('close');
    expect(window.minimize).not.toHaveBeenCalled();
    expect(window.close).not.toHaveBeenCalled();
    expect(controller.state()).toEqual({ maximized: false, fullScreen: false });
  });
});
