/*
 * ------------------------------------------------------------------------
 * 名称：RuntimeScreen.test.tsx
 * 说明：画面组件的测试（Worker、画布与端口都用假对象）：创建 Worker 并交出离屏画布、收到端口后画面端口交给 Worker、版面随容器尺寸与属性变化、键盘、鼠标、游戏手柄的原始事件经输入端口送出、失焦与卸载时松开全部按键、可编辑元素不采集、卸载时终止 Worker。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import { UiProvider } from '@rujianjs/ui-kit';
import { act, fireEvent, render } from '@testing-library/react';
import { createRef, type RefObject } from 'react';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import type { EmberBridge } from '../../contracts/shell/shell-bridge';
import type { NativeKeyEvent } from '../../contracts/shell/keyboard-capture';
import { RuntimeScreen } from './RuntimeScreen';

/** 假 Worker：记录收到的消息与转移列表。 */
class FakeWorker {
  static instances: FakeWorker[] = [];
  readonly posted: Array<{ message: Record<string, unknown>; transfer: unknown[] | undefined }> = [];
  terminated = false;
  constructor(
    readonly url: URL,
    readonly options: WorkerOptions | undefined
  ) {
    FakeWorker.instances.push(this);
  }
  postMessage(message: Record<string, unknown>, transfer?: unknown[]): void {
    this.posted.push({ message, transfer });
  }
  addEventListener(): void {}
  terminate(): void {
    this.terminated = true;
  }
  messages(type: string): Array<Record<string, unknown>> {
    return this.posted.filter((entry) => entry.message['type'] === type).map((entry) => entry.message);
  }
}

/** 假端口。 */
function fakePort() {
  return { postMessage: vi.fn(), close: vi.fn() };
}

/** 记录 ResizeObserver 的回调，让测试主动触发。 */
let resizeCallback: ((entries: unknown[]) => void) | undefined;

/** 渲染组件，返回画面区与假接口。 */
function setup(props: { aspect?: '4:3' | '8:7'; scaling?: 'fit' | 'integer'; showAllLines?: boolean; captureMouse?: boolean } = {}) {
  const requestPorts = vi.fn();
  const nativeKeyListeners = new Set<(event: NativeKeyEvent) => void>();
  const releasedListeners = new Set<() => void>();
  const setKeyboardCapture = vi.fn((_enabled: boolean) => Promise.resolve(true));
  const shell = {
    setKeyboardCapture,
    onNativeKey: (listener: (event: NativeKeyEvent) => void) => {
      nativeKeyListeners.add(listener);
      return () => nativeKeyListeners.delete(listener);
    },
    onKeyboardReleased: (listener: () => void) => {
      releasedListeners.add(listener);
      return () => releasedListeners.delete(listener);
    }
  };
  const bridge = { streams: { requestPorts }, shell, core: {}, main: {}, onCoreEvent: () => () => undefined, getCoreStatus: () => Promise.reject(new Error('未使用')) } as unknown as EmberBridge;
  const screenRef: RefObject<HTMLDivElement | null> = createRef<HTMLDivElement>();
  const view = render(
    <UiProvider>
      <RuntimeScreen bridge={bridge} screenRef={screenRef} {...props} />
    </UiProvider>
  );
  const screenElement = view.getByRole('application', { name: '模拟器画面' });
  const worker = FakeWorker.instances[FakeWorker.instances.length - 1];
  if (worker === undefined) throw new Error('没有创建 Worker');
  return {
    view,
    screenElement,
    worker,
    requestPorts,
    setKeyboardCapture,
    /** 主进程交来一次独占期间的物理按键。 */
    emitNativeKey: (code: string, down: boolean) => act(() => nativeKeyListeners.forEach((listener) => listener({ code, down }))),
    /** 主进程通知独占已被自动关闭。 */
    emitReleased: () => act(() => releasedListeners.forEach((listener) => listener())),
    rerender: (next: { aspect?: '4:3' | '8:7'; scaling?: 'fit' | 'integer'; showAllLines?: boolean; captureMouse?: boolean }) => view.rerender(<UiProvider><RuntimeScreen bridge={bridge} screenRef={screenRef} {...next} /></UiProvider>)
  };
}

/** 模拟预加载脚本交来端口。 */
function deliverPorts(frames: unknown, input: unknown): void {
  const event = Object.assign(new Event('message'), { data: { type: 'ember.stream-ports' }, source: window, ports: [frames, input] });
  act(() => {
    window.dispatchEvent(event);
  });
}

