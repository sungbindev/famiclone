/*
 * ------------------------------------------------------------------------
 * 名称：RuntimeScreen.test.tsx
 * 说明：画面组件的测试（Worker、画布与端口都用假对象）：创建 Worker 并交出离屏画布、收到端口后画面端口交给 Worker、版面随容器尺寸与属性变化、键盘事件经输入端口送出、失焦与卸载时松开全部按键、可编辑元素不采集、卸载时终止 Worker。
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
  it('按下与松开的按键经输入端口送出整套掩码，重复的按下不重复发送', () => {
    const { screenElement } = setup();
    const input = fakePort();
    deliverPorts(fakePort(), input);
    fireEvent.keyDown(screenElement, { code: 'KeyX' });
    fireEvent.keyDown(screenElement, { code: 'KeyX', repeat: true });
    fireEvent.keyDown(screenElement, { code: 'ArrowRight' });
    fireEvent.keyUp(screenElement, { code: 'KeyX' });
    expect(input.postMessage.mock.calls).toEqual([[{ type: 'buttons', port: 0, buttons: 0x01 }], [{ type: 'buttons', port: 0, buttons: 0x81 }], [{ type: 'buttons', port: 0, buttons: 0x80 }]]);
  });

  it('映射里的键阻止默认行为，其他键不阻止', () => {
    const { screenElement } = setup();
    deliverPorts(fakePort(), fakePort());
    expect(fireEvent.keyDown(screenElement, { code: 'Enter' })).toBe(false);
    expect(fireEvent.keyDown(screenElement, { code: 'KeyQ' })).toBe(true);
  });

  it('画面区失焦、窗口失焦、页面隐藏、卸载时松开全部按键', () => {
    const { view, screenElement } = setup();
    const input = fakePort();
    deliverPorts(fakePort(), input);
    const press = () => fireEvent.keyDown(screenElement, { code: 'KeyZ' });
    const released = () => input.postMessage.mock.calls.filter(([message]) => (message as { buttons: number }).buttons === 0).length;

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
