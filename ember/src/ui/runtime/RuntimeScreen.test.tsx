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
function setup(props: { aspect?: '4:3' | '8:7'; scaling?: 'fit' | 'integer' } = {}) {
  const requestPorts = vi.fn();
  const bridge = { streams: { requestPorts }, core: {}, main: {}, onCoreEvent: () => () => undefined, getCoreStatus: () => Promise.reject(new Error('未使用')) } as unknown as EmberBridge;
  const screenRef: RefObject<HTMLDivElement | null> = createRef<HTMLDivElement>();
  const view = render(
    <UiProvider>
      <RuntimeScreen bridge={bridge} screenRef={screenRef} {...props} />
    </UiProvider>
  );
  const screenElement = view.getByRole('application', { name: '模拟器画面' });
  const worker = FakeWorker.instances[FakeWorker.instances.length - 1];
  if (worker === undefined) throw new Error('没有创建 Worker');
  return { view, screenElement, worker, requestPorts, rerender: (next: { aspect?: '4:3' | '8:7'; scaling?: 'fit' | 'integer' }) => view.rerender(<UiProvider><RuntimeScreen bridge={bridge} screenRef={screenRef} {...next} /></UiProvider>) };
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
  it('容器尺寸变化时按设备像素尺寸发版面，带上画面比例与缩放方式的默认值', () => {
    const { worker } = setup();
    act(() => resizeCallback?.([{ devicePixelContentBoxSize: [{ inlineSize: 1280, blockSize: 960 }] }]));
    const views = worker.messages('view');
    expect(views[views.length - 1]).toEqual({ type: 'view', view: { width: 1280, height: 960, aspect: '8:7', scaling: 'fit' } });
  });

  it('属性变化后重新发版面', () => {
    const { worker, rerender } = setup();
    act(() => resizeCallback?.([{ devicePixelContentBoxSize: [{ inlineSize: 800, blockSize: 600 }] }]));
    rerender({ aspect: '4:3', scaling: 'integer' });
    const views = worker.messages('view');
    expect(views[views.length - 1]).toEqual({ type: 'view', view: { width: 800, height: 600, aspect: '4:3', scaling: 'integer' } });
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
    pads = [{ index: 0, connected: true, buttons: [{ pressed: true }, { pressed: false }], axes: [0.5, -1] }];
    tick();
    tick();
    expect(input.postMessage.mock.calls).toEqual([[{ type: 'gamepad', index: 0, connected: true, buttons: [true, false], axes: [0.5, -1] }]]);
    view.unmount();
    expect(input.postMessage.mock.calls[1]).toEqual([{ type: 'gamepad', index: 0, connected: false, buttons: [], axes: [] }]);
    Reflect.deleteProperty(navigator, 'getGamepads');
  });
});