beforeEach(() => {
  FakeWorker.instances = [];
  vi.stubGlobal('Worker', FakeWorker);
  vi.stubGlobal(
    'ResizeObserver',
    class {
      constructor(callback: (entries: unknown[]) => void) {
        resizeCallback = callback;
      }
      observe(): void {}
      disconnect(): void {}
    }
  );
  Object.defineProperty(window, 'matchMedia', { configurable: true, value: () => ({ addEventListener: () => undefined, removeEventListener: () => undefined }) });
  Object.defineProperty(HTMLCanvasElement.prototype, 'transferControlToOffscreen', { configurable: true, value: () => ({ offscreen: true }) });
});

afterEach(() => {
  vi.unstubAllGlobals();
  resizeCallback = undefined;
});

describe('RuntimeScreen：Worker 与端口', () => {
  it('创建 ES 模块 Worker，把离屏画布交给它，并请求端口', () => {
    const { worker, requestPorts } = setup();
    expect(worker.options).toEqual({ type: 'module' });
    expect(worker.url.pathname).toMatch(/render-worker\.ts$/);
    const [init] = worker.posted;
    expect(init?.message).toEqual({ type: 'init', canvas: { offscreen: true } });
    expect(init?.transfer).toEqual([{ offscreen: true }]);
    expect(requestPorts).toHaveBeenCalledTimes(1);
  });

  it('画面区可聚焦并在显示时自动聚焦', () => {
    const { screenElement } = setup();
    expect(screenElement.tabIndex).toBe(0);
    expect(document.activeElement).toBe(screenElement);
  });

  it('收到端口后把画面端口转移给 Worker，输入端口留在页面里', () => {
    const { worker } = setup();
    const frames = fakePort();
    const input = fakePort();
    deliverPorts(frames, input);
    const attach = worker.posted.find((entry) => entry.message['type'] === 'attach');
    expect(attach?.message['port']).toBe(frames);
    expect(attach?.transfer).toEqual([frames]);
    expect(input.postMessage).not.toHaveBeenCalled();
  });

  it('忽略不是自己窗口发来的或标记不对的端口消息', () => {
    const { worker } = setup();
    const frames = fakePort();
    const input = fakePort();
    act(() => {
      window.dispatchEvent(Object.assign(new Event('message'), { data: { type: 'ember.stream-ports' }, source: null, ports: [frames, input] }));
      window.dispatchEvent(Object.assign(new Event('message'), { data: { type: 'other' }, source: window, ports: [frames, input] }));
      window.dispatchEvent(Object.assign(new Event('message'), { data: { type: 'ember.stream-ports' }, source: window, ports: [frames] }));
    });
    expect(worker.messages('attach')).toHaveLength(0);
  });

  it('新端口到达时关闭旧的输入端口', () => {
    setup();
    const firstInput = fakePort();
    deliverPorts(fakePort(), firstInput);
    deliverPorts(fakePort(), fakePort());
    expect(firstInput.close).toHaveBeenCalledTimes(1);
  });

  it('卸载时终止 Worker、关闭输入端口并移除画布', () => {
    const { view, worker, screenElement } = setup();
    const input = fakePort();
    deliverPorts(fakePort(), input);
    expect(screenElement.querySelector('canvas')).not.toBeNull();
    view.unmount();
    expect(worker.terminated).toBe(true);
    expect(input.close).toHaveBeenCalled();
  });
});

describe('RuntimeScreen：版面', () => {
  it('容器尺寸变化时按设备像素尺寸发版面，带上画面比例、缩放方式与裁剪行数的默认值（显示全部行）', () => {
    const { worker } = setup();
    act(() => resizeCallback?.([{ devicePixelContentBoxSize: [{ inlineSize: 1280, blockSize: 960 }] }]));
    const views = worker.messages('view');
    expect(views[views.length - 1]).toEqual({ type: 'view', view: { width: 1280, height: 960, aspect: '8:7', scaling: 'fit', cropLines: 0 } });
  });

  it('属性变化后重新发版面', () => {
    const { worker, rerender } = setup();
    act(() => resizeCallback?.([{ devicePixelContentBoxSize: [{ inlineSize: 800, blockSize: 600 }] }]));
    rerender({ aspect: '4:3', scaling: 'integer' });
    const views = worker.messages('view');
    expect(views[views.length - 1]).toEqual({ type: 'view', view: { width: 800, height: 600, aspect: '4:3', scaling: 'integer', cropLines: 0 } });
  });

  it('关闭“显示全部 240 行”后上下各裁 8 行，打开后恢复', () => {
    const { worker, rerender } = setup({ showAllLines: false });
    act(() => resizeCallback?.([{ devicePixelContentBoxSize: [{ inlineSize: 800, blockSize: 600 }] }]));
    let views = worker.messages('view');
    expect(views[views.length - 1]).toEqual({ type: 'view', view: { width: 800, height: 600, aspect: '8:7', scaling: 'fit', cropLines: 8 } });
    rerender({ showAllLines: true });
    views = worker.messages('view');
    expect(views[views.length - 1]).toEqual({ type: 'view', view: { width: 800, height: 600, aspect: '8:7', scaling: 'fit', cropLines: 0 } });
  });
});

