/*
 * ------------------------------------------------------------------------
 * 名称：native-keyboard-capture.test.ts
 * 说明：键盘独占实现的测试（原生模块与窗口都用假对象）：开启时把窗口句柄交给原生模块、按键转成键位名交给订阅者、窗口失焦等事件自动关闭并通知、不支持或失败时返回 false、只加载一次原生模块、关闭后不再转发、平台不是 Windows 时不加载。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import { EventEmitter } from 'node:events';
import { describe, expect, it, vi } from 'vitest';
import { loadNativeKeyboard, NativeKeyboardCapture, type CaptureWindow, type NativeKeyboardModule, type RawKeyEvent } from './native-keyboard-capture';

/** 假窗口：用事件发射器模拟窗口事件。 */
class FakeWindow extends EventEmitter implements CaptureWindow {
  destroyed = false;
  getNativeWindowHandle(): Buffer {
    return Buffer.from([1, 2, 3, 4, 0, 0, 0, 0]);
  }
  isDestroyed(): boolean {
    return this.destroyed;
  }
}

/** 假原生模块：记录调用，可以主动交来按键。 */
function fakeNative(enableResult = 0) {
  let callback: ((event: RawKeyEvent) => void) | undefined;
  const module: NativeKeyboardModule = {
    enable: vi.fn((_handle: Buffer, next: (event: RawKeyEvent) => void) => {
      callback = next;
      return enableResult;
    }),
    disable: vi.fn()
  };
  return { module, press: (event: Partial<RawKeyEvent> & { make: number }) => callback?.({ e0: false, e1: false, up: false, ...event }) };
}

/** 创建被测对象。 */
function setup(options: { enableResult?: number; native?: boolean; window?: boolean } = {}) {
  const native = fakeNative(options.enableResult ?? 0);
  const window = new FakeWindow();
  const load = vi.fn(() => (options.native === false ? undefined : native.module));
  const onError = vi.fn();
  const capture = new NativeKeyboardCapture({ load, getWindow: () => (options.window === false ? undefined : window), onError });
  return { capture, native, window, load, onError };
}

describe('setCapture', () => {
  it('开启时把窗口句柄与回调交给原生模块，返回 true', () => {
    const { capture, native } = setup();
    expect(capture.setCapture(true)).toBe(true);
    expect(native.module.enable).toHaveBeenCalledTimes(1);
    expect(vi.mocked(native.module.enable).mock.calls[0]?.[0]).toEqual(Buffer.from([1, 2, 3, 4, 0, 0, 0, 0]));
  });

  it('重复开启不重复调用原生模块，原生模块只加载一次', () => {
    const { capture, native, load } = setup();
    capture.setCapture(true);
    expect(capture.setCapture(true)).toBe(true);
    capture.setCapture(false);
    capture.setCapture(true);
    expect(native.module.enable).toHaveBeenCalledTimes(2);
    expect(load).toHaveBeenCalledTimes(1);
  });

  it('关闭时调用原生模块的 disable，没有开启时关闭什么也不做', () => {
    const { capture, native } = setup();
    expect(capture.setCapture(false)).toBe(false);
    expect(native.module.disable).not.toHaveBeenCalled();
    capture.setCapture(true);
    expect(capture.setCapture(false)).toBe(false);
    expect(native.module.disable).toHaveBeenCalledTimes(1);
  });

  it('没有原生模块、没有窗口或窗口已销毁时返回 false', () => {
    expect(setup({ native: false }).capture.setCapture(true)).toBe(false);
    expect(setup({ window: false }).capture.setCapture(true)).toBe(false);
    const destroyed = setup();
    destroyed.window.destroyed = true;
    expect(destroyed.capture.setCapture(true)).toBe(false);
    expect(destroyed.native.module.enable).not.toHaveBeenCalled();
  });

  it('原生模块开启失败时返回 false 并记录错误码，之后不会误以为已开启', () => {
    const { capture, native, onError, window } = setup({ enableResult: 1400 });
    expect(capture.setCapture(true)).toBe(false);
    expect(onError).toHaveBeenCalledWith(expect.stringContaining('1400'));
    expect(window.listenerCount('blur')).toBe(0);
    capture.setCapture(false);
    expect(native.module.disable).not.toHaveBeenCalled();
  });
});

describe('按键', () => {
  it('原始扫描码转成键位名交给订阅者，按下与松开都有', () => {
    const { capture, native } = setup();
    const keys: unknown[] = [];
    capture.onKey((event) => keys.push(event));
    capture.setCapture(true);
    native.press({ make: 0x01 });
    native.press({ make: 0x01, up: true });
    native.press({ make: 0x48, e0: true });
    expect(keys).toEqual([
      { code: 'Escape', down: true },
      { code: 'Escape', down: false },
      { code: 'ArrowUp', down: true }
    ]);
  });

  it('假 Shift、Pause 与未收录的扫描码不交给订阅者', () => {
    const { capture, native } = setup();
    const listener = vi.fn();
    capture.onKey(listener);
    capture.setCapture(true);
    native.press({ make: 0x2a, e0: true });
    native.press({ make: 0x1d, e1: true });
    native.press({ make: 0x7f });
    expect(listener).not.toHaveBeenCalled();
  });

  it('取消订阅后不再收到', () => {
    const { capture, native } = setup();
    const listener = vi.fn();
    const stop = capture.onKey(listener);
    capture.setCapture(true);
    stop();
    native.press({ make: 0x1e });
    expect(listener).not.toHaveBeenCalled();
  });
});

describe('自动关闭', () => {
  it.each(['blur', 'hide', 'minimize', 'closed'] as const)('窗口 %s 时关闭独占并通知订阅者', (event) => {
    const { capture, native, window } = setup();
    const released = vi.fn();
    capture.onReleased(released);
    capture.setCapture(true);
    window.emit(event);
    expect(native.module.disable).toHaveBeenCalledTimes(1);
    expect(released).toHaveBeenCalledTimes(1);
    expect(window.listenerCount('blur')).toBe(0);
    expect(window.listenerCount('closed')).toBe(0);
  });

  it('界面主动关闭后，窗口事件不再通知', () => {
    const { capture, window } = setup();
    const released = vi.fn();
    capture.onReleased(released);
    capture.setCapture(true);
    capture.setCapture(false);
    window.emit('blur');
    expect(released).not.toHaveBeenCalled();
  });

  it('自动关闭后可以再次开启', () => {
    const { capture, native, window } = setup();
    capture.setCapture(true);
    window.emit('blur');
    expect(capture.setCapture(true)).toBe(true);
    expect(native.module.enable).toHaveBeenCalledTimes(2);
  });

  it('dispose 关闭独占', () => {
    const { capture, native } = setup();
    capture.setCapture(true);
    capture.dispose();
    expect(native.module.disable).toHaveBeenCalledTimes(1);
  });
});

describe('loadNativeKeyboard', () => {
  it('不是 Windows 时不加载，也不报错', () => {
    const onError = vi.fn();
    expect(loadNativeKeyboard('不存在.node', 'darwin', onError)).toBeUndefined();
    expect(loadNativeKeyboard('不存在.node', 'linux', onError)).toBeUndefined();
    expect(onError).not.toHaveBeenCalled();
  });

  it('Windows 上文件不存在或不是原生模块时返回 undefined 并记录原因', () => {
    const onError = vi.fn();
    expect(loadNativeKeyboard('Z:\\不存在\\ember_keyboard.node', 'win32', onError)).toBeUndefined();
    expect(onError).toHaveBeenCalledWith(expect.stringContaining('加载键盘独占模块失败'));
  });
});