describe('RuntimeScreen：键盘', () => {
  it('按下与松开的键经输入端口送出原始事件，重复的按下不重复发送，没有任何默认键位', () => {
    const { screenElement } = setup();
    const input = fakePort();
    deliverPorts(fakePort(), input);
    fireEvent.keyDown(screenElement, { code: 'KeyX' });
    fireEvent.keyDown(screenElement, { code: 'KeyX', repeat: true });
    fireEvent.keyDown(screenElement, { code: 'ArrowRight' });
    fireEvent.keyUp(screenElement, { code: 'KeyX' });
    fireEvent.keyUp(screenElement, { code: 'KeyQ' });
    expect(input.postMessage.mock.calls).toEqual([[{ type: 'key', code: 'KeyX', down: true }], [{ type: 'key', code: 'ArrowRight', down: true }], [{ type: 'key', code: 'KeyX', down: false }]]);
  });

  it('上报的键阻止默认行为，功能键与带 Ctrl 的组合不上报也不阻止', () => {
    const { screenElement } = setup();
    const input = fakePort();
    deliverPorts(fakePort(), input);
    expect(fireEvent.keyDown(screenElement, { code: 'Enter' })).toBe(false);
    expect(fireEvent.keyDown(screenElement, { code: 'F9' })).toBe(true);
    expect(fireEvent.keyDown(screenElement, { code: 'KeyO', ctrlKey: true })).toBe(true);
    expect(input.postMessage.mock.calls).toEqual([[{ type: 'key', code: 'Enter', down: true }]]);
  });

  it('画面区失焦、窗口失焦、页面隐藏、卸载时松开全部按键', () => {
    const { view, screenElement } = setup();
    const input = fakePort();
    deliverPorts(fakePort(), input);
    const press = () => fireEvent.keyDown(screenElement, { code: 'KeyZ' });
    const released = () => input.postMessage.mock.calls.filter(([message]) => (message as { type: string; down: boolean }).type === 'key' && !(message as { down: boolean }).down).length;

    press();
    fireEvent.blur(screenElement);
    expect(released()).toBe(1);

    press();
    act(() => {
      window.dispatchEvent(new Event('blur'));
    });
    expect(released()).toBe(2);

    press();
    Object.defineProperty(document, 'visibilityState', { configurable: true, get: () => 'hidden' });
    act(() => {
      document.dispatchEvent(new Event('visibilitychange'));
    });
    Object.defineProperty(document, 'visibilityState', { configurable: true, get: () => 'visible' });
    expect(released()).toBe(3);

    press();
    view.unmount();
    expect(released()).toBe(4);
  });

  it('焦点在可编辑元素上时不采集', () => {
    const { screenElement } = setup();
    const input = fakePort();
    deliverPorts(fakePort(), input);
    const field = document.createElement('input');
    screenElement.append(field);
    fireEvent.keyDown(field, { code: 'KeyX' });
    expect(input.postMessage).not.toHaveBeenCalled();
  });
});

/** 记录发往核心的按键消息。 */
function keyMessages(input: ReturnType<typeof fakePort>): unknown[] {
  return input.postMessage.mock.calls.map(([message]) => message).filter((message) => (message as { type: string }).type === 'key');
}

describe('RuntimeScreen：键盘独占', () => {
  it('画面区持有焦点时请主进程独占键盘，失去焦点时关闭', () => {
    const { screenElement, setKeyboardCapture } = setup();
    expect(setKeyboardCapture.mock.calls).toEqual([[true]]);
    act(() => screenElement.blur());
    expect(setKeyboardCapture.mock.calls).toEqual([[true], [false]]);
    act(() => screenElement.focus());
    expect(setKeyboardCapture.mock.calls).toEqual([[true], [false], [true]]);
  });

  it('独占期间的物理按键全部上报，Esc、Tab、F 键与 Ctrl、Alt 组合也不例外，重复的按下不重复发送', () => {
    const { emitNativeKey } = setup();
    const input = fakePort();
    deliverPorts(fakePort(), input);
    for (const code of ['Escape', 'Tab', 'F5', 'ControlLeft', 'KeyC']) emitNativeKey(code, true);
    emitNativeKey('KeyC', true);
    emitNativeKey('KeyC', false);
    expect(keyMessages(input)).toEqual([
      { type: 'key', code: 'Escape', down: true },
      { type: 'key', code: 'Tab', down: true },
      { type: 'key', code: 'F5', down: true },
      { type: 'key', code: 'ControlLeft', down: true },
      { type: 'key', code: 'KeyC', down: true },
      { type: 'key', code: 'KeyC', down: false }
    ]);
  });

  it('主进程自动关闭独占（窗口失焦等）时松开全部按键，窗口回到前台后再次开启', () => {
    const { emitNativeKey, emitReleased, setKeyboardCapture } = setup();
    const input = fakePort();
    deliverPorts(fakePort(), input);
    emitNativeKey('AltLeft', true);
    emitReleased();
    expect(keyMessages(input)).toEqual([
      { type: 'key', code: 'AltLeft', down: true },
      { type: 'key', code: 'AltLeft', down: false }
    ]);
    act(() => {
      window.dispatchEvent(new Event('focus'));
    });
    expect(setKeyboardCapture.mock.calls).toEqual([[true], [true]]);
  });

  it('卸载时关闭独占并松开全部按键', () => {
    const { view, emitNativeKey, setKeyboardCapture } = setup();
    const input = fakePort();
    deliverPorts(fakePort(), input);
    emitNativeKey('KeyZ', true);
    view.unmount();
    expect(setKeyboardCapture.mock.calls).toEqual([[true], [false]]);
    expect(keyMessages(input)).toEqual([
      { type: 'key', code: 'KeyZ', down: true },
      { type: 'key', code: 'KeyZ', down: false }
    ]);
  });
});

/** 把画面区设为 512x480 并触发一次尺寸变化：8:7 的画面在它里面宽 512、高 420、上下各留 30。 */
function sizeScreen(screenElement: HTMLElement): void {
  screenElement.getBoundingClientRect = () => ({ left: 0, top: 0, width: 512, height: 480, right: 512, bottom: 480, x: 0, y: 0, toJSON: () => ({}) });
  act(() => resizeCallback?.([{ devicePixelContentBoxSize: [{ inlineSize: 512, blockSize: 480 }] }]));
}

describe('RuntimeScreen：鼠标', () => {
  it('移动、按下、松开、滚轮、离开经输入端口送出，坐标换算成标准帧的像素', () => {
    const { screenElement } = setup();
    const input = fakePort();
    deliverPorts(fakePort(), input);
    sizeScreen(screenElement);
    fireEvent.pointerMove(screenElement, { clientX: 256, clientY: 240, buttons: 0 });
    fireEvent.pointerMove(screenElement, { clientX: 266, clientY: 240, buttons: 0 });
    fireEvent.pointerDown(screenElement, { clientX: 266, clientY: 240, buttons: 1 });
    fireEvent.pointerUp(screenElement, { clientX: 266, clientY: 240, buttons: 0 });
    fireEvent.wheel(screenElement, { clientX: 266, clientY: 240, deltaY: 200 });
    fireEvent.pointerLeave(screenElement, { buttons: 0 });
    const common = { type: 'mouse', y: 120, dy: 0 };
    expect(input.postMessage.mock.calls).toEqual([
      [{ ...common, kind: 'move', x: 128, dx: 0, buttons: 0, wheel: 0 }],
      [{ ...common, kind: 'move', x: 133, dx: 5, buttons: 0, wheel: 0 }],
      [{ ...common, kind: 'down', x: 133, dx: 0, buttons: 1, wheel: 0 }],
      [{ ...common, kind: 'up', x: 133, dx: 0, buttons: 0, wheel: 0 }],
      [{ ...common, kind: 'wheel', x: 133, dx: 0, buttons: 0, wheel: 2 }],
      [{ ...common, kind: 'leave', x: 133, dx: 0, buttons: 0, wheel: 0 }]
    ]);
  });

  it('还没有画布尺寸时不上报坐标，画面区上的右键菜单被阻止', () => {
    const { screenElement } = setup();
    const input = fakePort();
    deliverPorts(fakePort(), input);
    fireEvent.pointerMove(screenElement, { clientX: 10, clientY: 10 });
    expect(input.postMessage).not.toHaveBeenCalled();
    expect(fireEvent.contextMenu(screenElement)).toBe(false);
  });
});

/** 把画面区当作指针锁定的对象：requestPointerLock 锁定并触发 pointerlockchange，返回解锁函数。 */
function stubPointerLock(screenElement: HTMLElement) {
  let locked: Element | null = null;
  Object.defineProperty(document, 'pointerLockElement', { configurable: true, get: () => locked });
  const request = vi.fn(() => {
    locked = screenElement;
    document.dispatchEvent(new Event('pointerlockchange'));
  });
  screenElement.requestPointerLock = request as unknown as typeof screenElement.requestPointerLock;
  document.exitPointerLock = vi.fn(() => {
    locked = null;
    document.dispatchEvent(new Event('pointerlockchange'));
  });
  return {
    request,
    unlock: () => {
      locked = null;
      act(() => {
        document.dispatchEvent(new Event('pointerlockchange'));
      });
    }
  };
}

/** 带相对位移的指针移动（jsdom 的事件构造参数不认 movementX）。 */
function moveBy(element: HTMLElement, movementX: number, movementY: number): void {
  const event = new MouseEvent('pointermove', { bubbles: true, buttons: 0 });
  Object.defineProperty(event, 'movementX', { value: movementX });
  Object.defineProperty(event, 'movementY', { value: movementY });
  act(() => {
    element.dispatchEvent(event);
  });
}

describe('RuntimeScreen：捕获鼠标（指针锁定）', () => {
  afterEach(() => {
    Reflect.deleteProperty(document, 'pointerLockElement');
  });

  it('关闭时点击画面不请求指针锁定，按下照常送给机器', () => {
    const { screenElement } = setup();
    const lock = stubPointerLock(screenElement);
    const input = fakePort();
    deliverPorts(fakePort(), input);
    sizeScreen(screenElement);
    fireEvent.pointerDown(screenElement, { clientX: 256, clientY: 240, buttons: 1 });
    expect(lock.request).not.toHaveBeenCalled();
    expect(input.postMessage).toHaveBeenCalledTimes(1);
  });

  it('开启时第一次点击只用来捕获，不把这次按下送给机器', () => {
    const { screenElement } = setup({ captureMouse: true });
    const lock = stubPointerLock(screenElement);
    const input = fakePort();
    deliverPorts(fakePort(), input);
    sizeScreen(screenElement);
    fireEvent.pointerDown(screenElement, { clientX: 256, clientY: 240, buttons: 1 });
    expect(lock.request).toHaveBeenCalledTimes(1);
    expect(input.postMessage).not.toHaveBeenCalled();
  });

  it('捕获期间：移动、按下、松开、滚轮都只有相对位移（CSS 像素换算成标准帧的像素），位置从画面中心累加并限制在整帧内', () => {
    const { screenElement } = setup({ captureMouse: true });
    const lock = stubPointerLock(screenElement);
    const input = fakePort();
    deliverPorts(fakePort(), input);
    sizeScreen(screenElement);
    fireEvent.pointerDown(screenElement, { clientX: 256, clientY: 240, buttons: 1 });
    expect(lock.request).toHaveBeenCalled();
    moveBy(screenElement, 10, 14);
    fireEvent.pointerDown(screenElement, { buttons: 1 });
    fireEvent.pointerUp(screenElement, { buttons: 0 });
    fireEvent.wheel(screenElement, { deltaY: 100 });
    moveBy(screenElement, 100000, -100000);
    const common = { type: 'mouse', buttons: 0 };
    expect(input.postMessage.mock.calls).toEqual([
      [{ ...common, kind: 'move', x: 133, y: 128, dx: 5, dy: 8, wheel: 0 }],
      [{ ...common, kind: 'down', x: 133, y: 128, dx: 0, dy: 0, buttons: 1, wheel: 0 }],
      [{ ...common, kind: 'up', x: 133, y: 128, dx: 0, dy: 0, wheel: 0 }],
      [{ ...common, kind: 'wheel', x: 133, y: 128, dx: 0, dy: 0, wheel: 1 }],
      [{ ...common, kind: 'move', x: 256, y: 0, dx: 50000, dy: -57142.86, wheel: 0 }]
    ]);
  });

  it('退出捕获（按 Esc）时上报一次离开并松开按键，之后鼠标的移动、按下、松开、滚轮都不再送给机器（里面的鼠标不跟着动）', () => {
    const { screenElement } = setup({ captureMouse: true });
    const lock = stubPointerLock(screenElement);
    const input = fakePort();
    deliverPorts(fakePort(), input);
    sizeScreen(screenElement);
    fireEvent.pointerDown(screenElement, { clientX: 256, clientY: 240, buttons: 1 });
    moveBy(screenElement, 10, 14);
    input.postMessage.mockClear();
    lock.unlock();
    expect(input.postMessage.mock.calls).toEqual([[{ type: 'mouse', kind: 'leave', x: 133, y: 128, dx: 0, dy: 0, buttons: 0, wheel: 0 }]]);
    fireEvent.pointerMove(screenElement, { clientX: 256, clientY: 240, buttons: 0 });
    fireEvent.pointerUp(screenElement, { clientX: 256, clientY: 240, buttons: 0 });
    fireEvent.wheel(screenElement, { clientX: 256, clientY: 240, deltaY: 100 });
    fireEvent.pointerLeave(screenElement, { buttons: 0 });
    expect(input.postMessage).toHaveBeenCalledTimes(1);
  });

  it('捕获期间双击右键释放捕获，第二次右键按下不送给机器；单击右键照常送出', () => {
    const { screenElement } = setup({ captureMouse: true });
    const lock = stubPointerLock(screenElement);
    const input = fakePort();
    deliverPorts(fakePort(), input);
    sizeScreen(screenElement);
    fireEvent.pointerDown(screenElement, { clientX: 256, clientY: 240, buttons: 1 });
    expect(lock.request).toHaveBeenCalled();
    fireEvent.pointerDown(screenElement, { button: 2, buttons: 2 });
    expect(input.postMessage).toHaveBeenCalledTimes(1);
    expect(input.postMessage.mock.calls[0]?.[0]).toMatchObject({ kind: 'down', buttons: 2 });
    fireEvent.pointerDown(screenElement, { button: 2, buttons: 2 });
    expect(document.exitPointerLock).toHaveBeenCalledTimes(1);
    expect(input.postMessage.mock.calls.at(-1)?.[0]).toMatchObject({ kind: 'leave', buttons: 0 });
  });

  it('关掉设置时把已有的捕获退掉', () => {
    const { screenElement, rerender } = setup({ captureMouse: true });
    stubPointerLock(screenElement);
    deliverPorts(fakePort(), fakePort());
    sizeScreen(screenElement);
    fireEvent.pointerDown(screenElement, { clientX: 256, clientY: 240, buttons: 1 });
    rerender({ captureMouse: false });
    expect(document.exitPointerLock).toHaveBeenCalled();
  });
});

describe('RuntimeScreen：游戏手柄', () => {
  it('每一帧读一次手柄，变化时上报，卸载时上报断开', () => {
    const callbacks: FrameRequestCallback[] = [];
    vi.spyOn(window, 'requestAnimationFrame').mockImplementation((callback) => callbacks.push(callback));
    vi.spyOn(window, 'cancelAnimationFrame').mockImplementation(() => undefined);
    let pads: unknown[] = [null];
    Object.defineProperty(navigator, 'getGamepads', { configurable: true, value: () => pads });
    const { view } = setup();
    const input = fakePort();
    deliverPorts(fakePort(), input);
    const tick = () => act(() => callbacks.splice(0).forEach((callback) => callback(0)));

    tick();
    expect(input.postMessage).not.toHaveBeenCalled();
    pads = [{ index: 0, id: 'Test Pad', connected: true, buttons: [{ pressed: true }, { pressed: false }], axes: [0.5, -1] }];
    tick();
    tick();
    expect(input.postMessage.mock.calls).toEqual([[{ type: 'gamepad', index: 0, id: 'Test Pad', connected: true, buttons: [true, false], axes: [0.5, -1] }]]);
    view.unmount();
    expect(input.postMessage.mock.calls[1]).toEqual([{ type: 'gamepad', index: 0, id: 'Test Pad', connected: false, buttons: [], axes: [] }]);
    Reflect.deleteProperty(navigator, 'getGamepads');
  });
});
